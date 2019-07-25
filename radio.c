#include "radio.h"



BOOL IsSqlActive ()
{
   int n;

   for (n=0;n<15;n++)
      if (SQL)
         return FALSE;

   return TRUE;        
}

void SetRadioTX (BOOL tx)
{
   if (tx==TRUE)
   {
      TxMode ();
      delay_ms (255);
      delay_ms (255);      //Tiempo de espera para transmisión
   }

   if (tx==FALSE)
   {
      delay_ms (255);
      delay_ms (255);      //Tiempo de espera para recepción
      RxMode ();
   }
}

void LoadEnable (void)
{
   NOP();
   NOP();
   NOP();
   NOP();
   NOP();
   PORTB.LE = 1;
   NOP();
   NOP();
   NOP();
   NOP();
   NOP();
   PORTB.LE = 0;
   NOP();
   NOP();
   NOP();
   NOP();
   NOP();
}


void RxMode(void)
{
   unsigned long r = 160;	//Programación para RX
   unsigned long n = 610;		//609;	
   unsigned int a = 23;		//77;		

   RECEPCION;		//Nos ponemos en recepción

   r<<=1;
   r|=R_COUNTER|PRESCALER_64;

   a<<=1;
   SendWordSPI ((unsigned long)r);
   LoadEnable();
   NOP();
   NOP();
   NOP();
   NOP();
   NOP();
   NOP();
   SendWordSPI ((unsigned long)n);
   SendByteSPI ((unsigned char)a);
   LoadEnable();
}

void TxMode(void)
{
   unsigned long r = 160;	//Programación para TX
   unsigned long n = 550;	//549;
   unsigned int a = 0;		//56;

   TRANSMISION;		//Nos ponemos en transmisión

   r<<=1;
   r|=R_COUNTER|PRESCALER_64;

   a<<=1;
   SendWordSPI ((unsigned long)r);
   LoadEnable();
   NOP();
   NOP();
   NOP();
   NOP();
   NOP();
   NOP();
   SendWordSPI ((unsigned long)n);
   SendByteSPI ((unsigned char)a);
   LoadEnable();
}

void test (char ch)
{
   SEI();
   TxMode ();

   while (TRUE)
      sci_outb (ch);     //Generamos 20 bytes de sincronismo

}
