/* identify the Entry Point  */

ENTRY(_startup)



/* specify the LPC2106 memory areas  */

MEMORY 
{
	flash     			: ORIGIN = 0,          LENGTH = 128K	/* FLASH ROM -Limited to 128k for bootloader               	*/	
	ram_isp_low(A)		: ORIGIN = 0x40000120, LENGTH = 223		/* variables used by Philips ISP bootloader	*/		 
	ram   				: ORIGIN = 0x40000200, LENGTH = 32513   /* free RAM area 				*/
	ram_isp_high(A)		: ORIGIN = 0x4007FFE0, LENGTH = 32		/* variables used by Philips ISP bootloader	*/
	ram_usb_dma	: ORIGIN = 0x7FD00000, LENGTH = 8192
	CONFIG(rw) : ORIGIN = 0x3a000, LENGTH = 0x1FFF
}


/* define a global symbol _stack_end  */

_stack_end = 0x40007EDC;



/* now define the output sections  */

SECTIONS 
{
	. = 0;								/* set location counter to address zero  */
	
	startup : { *(.startup)} >flash		/* the startup code goes into FLASH */

	/* Create spot in flash for hard-coded boot loader serial number */
	.bl_version_number : AT (0x80E0) 
	{ 
		*(.bl_version_number) 
	} >test

	.text :								/* collect all sections that should go into FLASH after startup  */ 
	{
		*(.text)						/* all .text sections (code)  */
		*(.rodata)						/* all .rodata sections (constants, strings, etc.)  */
		*(.rodata*)						/* all .rodata* sections (constants, strings, etc.)  */
		*(.glue_7)						/* all .glue_7 sections  (no idea what these are) */
		*(.glue_7t)						/* all .glue_7t sections (no idea what these are) */
		_etext = .;						/* define a global symbol _etext just after the last code byte */
	} >flash							/* put all the above into FLASH */
	

	
        . = ALIGN(4);
	.data :								/* collect all initialized .data sections that go into RAM  */ 
	{
		_data = .;						/* create a global symbol marking the start of the .data section  */
		*(.data)						/* all .data sections  */
		_edata = .;						/* define a global symbol marking the end of the .data section  */
	} >ram AT >flash					/* put all the above into RAM (but load the LMA copy into FLASH) */
        
        . = ALIGN(4);
	.bss :								/* collect all uninitialized .bss sections that go into RAM  */
	{
		_bss_start = .;					/* define a global symbol marking the start of the .bss section */
		*(.bss)							/* all .bss sections  */
	} >ram								/* put all the above in RAM (it will be cleared in the startup code */

	. = ALIGN(4);						/* advance location counter to the next 32-bit boundary */
	_bss_end = . ;						/* define a global symbol marking the end of the .bss section */
}
	_end = .;							/* define a global symbol marking the end of application RAM */
	end = .;
	_heap = .;
        . += 0x1000;
