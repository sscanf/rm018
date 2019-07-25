/////////////////////////////////////////////////////////////////////////
//Radiomódem para EPEL
//Éste es el módem que va en el vagón.
//
//
// Control de modificaciones:
//
// rev.1
// (Creación)
//
// rev.2
// - Se ha alargado el pito de sincronismo cuando se intenta el enlace.
//
///////////////////////////////////////////////////////////////////////////////////

#define RELEASE
//#define DEBUG

#ifdef RELEASE
#define OSC 2000
#endif

#ifdef DEBUG
#define OSC 4000
#endif

#include <y:\c6805\headers\705c8a.h>
#include <y:\c6805\headers\CTYPE.h>
#include <y:\c6805\examples\delay.h>
#include "rm018.h"
#include "sio.h"
#include "radio.h"

#define UART   0;
#define RADIO  1;

//****************** REGISTROS DE CONFIGURACIÓN DE LA CPU *******************


//****************** OPCIONES PARA EL COMPILADOR ****************************

#pragma option s3;
#pragma option v;

#ifdef RELEASE
#pragma mor @ 0x1ff0 = 0;
#pragma mor @ 0x1ff1 = 0;
#pragma portrw mor1_ @ 0x1ff0;
#pragma portrw mor2_ @ 0x1ff1;
#endif

#pragma portrw opt_ @ 0x1fdf;
#pragma mori @ 0x1fdf = RAM0*RAM0_STATUS | RAM1*RAM1_STATUS;

//////////////////// PROTOTIPOS /////////////////////////////////////////////

WORD crc16 ();
void sci_write (char *buffer);
void sci_outb (char byte);
void sci_outw (WORD byte);
int  DoDTEMsg(void);
void SendDteACK(int type);
void enlaza ();
void TxRadio (void);
void TxDte (void);
void CambioIndic ();
BOOL cmp_indic (void);
void SendADREQ(int vagon);
void SendRadioMsg(int type);      //Enviamos un mensaje por radio
void GetMsg (unsigned char byte);

///////////////////////////// RAM PUBLICA ///////////////////////////////////////

struct _eeprom
{
   int iAddres;
   long lData;
}eeprom;

BOOL bRxMsg;


char msg[LEN_MSG] @ 0x100;
char indic[12];   //Indicativo

int iIndex;

//////////////////////////////// ROM ///////////////////////////////////////////
//
// Rutina de recepción de la UART 16C450
//
void RxUart ()
{
   long byte;
   static char ant;
   int bStart=FALSE;

   SEI();      //Inhibimos las interrupciones.
   
   if (bRxMsg==TRUE)
   {
      byte=sio_getch();
      goto _fin;
   }

   while (bRxMsg==FALSE)
   {
      byte=sio_getch();
      if (byte==-1)
         goto _fin;
     
      if (bStart==TRUE)
      {
         msg[iIndex]=(char)byte;
         iIndex++;
      }
      
      //Cada vez que recibe un 0xeca1 resetea el contador del buffer
      if (byte==0xa1 && ant==0xec)
      {        
         bStart=TRUE;
         iIndex=0;
      }
  
      if (byte==EOF && bStart==TRUE)        //Final de mensaje?
      {
         bRxMsg=TRUE;
         bStart=FALSE;
         iIndex-=2;
         goto _fin;
      }
   
      if (iIndex>LEN_MSG)         //Impedimos el desbordamiento del buffer
         iIndex=LEN_MSG;
   
       ant = byte;
   }

_fin:
   CLI();
}

void RxRadio (unsigned char byte)
{
   GetMsg (byte);
}

void GetMsg (unsigned char byte)
{
   static BOOL bStart=FALSE;
   static BOOL bCtrlStart;
   static char ant;

   //Mientras no se esté procesando ningún mensaje.

   if (bRxMsg==FALSE)   
   {
      if (bStart==TRUE)
      {
         msg[iIndex]=byte;
         iIndex++;
      }

      //Cada vez que recibe un 0xeca1 resetea el contador del buffer
      if (byte==0xa1 && ant==0xec)
      {        
         bStart=TRUE;
         iIndex=0;
      }
  
      if (byte==EOF && bStart==TRUE)        //Final de mensaje?
      {
         bRxMsg=TRUE;
         bStart=FALSE;
         iIndex-=2;
      }
   
      if (iIndex>LEN_MSG)         //Impedimos el desbordamiento del buffer
         iIndex=LEN_MSG;
    }
    ant = byte;
}

void main (void)
{
   int iMsg;

   iIndex=0;
   bRxMsg=FALSE;

   sio_open ();
   sio_cnt_irq ((long)RxUart);  //Definimos nuestra rutina de interrupción para
                                //la recepción de la UART.
   sio_lctrl (C_RTS);

   ECON_ON;                     //Activamos el economizador
   CLI();
   while (TRUE)
   {
      SCCR2.RIE  = IsSqlActive();     //Receive interrupt

      if (bRxMsg)
      {
         iMsg=DoDTEMsg();
         if (iMsg)
         {
            // Ejecutamos el mensaje.
            SEI();
            switch (msg[12]) //Miramos de que mensaje se trata
            {
               case MSG_ACK:
               case MSG_NACK:
               case MSG_TXD:
                     TxDte();
               break;

               case MSG_TEST:
                  test ((char)msg[13]);
               break;
               case MSG_ACT:
                  enlaza();
                break;

               default:
                 TxRadio();
               break;     
            }
           iIndex=0;
           CLI();
         }
      }    
   }
}

int DoDTEMsg(void)
{
   WORD crc,crc2;
   int tmp,tmp1;
   int crc_l;
   int crc_h;
   int n;

   SEI();
   //Primero convertimos el CRC16 en HEX ya que nos lo envían en Cutre-ASCII

   tmp = msg[iIndex]-0x30;       //SemiByte bajo CRC  (byte bajo)
   tmp1= msg[iIndex-1]-0x30;     //SemiByte alto CRC (byte bajo)
   crc_l = MAKEBYTE (tmp1,tmp);  //Componemos el byte

   tmp = msg[iIndex-2]-0x30;     //SemiByte bajo CRC (Byte alto)
   tmp1= msg[iIndex-3]-0x30;     //SemiByte alto CRC (Byte alto)
   crc_h = MAKEBYTE (tmp1,tmp);  //Componemos el byte
   crc = MAKEWORD (crc_h,crc_l); //Componemos un WORD con los dos bytes

   crc2 = (WORD) crc16 ();       //Calculamos el CRC

   if (crc2!=crc)  
      goto fin;   //El CRC es incorrecto 
      
   bRxMsg = FALSE;   //Quitamos flag de mensaje recibido.
   sio_lctrl (C_RTS);
   CLI();
   return msg[12];

fin:  
   bRxMsg = FALSE;   //Quitamos flag de mensaje recibido.
   sio_lctrl (C_RTS);
   CLI();
   return FALSE;

}

void enlaza()
{
   unsigned int n,i;
   
   SEI();
   SetRadioTX (TRUE);

   for (i=0;i<17;i++)
      for (n=0;n<200;n++)
         sci_outb (0xff);     //Generamos 400 bytes de sincronismo

   for (n=0;n<10;n++)
         sci_outb (0xaa);     //Generamos 400 bytes de sincronismo

   sci_outw ((WORD)FRAME);//Enviamos el Frame
   for (n=0;n<iIndex+1;n++)
      sci_outb (msg[n]);  //Datos del mensaje a transmitir

   sci_outb (EOF);   //Enviamos el EOF
   delay_ms(10);
   SetRadioTX (FALSE);
   CLI();
}

void SendDteACK(int type)      //Enviamos un ACK al DTE
{
   int n;
   long crc;
   BYTE tmp,b;

   msg[12]=type;

   iIndex=16;
   crc = crc16();    //Calculamos el CRC

/////////////////////////

	msg[13]=HIBYTE(crc);
	b = HINIBBLE (msg[13]);
   b+=0x30;
	tmp=b;

	msg[14]=HIBYTE(crc);
	b=LONIBBLE (msg[14]);
	b+=0x30;
	msg[14]=b;
	msg[13]=tmp;

//////////////////////////

	msg[15]=LOBYTE(crc);
	b = HINIBBLE (msg[15]);
	b+=0x30;
	tmp=b;

	msg[16]=LOBYTE(crc);
	b=LONIBBLE (msg[16]);
	b+=0x30;
	msg[16]=b;
	msg[15]=tmp;
   msg[17]=EOF;

   sio_out (HIBYTE(FRAME));      //Enviamos el Frame
   sio_out (LOBYTE(FRAME));      //Enviamos el Frame
   for (n=0;n<18;n++)
      sio_out (msg[n]);  //Datos del mensaje a transmitir
}


void SendRadioMsg(int type)      //Enviamos un mensaje por radio
{
   int n;
   long crc;
   BYTE tmp,b;

   msg[12]=type;

   iIndex=16;
   crc = crc16();    //Calculamos el CRC

/////////////////////////

	msg[13]=HIBYTE(crc);
	b = HINIBBLE (msg[13]);
   b+=0x30;
	tmp=b;

	msg[14]=HIBYTE(crc);
	b=LONIBBLE (msg[14]);
	b+=0x30;
	msg[14]=b;
	msg[13]=tmp;

//////////////////////////

	msg[15]=LOBYTE(crc);
	b = HINIBBLE (msg[15]);
	b+=0x30;
	tmp=b;

	msg[16]=LOBYTE(crc);
	b=LONIBBLE (msg[16]);
	b+=0x30;
	msg[16]=b;
	msg[15]=tmp;
   msg[17]=EOF;

   SetRadioTX (TRUE);
   sci_outw (FRAME);      //Enviamos el Frame
   for (n=0;n<18;n++)
      sci_outb (msg[n]);  //Datos del mensaje a transmitir
   
   delay_ms (10);
   SetRadioTX (FALSE);
}

void SendADREQ (int vagon)
{
   vagon+=0x30;    //Cómo el máximo de vagones es 8 vagones, sólo cambiamos las unidades.

   sci_outb ('A');
   sci_outb ('D');
   sci_outb ('0');
   sci_outb (vagon);
   sci_outb (0x0d);
   sci_outb (0x0a);
}
void TxRadio ()
{
   int n;
   
   while (!SQL);     //Esperamos que la línea de SQL suba.
   SEI();
   SetRadioTX (TRUE);

   for (n=0;n<20;n++)
      sci_outb (0xff);     //Generamos 20 bytes de sincronismo

   for (n=0;n<10;n++)
         sci_outb (0xaa);     //Generamos 400 bytes de sincronismo

   sci_outw ((WORD)FRAME);//Enviamos el Frame
   for (n=0;n<iIndex+1;n++)
      sci_outb (msg[n]);  //Datos del mensaje a transmitir

   sci_outb (EOF);   //Enviamos el EOF
   SetRadioTX (FALSE);
   CLI();
}

void TxDte (void)
{
   int n;
   sio_out (HIBYTE(FRAME));      //Enviamos el Frame
   sio_out (LOBYTE(FRAME));      //Enviamos el Frame
   for (n=0;n<iIndex+1;n++)
      sio_out (msg[n]);  //Datos del mensaje a transmitir

   sio_out (EOF);   //Enviamos el EOF
}

////////////////////////// COMUNICACIONES ///////////////////////////////////


void sci_write (char *buffer)
{
    char __ps;
    while(__ps = *buffer++)
        sci_outb(__ps);
}

void sci_outb (char byte)
{
    int tmp = SCSR;
    SCDR=byte;
    while (SCSR.TDRE==FALSE);   //Esperamos que el buffer de tx esté libre
}

void sci_outw (WORD byte)
{
    int tmp = SCSR;
    SCDR=HIBYTE (byte);
    while (SCSR.TDRE==FALSE);   //Esperamos que el buffer de tx esté libre

    SCDR=LOBYTE (byte);
    while (SCSR.TDRE==FALSE);   //Esperamos que el buffer de tx esté libre
}

WORD crc16 ()
{
   WORD res=0xffff;
   char tmp;

   if (iIndex<5)
      return  -1; //El mensaje no es válido

    //Calculamos el CRC con los datos del mensaje.
   for (int n=0;n<iIndex-3;n++)
   {
      res^=msg[n];
      for (int bit=0;bit<8;bit++)
      {
         tmp=res&1;
         if (tmp)
         {
            res=res>>1;
            res^=0xa001;
         }
         else
            res=res>>1;
      }
   }
  return res;
}

int SendByteSPI (unsigned char byte)
{
   SPDR = byte;
   while (!SPSR.7);     //Transmite el byte
   return FALSE;
}

int SendWordSPI (unsigned long byte)
{
   SPDR = HIBYTE(byte);
   while (!SPSR.7);     //Transmite el byte

   SPDR = LOBYTE(byte);
   while (!SPSR.7);     //Transmite el byte

   return FALSE;
}

void __STARTUP(void)
{
   SEI();

#ifdef RELEASE
// MASK OPTION REGISTER
   mor1_ = 0;
   mor2_ = 0;
#endif

// Configuración de los puertos
   DDRA = 0b00000000;
   DDRB = 0b11111011;
   DDRC = 0b00000000;


    //Configuración SPI
   SPCR.SPIE  = FALSE;
   SPCR.SPE   = TRUE;
   SPCR.MSTR  = TRUE;
   SPCR.CPOL  = FALSE;
   SPCR.CPHA  = FALSE;
   SPCR.SPR0  = 1; //Clock dividido por 32
   SPCR.SPR1  = 1;

    //Configuración SCI
   SCCR1.M    = FALSE;   //8 bits de datos, 1 stop
   SCCR1.WAKE = FALSE;

   SCCR2.TIE  = FALSE;  //Transmit interrupt
   SCCR2.TCIE = FALSE;  //Transmit complete interrupt
   SCCR2.RIE  = FALSE;   //Receive interrupt
   SCCR2.ILIE = FALSE;  //Idle Line interrupt
   SCCR2.TE   = TRUE;   //Transmit Enable
   SCCR2.RE   = TRUE;   //Receive Enable
   SCCR2.RWU  = FALSE;  //Receive Wake-Up
   SCCR2.SBK  = FALSE;  //Send Break


   BAUD.SCP1=      1;   //19200
   BAUD.SCP0=      0;
   BAUD.SCR2=      0;
   BAUD.SCR1=      0;
   BAUD.SCR0=      0;

/*
#ifdef DEBUG      //Programaci¢n para emulador
   BAUD.SCP1=      1;   //9600 bauds
   BAUD.SCP0=      1;
   BAUD.SCR2=      0;
   BAUD.SCR1=      0;
   BAUD.SCR0=      0;
#endif
*/
   // Configuración del Watch Dog
   COPCR.COPF = FALSE;
   COPCR.CME  = FALSE;
   COPCR.COPE = FALSE;
   COPCR.CM1  = 0;
   COPCR.CM0  = 0;
}

///////////////////////////////////////////////////////////////////////

// ----------------------- RUTINAS INTERRUPCION --------------------------

void __SCI (void)
{
   int tmp = SCSR;
   tmp=SCDR;
   GetMsg (tmp);
}

void __IRQ(void)
{  
   if (WAKEUP) //Si no está durmiendo
      UartISR();
}

#include "sio.c"
#include "radio.c"