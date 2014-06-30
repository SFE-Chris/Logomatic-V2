/*
	SparkFun's ARM Bootloader
	7/28/08
	
	Bootload binary file(FW.SFE) over USB
  
    Modified 10/10/2011 by Magnus Karlsson for FAT32 support

*/	
#include "LPC214x.h"

//UART0 Debugging
#include "serial.h"
#include "rprintf.h"

//Memory manipulation and basic system stuff
#include "firmware.h"
#include "system.h"

//SD Logging
#include "rootdir.h"
#include "sd_raw.h"

//USB
#include "main_msc.h"

//This is the file name that the bootloader will scan for
#define FW_FILE "FW.SFE"

int main (void)
{
	boot_up();						//Initialize USB port pins and set up the UART
	rprintf("Boot up complete\n");	

	if(IOPIN0 & (1<<23))			//Check to see if the USB cable is plugged in
	{
		main_msc();					//If so, run the USB device driver.
	}
	else{
		rprintf("No USB Detected\n");
	}
	
	//Init SD
	if(sd_raw_init())				//Initialize the SD card
	{
		openroot();					//Open the root directory on the SD card
		rprintf("Root open\n");
		if(root_file_exists(FW_FILE))	//Check to see if the firmware file is residing in the root directory
		{
			rprintf("New firmware found\n");
			load_fw(FW_FILE);			//If we found the firmware file, then program it's contents into memory.
			rprintf("New firmware loaded\n");
		}
	}
	else{
		//Didn't find a card to initialize
		rprintf("No SD Card Detected\n");
		delay_ms(250);
	}
	rprintf("Boot Done. Calling firmware...\n");
	call_firmware();					//Run the new code!

	while(1);
}
