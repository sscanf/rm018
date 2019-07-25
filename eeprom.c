#include "eeprom.h"

void ee_write(void)
{
   unsigned long instr;
   PORTB.6 = TRUE;
   instr= (unsigned long)WRITE;
   instr+=(unsigned int)eeprom.iAddres;

   send_data(instr);
   send_data(eeprom.lData);
   PORTB.6 = FALSE;
   t_delay(1000);
}

long ee_read()
/* returns byte at location adr */
{
   unsigned long d,instr;
   PORTB.6 = TRUE;
   instr= (unsigned long)READ;
   instr+=(unsigned int)eeprom.iAddres;
   send_data(instr);
   d = get_data();
   PORTB.6 = FALSE;
   t_delay(1000);
   return(d);
}

void ee_write_enable(void)
/* enable eeprom for writes and erases */
{
   PORTB.6 = TRUE;
   send_data((unsigned long)EWEN);
   PORTB.6 = FALSE;
   t_delay(1000);
}

void ee_erase_all(void)
/* erases all bytes */
{
   PORTB.6 = TRUE;
   send_data((unsigned long)ERAL);
   PORTB.6 = FALSE;
   t_delay(1000);
}

void ee_write_all()
/* writes specified byte d to all locations */
{
   PORTB.6 = TRUE;
   send_data((unsigned long)WRAL);
   send_data(eeprom.lData);
   PORTB.6 = FALSE;
   t_delay(1000);
}

void ee_erase()
/* earses specified location */
{
   unsigned long instr;
   PORTB.6 = TRUE;
   instr=ERASE;
   instr+=(unsigned int)eeprom.iAddres;
   send_data(instr);
   PORTB.6 = FALSE;
   t_delay(1000);
}

void ee_disable(void)
/* disable write and erase */
{
   PORTB.6 = TRUE;
   send_data((unsigned long)EWDS);
   PORTB.6 = FALSE;
   t_delay(1000);
}

void send_data(unsigned long d)
{
   SPDR = HIBYTE(d);
   while (!SPSR.7);                        //Transmite el byte

   SPDR = LOBYTE(d);
   while (!SPSR.7);                        //Transmite el byte
}

long get_data(void)
{
   int hByte, lByte;

   SPCR.CPHA  = TRUE;

   SPDR = 0;
   while (!SPSR.7);                        //Transmite el byte  
   hByte = SPDR;

   SPDR = 0;
   while (!SPSR.7);                        //Transmite el byte
   lByte = SPDR;

   SPCR.CPHA  = FALSE;
   
   return(MAKEWORD (hByte, lByte));
}

void t_delay(unsigned long t)
/* for timing between clock pulses and CS 0 pulses */
{
   unsigned long n;
   for(n=0; n<t; n++);  /* just loop */  ;
}


