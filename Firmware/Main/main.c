/*********************************************************************************
 * Logomatic V2 Firmware
 * Sparkfun Electronics 2008
 * ******************************************************************************/

/*******************************************************
 *         Header Files
 ******************************************************/
#include <stdio.h>
#include <string.h>
#include "LPC21xx.h"

//UART0 Debugging
#include "serial.h"
#include "rprintf.h"

//Needed for main function calls
#include "main_msc.h"
#include "fat.h"
#include "armVIC.h"
#include "itoa.h"
#include "rootdir.h"
#include "sd_raw.h"
#include "string_printf.h"


/*******************************************************
 *         Global Variables
 ******************************************************/

#define ON  1
#define OFF 0

#define BUF_SIZE 512

char RX_array1[BUF_SIZE];
char RX_array2[BUF_SIZE];
char log_array1 = 0;
char log_array2 = 0;
short RX_in = 0;
char get_frame = 0;

signed int stringSize;
struct fat_file_struct* handle;
struct fat_file_struct * fd;
char stringBuf[256];

// Default Settings
static char   mode = 0;
static char    asc = 'N';
static int    baud = 9600;
static int    freq = 100;
static char   trig = '$';
static short frame = 100;
static char  ad1_7 = 'N';
static char  ad1_6 = 'N';
static char  ad1_3 = 'N';
static char  ad1_2 = 'N';
static char  ad0_4 = 'N';
static char  ad0_3 = 'N';
static char  ad0_2 = 'N';
static char  ad0_1 = 'N';


/*******************************************************
 *     Function Declarations
 ******************************************************/

void Initialize(void);

void setup_uart0(int newbaud, char want_ints);

void mode_0(void);
void mode_1(void);
void mode_2(void);
void mode_action(void);

void Log_init(void);
void test(void);
void stat(int statnum, int onoff);
void AD_conversion(int regbank);

void feed(void);

//BHW Never defined... static void IRQ_Routine(void) __attribute__ ((interrupt("IRQ")));
static void UART0ISR(void); //__attribute__ ((interrupt("IRQ")));
static void UART0ISR_2(void); //__attribute__ ((interrupt("IRQ")));
static void MODE2ISR(void); //__attribute__ ((interrupt("IRQ")));

void FIQ_Routine(void) __attribute__ ((interrupt("FIQ")));
void SWI_Routine(void) __attribute__ ((interrupt("SWI")));
void UNDEF_Routine(void) __attribute__ ((interrupt("UNDEF")));

void fat_initialize(void);

void delay_ms(int count);


/*******************************************************
 *          MAIN
 ******************************************************/

int main (void)
{
  int i;
  char name[32];
  int count = 0;
  
  enableFIQ();
  
  Initialize();
  
  setup_uart0(9600, 0);

  fat_initialize();   


  // Flash Status Lights
  for(i = 0; i < 5; i++)
  {
    stat(0,ON);
    delay_ms(50);
    stat(0,OFF);
    stat(1,ON);
    delay_ms(50);
    stat(1,OFF);
  }
  
  Log_init();

  count++;  //BHW TODO: this makes the count start from 1, not 0 to match docs
  string_printf(name,"LOG%02d.txt",count);
  while(root_file_exists(name))
  {
    count++;
    if(count == 250)  //BHW TODO: This less than 250 limit doesn't match docs
    {
      rprintf("Too Many Logs!\n\r");
      while(1)
      {
        stat(0,ON);
        stat(1,ON);
        delay_ms(1000);
        stat(0,OFF);
        stat(1,OFF);
        delay_ms(1000);
      }

    }
    string_printf(name,"LOG%02d.txt",count);
  }
  
  handle = root_open_new(name);
    

  sd_raw_sync();  
    
  if(mode == 0){ mode_0(); }
  else if(mode == 1){ mode_1(); }
  else if(mode == 2){ mode_2(); }

      return 0;
}


/*******************************************************
 *         Initialize
 ******************************************************/

#define PLOCK 0x400

void Initialize(void)
{
  rprintf_devopen(putc_serial0);
  
  // 0xCF351505 = 0b 11 00 11 11 00 11 01 01 00 01 01 01 00 00 01 01
  // 0x15441801 = 0b 00 01 01 01 01 00 01 00 00 01 10 00 00 00 00 01
  //
  // Symbol | Value | Function
  // -------|-------|----------------------------------
  // PINSEL0|       |
  // P0.0   | 01    | TXD (UART0)
  // P0.1   | 01    | RxD (UART0)
  // P0.2   | 00    | GPIO Port 0.2
  // P0.3   | 00    | GPIO Port 0.3
  // P0.4   | 01    | SCK0 (SPI0)
  // P0.5   | 01    | MISO0 (SPI0)
  // P0.6   | 01    | MOSI0 (SPI0)
  // P0.7   | 00    | GPIO Port 0.7
  // P0.8   | 01    | TXD (UART1)
  // P0.9   | 01    | RXD (UART1)
  // P0.10  | 11    | AD1.2
  // P0.11  | 00    | GPIO Port 0.11
  // P0.12  | 11    | AD1.3
  // P0.13  | 11    | AD1.4
  // P0.14  | 00    | GPIO Port 0.14
  // P0.15  | 11    | AD1.5
  // PINSEL1|       |
  // P0.16  | 01    | EINT0
  // P0.17  | 00    | GPIO Port 0.17
  // P0.18  | 00    | GPIO Port 0.18
  // P0.19  | 00    | GPIO Port 0.19
  // P0.20  | 00    | GPIO Port 0.20
  // P0.21  | 10    | AD1.6
  // P0.22  | 01    | AD1.7
  // P0.23  | 00    | GPIO Port 0.23
  // P0.24  | 00    | Reserved
  // P0.25  | 01    | AD0.4
  // P0.26  | 00    | Reserved
  // P0.27  | 01    | Reserved
  // P0.28  | 01    | AD0.1
  // P0.29  | 01    | AD0.2
  // P0.30  | 01    | AD0.3
  // P0.31  | 00    | GPO Port only

  PINSEL0 = 0xCF351505;
  PINSEL1 = 0x15441801;

  // P0.0  = INPUT  | TXD (UART0)
  // P0.1  = INPUT  | RxD (UART0)
  // P0.2  = OUTPUT | STAT0 LED
  // P0.3  = INPUT  | STOP Button
  // P0.4  = INPUT  | SCK0 (SPI0)
  // P0.5  = INPUT  | MISO0 (SPI0)
  // P0.6  = INPUT  | MOSI0 (SPI0)
  // P0.7  = OUTPUT | Chip Select 0
  // P0.8  = INPUT  | TXD (UART1)
  // P0.9  = INPUT  | RXD (UART1)
  // P0.10 = INPUT  | AD1.2
  // P0.11 = OUTPUT | STAT1 LED
  // Rest of Port 0 are inputs
  IODIR0 |= 0x00000884;
  IOSET0 = 0x00000080;  // Set P0.7 HIGH | CS0 HIGH

  S0SPCR = 0x08;  // SPI clk to be pclk/8
  S0SPCR = 0x30;  // master, msb, first clk edge, active high, no ints

}

// Make values in PLL control & configure registers take effect 
void feed(void)
{
  // Interrupts must be disabled to make consecutive APB bus cycles
  PLLFEED=0xAA;
  PLLFEED=0x55;
}

static void UART0ISR(void)
{
  if(RX_in < BUF_SIZE)
  {
    RX_array1[RX_in] = U0RBR;
  
    RX_in++;

    if(RX_in == BUF_SIZE) log_array1 = 1;
  }
  else if(RX_in >= BUF_SIZE)
  {
    RX_array2[RX_in-BUF_SIZE] = U0RBR;
    RX_in++;

    if(RX_in == 2 * BUF_SIZE)
    {
      log_array2 = 1;
      RX_in = 0;
    }
  }


  U0IIR; // Have to read this to clear the interrupt 

  VICVectAddr = 0;  // Acknowledge interrupt
}

static void UART0ISR_2(void)
{
  char temp;
  temp = U0RBR; // Read a byte from UART0 receive buffer

  if(temp == trig)
  {
    get_frame = 1;
  }
  
  if(get_frame)
  {
    if(RX_in < frame)
    {
      RX_array1[RX_in] = temp;
      RX_in++;

      if(RX_in == frame)
      {
        // Delimiters
        RX_array1[RX_in] = '\n';
        RX_array1[RX_in + 1] = '\r';
        log_array1 = 1;
        get_frame = 0;
      }
    }
    else if(RX_in >= frame)
    {
      RX_array2[RX_in - frame] = temp;
      RX_in++;

      if(RX_in == 2*frame)
      {
        // Delimiters
        RX_array2[RX_in - frame] = '\n';
        RX_array2[RX_in + 1 - frame] = '\r';
        log_array2 = 1;
        get_frame = 0;
        RX_in = 0;
      }
    }
  }

  temp = U0IIR; // Have to read this to clear the interrupt

  VICVectAddr = 0;  // Acknowledge interrupt
}

static inline int pushValue(char* q, int ind, int value)
{
  char* p = q + ind;

  if(asc == 'Y') // ASCII
  {
    // itoa returns the number of bytes written excluding
    // trailing '\0', hence the "+ 1"
    return itoa(value, p, 10) + ind + 1;
  }
  else if(asc == 'N') // binary
  {
    p[0] = value >> 8;
    p[1] = value;
    return ind + 2;
  }
  else // invalid
  {
    return ind;
  }
}

static int sample(char* q, int ind, volatile unsigned long* ADxCR,
                  volatile unsigned long* ADxDR, int mask, char adx_bit)
{
  if(adx_bit == 'Y')
  {
    int value = 0;

    *ADxCR = 0x00020FF00 | mask;
    *ADxCR |= 0x01000000;  // start conversion
    while((value & 0x80000000) == 0)
    {
      value = *ADxDR;
    }
    *ADxCR = 0x00000000;

    // The upper ten of the lower sixteen bits of 'value' are the
    // result. The result itself is unsigned. Hence a cast to
    // 'unsigned short' yields the result with six bits of
    // noise. Those are removed by the following shift operation.
    return pushValue(q, ind, (unsigned short)value >> 6);
  }
  else
  {
    return ind;
  }
}

static void MODE2ISR(void)
{
  int ind = 0;
  int j;
  char q[50];


  T0IR = 1; // reset TMR0 interrupt
  
  for(j = 0; j < 50; j++)
  {
    q[j] = 0;
  }


#define SAMPLE(X, BIT) ind = sample(q, ind, &AD##X##CR, &AD##X##DR, 1 << BIT, ad##X##_##BIT)
  SAMPLE(1, 3);
  SAMPLE(0, 3);
  SAMPLE(0, 2);
  SAMPLE(0, 1);
  SAMPLE(1, 2);
  SAMPLE(0, 4);
  SAMPLE(1, 7);
  SAMPLE(1, 6);
#undef SAMPLE
  
  for(j = 0; j < ind; j++)
  {
    if(RX_in < BUF_SIZE)
    {
      RX_array1[RX_in] = q[j];
      RX_in++;

      if(RX_in == BUF_SIZE) log_array1 = 1;
    }
    else if(RX_in >= BUF_SIZE)
    {
      RX_array2[RX_in - BUF_SIZE] = q[j];
      RX_in++;

      if(RX_in == 2 * BUF_SIZE)
      {
        log_array2 = 1;
        RX_in = 0;
      }
    }
  }
  if(RX_in < BUF_SIZE)
  {
    if(asc == 'N') { RX_array1[RX_in] = '$'; }
    else if(asc == 'Y'){ RX_array1[RX_in] = 13; }
    RX_in++;

    if(RX_in == BUF_SIZE) log_array1 = 1;
  }
  else if(RX_in >= BUF_SIZE)
  {
    
    if(asc == 'N') RX_array2[RX_in - BUF_SIZE] = '$';
    else if(asc == 'Y'){ RX_array2[RX_in - BUF_SIZE] = 13; }
    RX_in++;
    
    if(RX_in == 2 * BUF_SIZE)
    {
      log_array2 = 1;
      RX_in = 0;
    }
  }
  if(RX_in < BUF_SIZE)
  {
    if(asc == 'N') RX_array1[RX_in] = '$';
    else if(asc == 'Y'){ RX_array1[RX_in] = 10; }
    RX_in++;

    if(RX_in == BUF_SIZE) log_array1 = 1;
  }
  else if(RX_in >= BUF_SIZE)
  {
    
    if(asc == 'N') RX_array2[RX_in - BUF_SIZE] = '$';
    else if(asc == 'Y'){ RX_array2[RX_in - BUF_SIZE] = 10; }
    RX_in++;
    
    if(RX_in == 2 * BUF_SIZE)
    {
      log_array2 = 1;
      RX_in = 0;
    }
  }

  VICVectAddr = 0;  // Acknowledge interrupt
}

void FIQ_Routine(void)
{
  int j;

  stat(0,ON);
  for(j = 0; j < 5000000; j++); // TODO: Why are we using a blocking delay n ISR
  stat(0,OFF);
  U0RBR;  // Trash oldest byte in UART0 Rx FiFO Why??

  U0IIR;  // Have to read this to clear the interrupt

  // TODO: Should we be acking int here?
}

void SWI_Routine(void)
{
  while(1);
}

void UNDEF_Routine(void)
{
  stat(0,ON);
}

void setup_uart0(int newbaud, char want_ints)
{
  baud = newbaud;
  U0LCR = 0x83;   // 8 bits, no parity, 1 stop bit, DLAB = 1
  
  if(baud == 1200)
  {
    U0DLM = 0x0C;
    U0DLL = 0x00;
  }
  else if(baud == 2400)
  {
    U0DLM = 0x06;
    U0DLL = 0x00;
  }
  else if(baud == 4800)
  {
    U0DLM = 0x03;
    U0DLL = 0x00;
  }
  else if(baud == 9600)
  {
    U0DLM = 0x01;
    U0DLL = 0x80;
  }
  else if(baud == 19200)
  {
    U0DLM = 0x00;
    U0DLL = 0xC0;
  }
  else if(baud == 38400)
  {
    U0DLM = 0x00;
    U0DLL = 0x60;
  }
  else if(baud == 57600)
  {
    U0DLM = 0x00;
    U0DLL = 0x40;
  }
  else if(baud == 115200)
  {
    U0DLM = 0x00;
    U0DLL = 0x20;
  }

  U0FCR = 0x01;
  U0LCR = 0x03;   

  if(want_ints == 1)
  {
    enableIRQ();
    VICIntSelect &= ~0x00000040;
    VICIntEnable |= 0x00000040;
    VICVectCntl1 = 0x26;
    VICVectAddr1 = (unsigned int)UART0ISR;
    U0IER = 0x01;
  }
  else if(want_ints == 2)
  {
    enableIRQ();
    VICIntSelect &= ~0x00000040;
    VICIntEnable |= 0x00000040;
    VICVectCntl2 = 0x26;
    VICVectAddr2 = (unsigned int)UART0ISR_2;
    U0IER = 0X01;
  }
  else if(want_ints == 0)
  {
    VICIntEnClr = 0x00000040;
    U0IER = 0x00;
  }
}
void stat(int statnum, int onoff)
{
  if(statnum) // Stat 1
  {
    if(onoff){ IOCLR0 = 0x00000800; } // On
    else { IOSET0 = 0x00000800; } // Off
  }
  else // Stat 0 
  {
    if(onoff){ IOCLR0 = 0x00000004; } // On
    else { IOSET0 = 0x00000004; } // Off
  }
}

void Log_init(void)
{
  int x, mark = 0, ind = 0;
  char temp, temp2 = 0, safety = 0;
//  signed char handle;

  if(root_file_exists("LOGCON.txt"))
  {
    //rprintf("\n\rFound LOGcon.txt\n");
    fd = root_open("LOGCON.txt");
    stringSize = fat_read_file(fd, (unsigned char *)stringBuf, 512);
    stringBuf[stringSize] = '\0';
    fat_close_file(fd);
  }
  else
  {
    //rprintf("Couldn't find LOGcon.txt, creating...\n");
    fd = root_open_new("LOGCON.txt");
    if(fd == NULL)
    {
      rprintf("Error creating LOGCON.txt, locking up...\n\r");
      while(1)
      {
        stat(0,ON);
        delay_ms(50);
        stat(0,OFF);
        stat(1,ON);
        delay_ms(50);
        stat(1,OFF);
      }
    }

    strcpy(stringBuf, "MODE = 0\r\nASCII = N\r\nBaud = 4\r\nFrequency = 100\r\nTrigger Character = $\r\nText Frame = 100\r\nAD1.3 = N\r\nAD0.3 = N\r\nAD0.2 = N\r\nAD0.1 = N\r\nAD1.2 = N\r\nAD0.4 = N\r\nAD1.7 = N\r\nAD1.6 = N\r\nSafety On = Y\r\n");
    stringSize = strlen(stringBuf);
    fat_write_file(fd, (unsigned char*)stringBuf, stringSize);
    sd_raw_sync();
  }

  for(x = 0; x < stringSize; x++)
  {
    temp = stringBuf[x];
    if(temp == 10)
    {
      mark = x;
      ind++;
      if(ind == 1)
      {
        mode = stringBuf[mark-2]-48; // 0 = auto uart, 1 = trigger uart, 2 = adc
        rprintf("mode = %d\n\r",mode);
      }
      else if(ind == 2)
      {
        asc = stringBuf[mark-2]; // default is 'N'
        rprintf("asc = %c\n\r",asc);
      }
      else if(ind == 3)
      {
        if(stringBuf[mark-2] == '1'){ baud = 1200; }
        else if(stringBuf[mark-2] == '2'){ baud = 2400; }
        else if(stringBuf[mark-2] == '3'){ baud = 4800; }
        else if(stringBuf[mark-2] == '4'){ baud = 9600; }
        else if(stringBuf[mark-2] == '5'){ baud = 19200; }
        else if(stringBuf[mark-2] == '6'){ baud = 38400; }
        else if(stringBuf[mark-2] == '7'){ baud = 57600; }
        else if(stringBuf[mark-2] == '8'){ baud = 115200; }

        rprintf("baud = %d\n\r",baud);
      }
      else if(ind == 4)
      {
        freq = (stringBuf[mark-2]-48) + (stringBuf[mark-3]-48) * 10;
        if((stringBuf[mark-4] >= 48) && (stringBuf[mark-4] < 58))
        {
          freq+= (stringBuf[mark-4]-48) * 100;
          if((stringBuf[mark-5] >= 48) && (stringBuf[mark-5] < 58)){ freq += (stringBuf[mark-5]-48)*1000; }
        }
        rprintf("freq = %d\n\r",freq);
      }
      else if(ind == 5)
      {
        trig = stringBuf[mark-2]; // default is $
        
        rprintf("trig = %c\n\r",trig);
      }
      else if(ind == 6)
      {
        frame = (stringBuf[mark-2]-48) + (stringBuf[mark-3]-48) * 10 + (stringBuf[mark-4]-48)*100;
        if(frame > 510){ frame = 510; } // up to 510 characters
        rprintf("frame = %d\n\r",frame);
      }
      else if(ind == 7)
      {
        ad1_3 = stringBuf[mark-2]; // default is 'N'
        if(ad1_3 == 'Y'){ temp2++; }
        rprintf("ad1_3 = %c\n\r",ad1_3);
      }
      else if(ind == 8)
      {
        ad0_3 = stringBuf[mark-2]; // default is 'N'
        if(ad0_3 == 'Y'){ temp2++; }
        rprintf("ad0_3 = %c\n\r",ad0_3);
      }
      else if(ind == 9)
      {
        ad0_2 = stringBuf[mark-2]; // default is 'N'
        if(ad0_2 == 'Y'){ temp2++; }
        rprintf("ad0_2 = %c\n\r",ad0_2);
      }
      else if(ind == 10)
      {
        ad0_1 = stringBuf[mark-2]; // default is 'N'
        if(ad0_1 == 'Y'){ temp2++; }
        rprintf("ad0_1 = %c\n\r",ad0_1);
      }
      else if(ind == 11)
      {
        ad1_2 = stringBuf[mark-2]; // default is 'N'
        if(ad1_2 == 'Y'){ temp2++; }
        rprintf("ad1_2 = %c\n\r",ad1_2);
      }
      else if(ind == 12)
      {
        ad0_4 = stringBuf[mark-2]; // default is 'N'
        if(ad0_4 == 'Y'){ temp2++; }
        rprintf("ad0_4 = %c\n\r",ad0_4);
      }
      else if(ind == 13)
      {
        ad1_7 = stringBuf[mark-2]; // default is 'N'
        if(ad1_7 == 'Y'){ temp2++; }
        rprintf("ad1_7 = %c\n\r",ad1_7);
      }
      else if(ind == 14)
      {
        ad1_6 = stringBuf[mark-2]; // default is 'N'
        if(ad1_6 == 'Y'){ temp2++; }
        rprintf("ad1_6 = %c\n\r",ad1_6);
      }
      else if(ind == 15)
      {
        safety = stringBuf[mark-2]; // default is 'Y'
        rprintf("safety = %c\n\r",safety);
      }
    }
  }

  if(safety == 'Y')
  {
    if((temp2 ==10) && (freq > 150)){ freq = 150; }
    else if((temp2 == 9) && (freq > 166)){ freq = 166; }
    else if((temp2 == 8) && (freq > 187)){ freq = 187; }
    else if((temp2 == 7) && (freq > 214)){ freq = 214; }
    else if((temp2 == 6) && (freq > 250)){ freq = 250; }
    else if((temp2 == 5) && (freq > 300)){ freq = 300; }
    else if((temp2 == 4) && (freq > 375)){ freq = 375; }
    else if((temp2 == 3) && (freq > 500)){ freq = 500; }
    else if((temp2 == 2) && (freq > 750)){ freq = 750; }
    else if((temp2 == 1) && (freq > 1500)){ freq = 1500; }
    else if((temp2 == 0)){ freq = 100; }
  }
  
  if(safety == 'T'){ test(); }

}


void mode_0(void) // Auto UART mode
{
  rprintf("MODE 0\n\r");
  setup_uart0(baud,1);
  stringSize = BUF_SIZE;
  mode_action();
  //rprintf("Exit mode 0\n\r");
}

void mode_1(void)
{
  rprintf("MODE 1\n\r");  

  setup_uart0(baud,2);
  stringSize = frame + 2;

  mode_action();
}

void mode_2(void)
{
  rprintf("MODE 2\n\r");  
  enableIRQ();
  // Timer0  interrupt is an IRQ interrupt
  VICIntSelect &= ~0x00000010;
  // Enable Timer0 interrupt
  VICIntEnable |= 0x00000010;
  // Use slot 2 for UART0 interrupt
  VICVectCntl2 = 0x24;
  // Set the address of ISR for slot 1
  VICVectAddr2 = (unsigned int)MODE2ISR;

  T0TCR = 0x00000002; // Reset counter and prescaler
  T0MCR = 0x00000003; // On match reset the counter and generate interrupt
  T0MR0 = 58982400 / freq;

  T0PR = 0x00000000;

  T0TCR = 0x00000001; // enable timer

  stringSize = BUF_SIZE;
  mode_action();
}

void mode_action(void)
{
  int j;
  while(1)
  {
    
    if(log_array1 == 1)
    {
      stat(0,ON);
        
      if(fat_write_file(handle,(unsigned char *)RX_array1, stringSize) < 0)
      {
        while(1)
        {
          stat(0,ON);
          for(j = 0; j < 500000; j++);
          stat(0,OFF);
          stat(1,ON);
          for(j = 0; j < 500000; j++);
          stat(1,OFF);
        }
      }
      
      sd_raw_sync();
      stat(0,OFF);
      log_array1 = 0;
    }

    if(log_array2 == 1)
    {
      stat(1,ON);
      
      if(fat_write_file(handle,(unsigned char *)RX_array2, stringSize) < 0)
      {
        while(1)
        {
          stat(0,ON);
          for(j = 0; j < 500000; j++);
          stat(0,OFF);
          stat(1,ON);
          for(j = 0; j < 500000; j++);
          stat(1,OFF);
        }
      }
      
      sd_raw_sync();
      stat(1,OFF);
      log_array2 = 0;
    }

    if((IOPIN0 & 0x00000008) == 0) // if button pushed, log file & quit
    {
      VICIntEnClr = 0xFFFFFFFF;

      if(RX_in < BUF_SIZE)
      {
        fat_write_file(handle, (unsigned char *)RX_array1, RX_in);
        sd_raw_sync();
      }
      else if(RX_in >= BUF_SIZE)
      {
        fat_write_file(handle, (unsigned char *)RX_array2, RX_in - BUF_SIZE);
        sd_raw_sync();
      }
      while(1)
      {
        stat(0,ON);
        for(j = 0; j < 500000; j++);
        stat(0,OFF);
        stat(1,ON);
        for(j = 0; j < 500000; j++);
        stat(1,OFF);
      }
    }
  }
}

void test(void)
{

  rprintf("\n\rLogomatic V2 Test Code:\n\r");
  rprintf("ADC Test will begin in 5 seconds, hit stop button to terminate the test.\r\n\n");

  delay_ms(5000);

  while((IOPIN0 & 0x00000008) == 0x00000008)
  {
    // Get AD1.3
    AD1CR = 0x0020FF08;
    AD_conversion(1);

    // Get AD0.3
    AD0CR = 0x0020FF08;
    AD_conversion(0);
    
    // Get AD0.2
    AD0CR = 0x0020FF04;
    AD_conversion(0);

    // Get AD0.1
    AD0CR = 0x0020FF02;
    AD_conversion(0);

    // Get AD1.2
    AD1CR = 0x0020FF04;
    AD_conversion(1);
    
    // Get AD0.4
    AD0CR = 0x0020FF10;
    AD_conversion(0);

    // Get AD1.7
    AD1CR = 0x0020FF80;
    AD_conversion(1);

    // Get AD1.6
    AD1CR = 0x0020FF40;
    AD_conversion(1);

    delay_ms(1000);
    rprintf("\n\r");
  }

  rprintf("\n\rTest complete, locking up...\n\r");
  while(1);
    
}

void AD_conversion(int regbank)
{
  int temp = 0, temp2;

  if(!regbank) // bank 0
  {
    AD0CR |= 0x01000000; // start conversion
    while((temp & 0x80000000) == 0)
    {
      temp = AD0DR;
    }
    temp &= 0x0000FFC0;
    temp2 = temp / 0x00000040;

    AD0CR = 0x00000000;
  }
  else      // bank 1
  {
    AD1CR |= 0x01000000; // start conversion
    while((temp & 0x80000000) == 0)
    {
      temp = AD1DR;
    }
    temp &= 0x0000FFC0;
    temp2 = temp / 0x00000040;

    AD1CR = 0x00000000;
  }

  rprintf("%d", temp2);
  rprintf("   ");
  
}

void fat_initialize(void)
{
  if(!sd_raw_init())
  {
    rprintf("SD Init Error\n\r");
    while(1);
  }

  if(openroot())
  { 
    rprintf("SD OpenRoot Error\n\r");
  }
}

void delay_ms(int count)
{
  int i;
  count *= 10000;
  for(i = 0; i < count; i++)
    asm volatile ("nop");
}
