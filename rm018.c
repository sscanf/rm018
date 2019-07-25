/////////////////////////////////////////////////////////////////////////
//Radiomódem para EPEL
//Éste es el módem que va en el vagón.
//
//
// Control de cambios:
//
// rev.1
// (Creación)
//
// rev.2
// - Mientras está despierto, si se cierra el SQL se asegura durante un
//   TimeOut antes de volverse a dormir.

#pragma option s3;
#pragma option v;


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
#include "eeprom.h"

//****************** REGISTROS DE CONFIGURACIÓN DE LA CPU *******************
//****************** OPCIONES PARA EL COMPILADOR ****************************

#ifdef RELEASE
#pragma mor @ 0x1ff0 = 0;
#pragma mor @ 0x1ff1 = 0;
#pragma portrw mor1_ @ 0x1ff0;
#pragma portrw mor2_ @ 0x1ff1;
#endif

//#pragma portrw opt_ @ 0x1fdf;
#pragma mori @ 0x1fdf = RAM0*RAM0_STATUS | RAM1*RAM1_STATUS;

//////////////////// PROTOTIPOS /////////////////////////////////////////////

WORD crc16 ();
void sci_write (char *buffer);
void sci_outb (char byte);
void sci_outw (WORD byte);
BOOL DoRxMsg(void);
void SendDteACK(void);
void ProgramIndic ();
void TxRadio ();
void LeeIndic();
void CambioIndic ();
void MsgACT(BOOL activar);
void SendRadioACK(int type);
void SendDteACK(void);
void MsgTXD(void);
void GetMsg (char __byte);
void DoDTEMsg (void);
int SendWordSPI (unsigned long byte);
int SendByteSPI (unsigned char byte);
void TestEEPROM();
void RxUart ();

///////////////////////////// RAM PUBLICA ///////////////////////////////////////
struct
{
	unsigned long lTiempo;
	BOOL bTimeOut;
}timer;

struct _eeprom
{
   int iAddres;
   long lData;
}eeprom;

char indic[12];   //Indicativo

int iIndex,ms;
BOOL bRxMsg;
BOOL bDTEMsg;
BOOL bMdmMsg;
BOOL bEspera;

char msg[LEN_MSG] @ 0x100;

void RxUart ()
{
   long byte;
   static char ant;
   int bStart=FALSE;
   
   SEI();      //Inhibimos las interrupciones.
   
   if (PORTB.7==FALSE) //Si no está activado, no hace caso.
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
         bDTEMsg=TRUE;
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

void main (void)
{  
   BOOL flg;
	int n,iSql,iTmMult;
	long tiempo;
	RELE_OFF;	//Relé desactivado
	LeeIndic();	//Leemos el indicativo

	SetRadioTX (FALSE);
	flg=PORTC.2;
   SCCR2.RIE  = FALSE;   //Receive interrupt
	
	sio_open();
   sio_cnt_irq ((long)RxUart);  //Definimos nuestra rutina de interrupción para
                                //la recepción de la UART.
   CLI();
  
	while (TRUE)
   {	
      if (!LOCAL)     //Han conectado el conector para programar en indicativo?
      {
         ECON_ON;     //Activamos el economizador
         delay_ms(50);  //Bucle anti-rebotes
         if (!LOCAL)  //Sigue estando activado?
         {
            CLI();
            MsgACT(TRUE);     //Activamos el relé.
            while (!LOCAL)  //Estamos esperando que lo desconecten.
            {
               if (bDTEMsg)
                  DoDTEMsg();
            }
            MsgACT(FALSE);    //Desactivamos el relé.
         }
      }
      else
      {
         CLI();
         ECON_ON;    //Activamos el economizador
         delay_ms (255);
         delay_ms (255);

         iSql=0;     //Cuenta el tiempo que está abierto o cerrado el SQL según
                     //el caso.
         
         SCCR2.RIE  = TRUE;   //Receive interrupt
   		for (ms=0;ms<50 ;ms++ )
   		{
   		   if (!SQL)
   		    iSql++;
   		   else
   		    iSql=0;

   			if (iSql==15)     //Demasiado tiempo con SQL abierto, es portadora?
   			{
   			   iSql=0;
   			   tiempo=3000;
               SCCR2.RIE  = TRUE;   //Receive interrupt

   			   while (tiempo)
   			   {
   			      if (bRxMsg)  //Ha recibido un mensaje de radio
   			      {   			         
   			         iTmMult = 3;
   			         tiempo=60000;  //Si no recibe nada en 3 minutos se
   			                        //desconectará.
   			         while (TRUE)
   			         {
      			         if (bRxMsg)       //Ha llegado algún mensaje de radio
      			         {
         			         if (DoRxMsg()) //Procesa el mensaje recibido
         			            break;
         			         iTmMult = 3;
         			         tiempo=60000;  //Reseteamos el contador
         			      }

         			      if (bDTEMsg)      //Ha llegado algún mensaje del host?
         			      {
         	  		         DoDTEMsg();    //Procesa el mensaje
         	  		         iTmMult = 3;
         			         tiempo=60000;  //Reseteamos el contador
         	  		      }

         			      delay_ms(1);
         	  		      tiempo--;
         	  		       
         	  		      if (tiempo==0)
         	  		      {
              	  		       if (iTmMult==0)
              	  		        break;

              	  		       iTmMult--;
              	  		       tiempo=60000;
              	  		   }
      	  		      }
      	  		   }
      	  		   
      	  		   //Este flag lo pone la __SCI para indicar que no decremente
      	  		   //el tiempo de espera porque podría estar recibiendo una
      	  		   //cabecera de 0xffff o 0xaaaa
      	  		   if (bEspera==FALSE)
      			      tiempo--;

   			      delay_ms(1);
   			      if (SQL) //Sube la líne de SQL
   			         iSql++;
   			      else
   			         iSql=0;

                  if (iSql==15)
   			         break;   //Demasiado tiempo sin SQL
   			   }
               SCCR2.RIE  = FALSE;   //Receive interrupt
   			   break;
   			}
   		}

         MsgACT(FALSE);   //Desactivamos el relé.
   		ECON_OFF;   	  //Desactivamos el economizador
   		STOP();
      }
   }
}

void TestEEPROM()
{
	const long ind[12]={0x3030,0x3030,0x3030,0x3030,0x3030,0x3031};
	long data;
	int n;
   ee_erase_all();

   eeprom.iAddres=0;
   ee_write_enable();
   ee_erase_all();
   for (n=0;n<6 ;n++)
   {
      eeprom.lData=ind[n];
      eeprom.iAddres=n;
      ee_write();
   }
   ee_disable();   return;

   for (n=0;n<0b111111 ;n++ )
   {
      eeprom.iAddres=n;
      data=ee_read ();
   }
}

void DoDTEMsg (void)
{
   WORD crc,crc2;
   int tmp,tmp1;
   int crc_l;
   int crc_h;
   int n;

//Primero convertimos el CRC16 en HEX ya que nos lo envían en Cutre-ASCII

   tmp = msg[iIndex]-0x30;     //SemiByte bajo CRC  (byte bajo)
   tmp1= msg[iIndex-1]-0x30;   //SemiByte altro CRC (byte bajo)
   crc_l = MAKEBYTE (tmp1,tmp);//Componemos el byte

   tmp = msg[iIndex-2]-0x30; //SemiByte bajo CRC (Byte alto)
   tmp1= msg[iIndex-3]-0x30; //SemiByte alto CRC (Byte alto)
   crc_h = MAKEBYTE (tmp1,tmp);  //Componemos el byte
   crc = MAKEWORD (crc_h,crc_l); //Componemos un WORD con los dos bytes

   crc2 = (WORD) crc16 ();     //Calculamos el CRC

   if (crc==crc2) //El CRC es correcto.
   {

      // Ejecutamos el mensaje.

      switch (msg[12]) //Miramos de que mensaje se trata
      {              
         case MSG_ACK:
         case MSG_TXD:
            TxRadio();
         break;

         case MSG_PID:  //Cambio de indicativo
            ProgramIndic();
            SendDteACK();
            SendRadioACK(MSG_ACK);
         break;

         case MSG_TEST:
            test ((char)msg[13]);
         break;


      }//switch
   }//crc
   bDTEMsg = FALSE;   //Quitamos flag de mensaje recibido.
   bRxMsg = FALSE;
}

BOOL DoRxMsg(void)
{
   WORD crc,crc2;
   int tmp,tmp1;
   int crc_l;
   int crc_h;
   int n;

   SCCR2.RIE  = FALSE;   //Receive interrupt

	   //Primero convertimos el CRC16 en HEX ya que nos lo envían en Cutre-ASCII
	   tmp = msg[iIndex]-0x30;   //SemiByte bajo CRC  (byte bajo)
	   tmp1= msg[iIndex-1]-0x30; //SemiByte altro CRC (byte bajo)
	   crc_l = MAKEBYTE (tmp1,tmp);  //Componemos el byte

	   tmp = msg[iIndex-2]-0x30; //SemiByte bajo CRC (Byte alto)
	   tmp1= msg[iIndex-3]-0x30; //SemiByte alto CRC (Byte alto)
	   crc_h = MAKEBYTE (tmp1,tmp);  //Componemos el byte
	   crc = MAKEWORD (crc_h,crc_l); //Componemos un WORD con los dos bytes

	   crc2 = (WORD) crc16 ();     //Calculamos el CRC

	   if (crc==crc2) //El CRC es correcto.
	   {
	      // Comprobamos si el indicativo coincide con el nuestro

	      for (n=0;n<12 ;n++ )
	      {
	         if (msg[n]!=indic[n])
	         {
               bRxMsg = FALSE;   //Quitamos flag de mensaje recibido.
               SCCR2.RIE  = TRUE;   //Receive interrupt
               iIndex=0;
               return TRUE;
	         }
	      }

	      // Ejecutamos el mensaje.

	      switch (msg[12]) //Miramos de que mensaje se trata
	      {
	         case MSG_DES:  //Desactivación
	            delay_ms(100);
	            delay_ms(100);
	            SendRadioACK(MSG_ACK);
	            delay_ms(100);
	            MsgACT(FALSE);   //Desactivamos el relé.
               
               bRxMsg = FALSE;   //Quitamos flag de mensaje recibido.
               iIndex=0;
               SCCR2.RIE  = TRUE;   //Receive interrupt
               return TRUE;

	         break;

	         case MSG_ACT:       //Activación
	            delay_ms(100);
	            delay_ms(100);
	            MsgACT(TRUE);    //Activamos el transmisor
	            delay_ms(100);
	            SendRadioACK(MSG_ACK);
	         break;

	         case MSG_CID:  //Programación de indicativo
	         case MSG_PDTS: //Petición de datos
	            MsgTXD();
	         break;

	      }//switch
	   }//crc
	   bRxMsg = FALSE;   //Quitamos flag de mensaje recibido

   bRxMsg = FALSE;   //Quitamos flag de mensaje recibido.
   SCCR2.RIE  = TRUE;   //Receive interrupt
   iIndex=0;
   return FALSE;
}


void MsgTXD(void)
{
   int n;
   sio_out (HIBYTE(FRAME)); //Enviamos el Frame
   sio_out (LOBYTE(FRAME)); //Enviamos el Frame

   for (n=0;n<iIndex+1;n++)
      sio_out (msg[n]);   //Datos del mensaje a transmitir
   sio_out (EOF);         //Enviamos el EOF  
}


void SendDteACK(void)      //Enviamos un ACK al DTE
{
   int n;
   long crc;
   BYTE tmp,b;

   for (n=0;n<12 ;n++ )
      msg[n]=indic[n];

   msg[12]=MSG_ACK;

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

void SendRadioACK(int type)      //Enviamos un ACK a la radio
{
   int n;
   long crc;
   BYTE tmp,b;

   for (n=0;n<12 ;n++ )
      msg[n]=indic[n];

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
//    msg[17]=EOF;
	TxRadio();
}

void MsgACT(BOOL activar)
{                             
	if (activar== TRUE)
	{
	  RELE_ON;
	}
	else
	  RELE_OFF;

}

void TxRadio ()
{
   int n;

   while (!SQL);     //Esperamos que la línea de SQL suba.
   SetRadioTX (TRUE);

   for (n=0;n<20;n++)
      sci_outb (0xff);     //Generamos 20 bytes de sincronismo

   for (n=0;n<20;n++)
      sci_outb (0xaa);     //Generamos 20 bytes de sincronismo

	int n;
	sci_outw ((WORD)FRAME);//Enviamos el Frame

	for (n=0;n<iIndex+1;n++)
	  sci_outb (msg[n]);  //Datos del mensaje a transmitir

	sci_outb (EOF);   //Enviamos el EOF
   SetRadioTX (FALSE);
}

void ProgramIndic ()
{
	//   Programa el indicativo en la eeprom
	int n,hByte,lByte;
	int id=13;

	//   if (iIndex<27)
	//      return;

	delay_ms (10);

	eeprom.iAddres=0;
	ee_write_enable();
	ee_erase_all();

	for (n=0;n<6;n++)
	{
		hByte = msg[id];
		indic[id-13]=msg[id];
		id++;
		lByte = msg[id];
		indic[id-13]=msg[id];
		id++;

		eeprom.lData = MAKEWORD(hByte,lByte);
		eeprom.iAddres=n;
		ee_write();
	}
	ee_disable();
}

void LeeIndic()
{
   int n,iIndex=0;
   long byte;

   delay_ms(10);
 
   for (n=0;n<6;n++)
   {
      eeprom.iAddres=n;
      byte = ee_read ();
      indic[iIndex]= HIBYTE (byte);
      iIndex++;
      indic[iIndex] = LOBYTE (byte);
      iIndex++;
   }
}

////////////////////////// COMUNICACIONES ///////////////////////////////////

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

void __STARTUP(void)
{
   SEI();

#ifdef RELEASE
// MASK OPTION REGISTER
   mor1_ = 0;
   mor2_ = 0;
#endif

// OPTION REGISTER
//   opt_ = 0xc0;
//     opt_ = RAM0*RAM0_STATUS | RAM1*RAM1_STATUS;

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


   BAUD.SCP1=      1;   //19200 bauds
   BAUD.SCP0=      0;
   BAUD.SCR2=      0;
   BAUD.SCR1=      0;
   BAUD.SCR0=      0;

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
	registera __ac;
	
	__ac= SCSR;
   GetMsg (SCDR);
}

void GetMsg (char __byte)
{
   static BOOL bStart=FALSE;
   static char ant;
   
   if (bRxMsg ==FALSE)      //Mientras no se esté procesando ningún mensaje.
   {
      if (bStart==TRUE)
      {
         msg[iIndex]=__byte;
         iIndex+=1;
      }

      //Si no está en medio de un mensaje y recibe un 0xffff o un 0xaaaa 
      //pone flag de espera para indicar a main que no se ponga en modo
      //STOP, porque podría ser una cabecera de mensaje.

      if (((__byte==0xff && ant==0xff) || (__byte==0xaa && ant==0xaa)) && !bStart)
         bEspera=TRUE;
      else
         bEspera=FALSE;

      //Cada vez que recibe un 0xeca1 resetea el contador del buffer
      if (__byte==0xa1 && ant==0xec)
      {
         bStart=TRUE;
         iIndex=0;
      }
  
      if (__byte==EOF && bStart==TRUE)        //Final de mensaje?
      {
         bRxMsg=TRUE;    //Se ha completado la recepción de un mensaje.
         bStart=FALSE;
         iIndex-=2;
      }
   
      if (iIndex>LEN_MSG)         //Impedimos el desbordamiento del buffer
         iIndex=LEN_MSG;
    }
    ant = __byte;
}

void __TIMER (void)
{
   timer.lTiempo--;

   if (!timer.lTiempo)
      timer.bTimeOut=TRUE;

	#asm
        lda TSR
        lda TCNTH
        lda TCNTL
   #endasm	
}

void __IRQ (void)
{
   if (WAKEUP)
      UartISR();           //interrupciones de la UART.
}

#include "sio.c"
#include "radio.c"
#include "eeprom.c"
