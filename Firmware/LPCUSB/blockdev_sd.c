/*****************************************************************************\
*              efs - General purpose Embedded Filesystem library              *
*          --------------------- -----------------------------------          *
*                                                                             *
* Filename : sd.c                                                             *
* Revision : Initial developement                                             *
* Description : This file contains the functions needed to use efs for        *
*               accessing files on an SD-card.                                *
*                                                                             *
* This library is free software; you can redistribute it and/or               *
* modify it under the terms of the GNU Lesser General Public                  *
* License as published by the Free Software Foundation; either                *
* version 2.1 of the License, or (at your option) any later version.          *
*                                                                             *
* This library is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           *
* Lesser General Public License for more details.                             *
*                                                                             *
*                                                    (c)2005 Michael De Nil   *
*                                                    (c)2005 Lennart Yseboodt *
\*****************************************************************************/

/*
    2006, Bertrik Sikken, modified for LPCUSB
*/

/*
    SDHC/FAT32 support added 10/10/2011 by Magnus Karlsson 
*/

#include "type.h"

#include <stdio.h>
#include "rprintf.h"

#include "blockdev.h"
#include "spi.h"

#define CMD_GOIDLESTATE     0
#define CMD_SENDOPCOND      1
#define CMD_SENDIFCOND      8
#define CMD_READCSD         9
#define CMD_READCID         10
#define CMD_SENDSTATUS      13
#define CMD_READSINGLEBLOCK 17
#define CMD_WRITE           24
#define CMD_WRITE_MULTIPLE  25
#define CMD_SDSENDOPCOND    41
#define CMD_APPCMD          55
#define CMD_READOCR         58

U8 highCapacity;

static void Command(U8 cmd, U32 param)
{
    U8  abCmd[8];

    // create buffer
    abCmd[0] = 0xff;
    abCmd[1] = 0x40 | cmd;
    abCmd[2] = (U8)(param >> 24);
    abCmd[3] = (U8)(param >> 16);
    abCmd[4] = (U8)(param >> 8);
    abCmd[5] = (U8)(param);
    if (cmd == CMD_GOIDLESTATE)
        abCmd[6] = 0x95;            /* Valid CRC for CMD0(0)*/
    else if (cmd == CMD_SENDIFCOND)
        abCmd[6] = 0x87;            /* Valid CRC for CMD8(0x1AA)  */
    else 
        abCmd[6] = 0x01;            /* Dummy CRC + Stop*/
    abCmd[7] = 0xff;            /* eat empty command - response */

    SPISendN(abCmd, 8);
}

/*****************************************************************************/

static U8 Resp8b(void)
{
    U8 i;
    U8 resp;

    /* Respone will come after 1 - 8 pings */
    for (i = 0; i < 8; i++)
    {
        resp = SPISend(0xff);
        if (resp != 0xff)
        {
            return resp;
        }
    }

    return resp;
}

/*****************************************************************************/

static void Resp8bError(U8 value)
{
    switch (value)
    {
        case 0x40:  rprintf("Argument out of bounds.\n");               break;
        case 0x20:  rprintf("Address out of bounds.\n");                break;
        case 0x10:  rprintf("Error during erase sequence.\n");          break;
        case 0x08:  rprintf("CRC failed.\n");                           break;
        case 0x04:  rprintf("Illegal command.\n");                      break;
        case 0x02:  rprintf("Erase reset (see SanDisk docs p5-13).\n"); break;
//        case 0x01:  rprintf("Card is initialising.\n");                 break;
        case 0x01:  rprintf(".");                 break;
            default:
            rprintf("Unknown error 0x%x (see SanDisk docs p5-13).\n", value);
            break;
    }
}


/* ****************************************************************************
 calculates size of card from CSD
 (extension by Martin Thomas, inspired by code from Holger Klabunde)
 CSD version 2.0 code by Magnus Karlsson
 */
int BlockDevGetSize(U32 *pdwDriveSize)
{
    U8 cardresp, i, by;
    U8 iob[16];
    U32 c_size, blocks; 
    U16 c_size_mult, read_bl_len;

    Command(CMD_READCSD, 0);
    do
    {
        cardresp = Resp8b();
    }
    while (cardresp != 0xFE);

    rprintf("CSD:");
    for (i = 0; i < 16; i++)
    {
        iob[i] = SPISend(0xFF);
        rprintf(" %02x", iob[i]);
    }
    rprintf("\n");

    SPISend(0xff);
    SPISend(0xff);

    if ((iob[0] & 0x80) == 0x80)
    {
        return -1; // Illegal CSD version data
    }
    else if ((iob[0] & 0x40) == 0) // CSD version 1.0
    {
        c_size = iob[6] & 0x03;     // bits 1..0
        c_size <<= 10;
        c_size += (U16) iob[7] << 2;
        c_size += iob[8] >> 6;

        by = iob[5] & 0x0F;
        read_bl_len = 1 << by;

        by = iob[9] & 0x03;
        by <<= 1;
        by += iob[10] >> 7;

        c_size_mult = 1 << (2 + by);
        blocks = ((c_size + 1) * (U32) c_size_mult *(U32) read_bl_len) / 512;
    }
    else // CSD version 2.0
    {
        c_size = (iob[9] | (iob[8] << 8) | (iob[7] << 16)) & 0x003fffff;
        blocks = (c_size + 1) * 1024;
    }

    *pdwDriveSize = (blocks);
    rprintf("Drive size: %u blocks\n", blocks);

    return 0;
}

/*****************************************************************************/

static U16 Resp16b(void)
{
    U16 resp;

    resp = (Resp8b() << 8) & 0xff00;
    resp |= SPISend(0xff);

    return resp;
}

/*****************************************************************************/

static U32 Resp32b(void)
{
    U32 resp;

    resp = SPISend(0xff) << 24;
    resp |= (SPISend(0xff) << 16);
    resp |= (SPISend(0xff) << 8);
    resp |= SPISend(0xff);

    return resp;
}

/*****************************************************************************/

static int State(void)
{
    U16 value;

    Command(CMD_SENDSTATUS, 0);
    value = Resp16b();

    switch (value)
    {
        case 0x0000: return 1;
        case 0x0001: rprintf("Card is Locked.\n");                                                  break;
        case 0x0002: rprintf("WP Erase Skip, Lock/Unlock Cmd Failed.\n");                           break;
        case 0x0004: rprintf("General / Unknown error -- card broken?.\n");                         break;
        case 0x0008: rprintf("Internal card controller error.\n");                                  break;
        case 0x0010: rprintf("Card internal ECC was applied, but failed to correct the data.\n");   break;
        case 0x0020: rprintf("Write protect violation.\n");                                         break;
        case 0x0040: rprintf("An invalid selection, sectors for erase.\n");                         break;
        case 0x0080: rprintf("Out of Range, CSD_Overwrite.\n");                                     break;
            default:
            if (value > 0x00FF)
            {
                Resp8bError((U8) (value >> 8));
            }
            else
            {
                rprintf("Unknown error: 0x%x (see SanDisk docs p5-14).\n", value);
            }
            break;
    }
    return -1;
}

/*****************************************************************************/


int BlockDevInit(void)
{
    int i;
    U8 resp;
    U32 resp32;

    SPIInit();              /* init at low speed */

    /* Try to send reset command up to 100 times */
    i = 100;
    do
    {
        Command(CMD_GOIDLESTATE, 0);
        resp = Resp8b();
    }
    while (resp != 1 && i--);

    if (resp != 1)
    {
        if (resp == 0xff)
        {
            rprintf("resp=0xff\n");
            return -1;
        }
        else
        {
            Resp8bError(resp);
            rprintf("resp!=0xff\n");
            return -2;
        }
    }

    highCapacity = 0;

    Command(CMD_SENDIFCOND, 0x1AA);
    resp = Resp8b();
    if (resp == 0x01) // SD Version 2.x
    {
        rprintf("SD Version 2.x card\n");
        resp32 = Resp32b();
        if ((resp32 & 0x00000fff) != 0x1AA)
        {
             rprintf("Bad CMD8 response\n");
             return -4;
        }
        rprintf("Waiting for card ready");
        /* Wait till card is ready initialising (returns 0 on ACMD_41) */
        /* Try up to 32000 times. */
        i = 32000;
        do
        {
            Command(CMD_APPCMD, 0);
            resp = Resp8b();
            if (resp != 0)
            {
                Resp8bError(resp);
            }
            Command(CMD_SDSENDOPCOND, 0x40000000); // ACMD41 with HCS bit set
            resp = Resp8b();
            if (resp != 0)
            {
                Resp8bError(resp);
            }
        }
        while (resp == 1 && i--);

        if (resp != 0)
        {
            Resp8bError(resp);
            return -3;
        }
        
        // Issue CMD58 to read OCR
        Command(CMD_READOCR, 0);
        resp = Resp8b();
        if (resp != 0)
        {
            Resp8bError(resp);
        }
        resp32 = Resp32b();
        if (resp32 & 0x40000000) // check the CCS bit
        {
            rprintf("High capacity SD card detected\n");
            highCapacity = 1; // high capacity card
        }
        else
        {
            rprintf("Standard capacity SD card detected\n");
        }
    }
    else // SD version 1.x
    {
        /* Wait till card is ready initialising (returns 0 on CMD_1) */
        /* Try up to 32000 times. */
        i = 32000;
        do
        {
            Command(CMD_SENDOPCOND, 0);
            resp = Resp8b();
            if (resp != 0)
            {
                Resp8bError(resp);
            }
        }
        while (resp == 1 && i--);

        if (resp != 0)
        {
            Resp8bError(resp);
            return -3;
        }
    }


    
    /* increase speed after init */
    SPISetSpeed(SPI_PRESCALE_MIN);

    if (State() < 0)
    {
        rprintf("Card didn't return the ready state, breaking up...\n");
        return -2;
    }

    rprintf("SD Init done...\n");

    return 0;
}

/*****************************************************************************/



/*****************************************************************************/


/*****************************************************************************/

/* ****************************************************************************
 * WAIT ?? -- FIXME
 * CMD_WRITE
 * WAIT
 * CARD RESP
 * WAIT
 * DATA BLOCK OUT
 *      START BLOCK
 *      DATA
 *      CHKS (2B)
 * BUSY...
 */

int BlockDevWrite(U32 dwAddress, U8 * pbBuf)
{
    U32 place;
    U16 t = 0;

    if (!highCapacity)
        place = 512 * dwAddress;
    else
        place = dwAddress;
    Command(CMD_WRITE, place);

    Resp8b();               /* Card response */

    SPISend(0xfe);          /* Start block */
    SPISendN(pbBuf, 512);
    SPISend(0xff);          /* Checksum part 1 */
    SPISend(0xff);          /* Checksum part 2 */

    SPISend(0xff);

    while (SPISend(0xff) != 0xff)
    {
        t++;
    }

    return 0;
}

/*****************************************************************************/

/* ****************************************************************************
 * WAIT ?? -- FIXME
 * CMD_CMD_
 * WAIT
 * CARD RESP
 * WAIT
 * DATA BLOCK IN
 *      START BLOCK
 *      DATA
 *      CHKS (2B)
 */

int BlockDevRead(U32 dwAddress, U8 * pbBuf)
{
    U8 cardresp;
    U8 firstblock;
    U16 fb_timeout = 0xffff;
    U32 place;

    if (!highCapacity)
        place = 512 * dwAddress;
    else
        place = dwAddress;
    Command(CMD_READSINGLEBLOCK, place);

    cardresp = Resp8b();        /* Card response */

    /* Wait for startblock */
    do
    {
        firstblock = Resp8b();
    }
    while (firstblock == 0xff && fb_timeout--);

    if (cardresp != 0x00 || firstblock != 0xfe)
    {
        Resp8bError(firstblock);
        return -1;
    }

    SPIRecvN(pbBuf, 512);

    /* Checksum (2 byte) - ignore for now */
    SPISend(0xff);
    SPISend(0xff);

    return 0;
}

/*****************************************************************************/
