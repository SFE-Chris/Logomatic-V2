/*

    NES : 10/25/07
    Functions for the internal writing and reading of the
    LPC2148 flash space. Used for bootloading and Unit Serial
    Number manipulation.
	
	You must enable Thumb operation on your compiler for this to work!
	For WinARM, use this line (should already be there, just uncomment it):
	THUMB_IW = -mthumb-interwork

	User settings are from 0x8000 for 256 bytes (0x8100).
	Unit serial number is from 0x8000 + 256 - 16 = 0x80F0
	Unit boot loader version is 0x8000 + 256 - 32 = 0x80E0
*/

#include "iap_flash.h"
#include "LPC214x.h"
#include "rprintf.h"

#define IAP_LOCATION 0x7FFFFFF1

typedef void (*IAP)(unsigned int[],unsigned int[]);
IAP iap_entry;

//Records a 256 byte chunk to flash. This will destroy an entire sector however so be careful 
//not to store anything else in that settings sector
//Read above about Thumb mode requirements
void write_to_flash(char* settings_buffer_ptr)
{
	//rprintf("\n\nRecord to flash!\n\n");
	//return; //v2.45 Test firmware - remove any possible writes to flash

	unsigned int command[5];
	unsigned int result[2];
    
	//Disable all interrupts
	int tempVIC;
	tempVIC = VICIntEnable; //Read current interrupts
	VICIntEnClr = 0xFFFFFFFF; //Disable all interrupts

    iap_entry = (IAP)IAP_LOCATION;

    //Prep
    command[0] = 50;
    command[1] = SETTINGS_SEC_L;
    command[2] = SETTINGS_SEC_H;
    command[3] = 0;
    command[4] = 0;
    iap_entry(command, result);

    //Erase
    command[0] = 52;
    command[1] = SETTINGS_SEC_L;
    command[2] = SETTINGS_SEC_H;
    command[3] = SETTINGS_CCLK;
    command[4] = 0;
    iap_entry(command, result);

    //Prep
    command[0] = 50;
    command[1] = SETTINGS_SEC_L;
    command[2] = SETTINGS_SEC_H;
    command[3] = 0;
    command[4] = 0;
    iap_entry(command, result);

    //write
    command[0] = 51;
    command[1] = SETTINGS_ADDR_L;
    command[2] = (int)settings_buffer_ptr;
    command[3] = 256;
    command[4] = SETTINGS_CCLK;
    iap_entry(command, result);

	//Re-enable interrupts
    VICIntEnable = tempVIC;
}

//Prints the current serial number
void read_serialnumber(void)
{
	char *serialnumber;
	serialnumber = (char*)SERIALNUMBER_START;

	rprintf("\n\nUnit Serial #: %s", serialnumber);

}

//Writes new serial number to flash space from ptr
void write_serialnumber(char* new_serialnumber)
{
	int i;
	
	char new_settings[256];

	char *current_settings_ptr;
	current_settings_ptr = (char*)SETTINGS_ADDR_L;
	
	//Read the current settings
	for(i = 0 ; i < 256 ; i++)
	{
		new_settings[i] = current_settings_ptr[i];
	}
	
	//Splice in new serial number
	//Move 16 bytes of serial number data to settings array
	for(i = 0 ; i < 16 ; i++)
	{
		new_settings[i + (256-16)] = new_serialnumber[i];
	}
	
	//Record settings to memory
	write_to_flash(new_settings);
}

//Writes a blank record to flash
void erase_settings(void)
{
	int i;
	
	char new_settings[256];

	//Read the current settings
	for(i = 0 ; i < 256 ; i++)
	{
		new_settings[i] = 0xFF;
	}
	
	//Record settings to memory
	write_to_flash(new_settings);
}

//Returns a given spot in the current settings array
char read_option(int spot_num)
{
	char *settings_ptr;
	settings_ptr = (char*)USER_SETTING_START;

	return( settings_ptr[spot_num] );
}

//Writes a given option setting to flash
void write_option(int spot_number, unsigned char user_byte)
{
	int i;
	
	char new_settings[256];

	char *current_settings_ptr;
	current_settings_ptr = (char*)USER_SETTING_START;
	
	//Read the current settings
	for(i = 0 ; i < 256 ; i++)
	{
		new_settings[i] = current_settings_ptr[i];
	}
	
	//Splice in new byte
	new_settings[spot_number] = user_byte;
	
	//Record settings to memory
	write_to_flash(new_settings);
}
