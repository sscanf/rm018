/************************************************
*                    C128-DRV                   *
*                    --------                   *
*       Driver controlador placas MOXA C-C128.  *
*                                               *
*       Copyright(c) Electrónica Barcelona S.L. *
*   Escrito por Oscar Casamitjana Vazquez       *
*                                               *
***********************************************/


#include <y:\c6805\headers\705c8a.h>
#include <y:\c6805\headers\CTYPE.h>
#include "sio.h"

#define MAX_LEN  50
#define CERRADO  0
#define ABIERTO  1
#define NUMPORTS 4

#define RXreg  0
#define TXreg  0
#define IERreg 1  // Interrupt Enable
#define IIDreg 2        // Interrupt Identification
#define FCRreg 2        // Fifo control write
#define CRreg  3        // Line Control
#define MCRreg 4        // Modem Control
#define LSRreg 5        // Line Status
#define MSRreg 6        // Modem Status
#define SCRreg 7  // Scratch register
#define LObaud 0        // Divisor Latch (LSB)
#define HIbaud 1        // Divisor Latch (MSB)
#define RDreg  (-1)

#define CERRADO  0
#define ABIERTO  1
#define OFF      0
#define TRUE     1
#define FALSE    0

#define ENTRADAS     0x00;
#define SALIDAS      0xff;
#define TIME_OUT     2000;

void (*IntRXD)();

void sio_open ()
{
   PORTB.0 = FALSE;
   PORTB.1 = TRUE;
}

long inp (int port)
{
	char byte;

   DDRA = ENTRADAS;
	byte = PORTA;
	PORTB.0  = TRUE;		//Indicamos a la SIO que ya hemos cogido el dato
	NOP();
	NOP();
	NOP();
	NOP();

   while (!CC.L);
	PORTB.0  = FALSE;
	return byte;
}

void outp (int port, char byte)
{
   DDRA  = SALIDAS;  //Ponemos el puerto de datos como salidas
   PORTA = byte;
   PORTB.1=FALSE;		//Indicamos a la SIO que tenemos un datoa para enviar
   while (PORTC.2==FALSE);	//Esperamos que la SIO envíe el dato
   PORTB.1=TRUE;     
}

int sio_cnt_irq (far *fun())
{
   IntRXD= fun;
	return (0);
}


long sio_getch()
{  
   long TimeOut=0;

   while (CC.L==TRUE && TimeOut<2000)
      TimeOut++;

   if (TimeOut==2000)   //Si no ha llegado ningún byte en TimeOut ms
      return -1;     //retorna con un error.

	return (inp (RXreg));
}

int sio_lctrl (int mode)
{
	return (0);
}


void sio_write (char *buff)
{
   char __ps;

   while (__ps = *buff ++)
      sio_out (__ps);
}

void sio_out(char byte)
{
	outp (TXreg,byte);
}

void sio_reset(void)
{
}

void UartISR ()
{
	int lcr;
	int byte,byte1;

   (*IntRXD)();
}
