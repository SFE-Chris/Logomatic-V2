#ifndef _STRING_PRINTF_H_
#define _STRING_PRINTF_H_

extern void string_myputchar(char* string_printf_buffer, unsigned char c);
extern void string_printf_devopen( int(*put)(int) );
extern void string_printf(char* string_printf_buffer, char const *format, ... );

#endif
