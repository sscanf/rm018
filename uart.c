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


//****************** REGISTROS DE CONFIGURACIÓN DE LA CPU *******************
//****************** OPCIONES PARA EL COMPILADOR ****************************

#ifdef RELEASE
#pragma mor @ 0x1ff0 = 0;
#pragma mor @ 0x1ff1 = 0;
#pragma portrw mor1_ @ 0x1ff0;
#pragma portrw mor2_ @ 0x1ff1;
#endif

#pragma portrw opt_ @ 0x1fdf;

//////////////////// DEFINICIONES  //////////////////////////////////////////

#define FRAME   0xeca1
#define EOF     13 //Campo de final de mensaje para el DTE
#define LEN_MSG 120 //Largura máxima de los mensajes del DTE

#define ENTRADAS	0b00000000
#define SALIDAS	0b11111111

//////////////////// PROTOTIPOS /////////////////////////////////////////////


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
     opt_ = RAM0*RAM0_STATUS | RAM1*RAM1_STATUS;

// Configuración de los puertos
   DDRA = 0b11111110;
   DDRB = 0b00001111;
   DDRC = 0b00000000;

    //Configuración SPI
   SPCR.SPIE  = FALSE;
   SPCR.SPE   = FALSE;
   SPCR.MSTR  = FALSE;
   SPCR.CPOL  = FALSE;
   SPCR.CPHA  = FALSE;
   SPCR.SPR0  = 1; //Clock dividido por 32
   SPCR.SPR1  = 1;

    //Configuración SCI
   SCCR1.M    = FALSE;   //8 bits de datos, 1 stop
   SCCR1.WAKE = FALSE;

   SCCR2.TIE  = FALSE;  //Transmit interrupt
   SCCR2.TCIE = FALSE;  //Transmit complete interrupt
   SCCR2.RIE  = FALSE;  //Receive interrupt
   SCCR2.ILIE = FALSE;  //Idle Line interrupt
   SCCR2.TE   = TRUE;   //Transmit Enable
   SCCR2.RE   = TRUE;   //Receive Enable
   SCCR2.RWU  = FALSE;  //Receive Wake-Up
   SCCR2.SBK  = FALSE;  //Send Break

#ifdef RELEASE
   BAUD.SCP0=      0;   //9600 bauds
   BAUD.SCP1=      1;
   BAUD.SCR0=      1;
   BAUD.SCR1=      0;
   BAUD.SCR2=      0;
#endif

#ifdef DEBUG
   BAUD.SCP0=      1;   //9600 bauds
   BAUD.SCP1=      1;
   BAUD.SCR0=      0;
   BAUD.SCR1=      0;
   BAUD.SCR2=      0;
#endif

   // Configuración del Watch Dog
   COPCR.COPF = FALSE;
   COPCR.CME  = FALSE;
   COPCR.COPE = FALSE;
   COPCR.CM1  = 0;
   COPCR.CM0  = 0;
}

struct _flag
{
   BOOL recibido : 1;
   BOOL enviado  : 1;
}flag;

void main (void)
{
	SEI();		                //No queremos interrupciones porque el 
	                            //programa no hace nada más.
   flag.recibido=FALSE;
   flag.enviado=FALSE;
   PORTB.1 = TRUE;
   PORTB.0 = FALSE;

	while (TRUE)
	{
		if (!PORTB.4 && !flag.recibido)//La CPU indica que tiene un nuevo dato 
		{                              //para enviar
   		DDRC=ENTRADAS;	             //El PORTC como entradas
   		NOP();
   		NOP();
         AC=SCSR;
   		SCDR=PORTC;                 //Enviamos el dato por el puerto serie
   	   while (SCSR.TDRE==FALSE);         //Esperamos a que el dato se haya enviado.
   	   
   		PORTB.0 = TRUE;             //Indicamos a la CPU que ya hemos enviado el dato.
   		flag.recibido=TRUE;
		}

		if (PORTB.4 && flag.recibido)
		{
         flag.recibido=FALSE;
         PORTB.0=FALSE;
		}

		if (SCSR.RDRF && !flag.enviado) //Recibimos un byte del SCI
		{
			DDRC=SALIDAS;               //El PORTC como salidas
   		NOP();
   		NOP();
			PORTC=SCDR;
			PORTB.1 = FALSE;            //Indicamos a la CPU que tiene un dato nuevo
			flag.enviado=TRUE;          //Dato enviado.
		}

		if (PORTB.5 && flag.enviado)   //La CPU indica que ya ha cogido el nuevo dato
		{
		   DDRC=ENTRADAS;              //Volvemos a dejar el PORTC como entradas.
		   PORTB.1=TRUE;              //Quitamos señal de nuevo dato.
		   flag.enviado=FALSE;         //Esperamos un nuevo dato.
		}
	}
}