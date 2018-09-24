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

void write_to_flash(char* settings_buffer_ptr);
void read_serialnumber(void);
void write_serialnumber(char* new_serialnumber_ptr);
void erase_settings(void);

char read_option(int spot_num);
void write_option(int spot_number, unsigned char user_byte);

//The following defines where each item is located in sector 8 flash
#define SETTINGS_SEC_L        8               //Flash sector where you want settings records to begin (see UM for details)
#define SETTINGS_SEC_H        8               //Flash sector where you want settings records to end (see UM for details)
#define SETTINGS_ADDR_L       0x00008000      //Must match the SETTINGS_SEC_L Flash sector start address
#define SETTINGS_ADDR_H       0x0000FFFF      //Must match the SETTINGS_SEC_H Flash sector end address
#define SETTINGS_CCLK         60000           //system clock cclk expressed in kHz (5*12 MHz)

#define SERIALNUMBER_START		((SETTINGS_ADDR_L + 256) - 16) //Location that the 16 byte serial number location resides

#define USER_SETTING_START SETTINGS_ADDR_L

#define SCREEN_FONT_TYPE	0 //Screen type (font size large/small) stored in options array[0]
#define GPS_PWR_EN			1 //Enable GPS power or turn it off for races
#define LAST_LOG_NUM		2 //Last log number so we can quickly search for next available log
#define GPS_HOUR_OFFSET		3 //Integer hour difference between local and UTC time
#define SPEED_TYPE			4 //m/s, miles/hr, km/hr speed display type
