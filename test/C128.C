/**********************************************
*	             C128-DRV					  *
*				 --------					  *
*	Driver controlador placas MOXA C-C128.	  *
*											  *
*	Copyright(c) Electr�nica Barcelona S.L.	  *
*   Escrito por Oscar Casamitjana Vazquez	  *
*											  *
***********************************************/


#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <process.h>
#include <string.h>
#include <mem.h>
#include "c128.h"

//#define MOXA
#define STANDARD
#define BASE   0x2F8
#define PLACA1 13
#define PLACA2 11

#define MAX_LEN  512
#define CERRADO  0
#define ABIERTO  1
#define NUMPORTS 4


#define RXreg  0
#define TXreg  0
#define IERreg 1  // Interrupt Enable
#define IIDreg 2	// Interrupt Identification
#define FCRreg 2	// Fifo control write
#define CRreg  3	// Line Control
#define MCRreg 4	// Modem Control
#define LSRreg 5	// Line Status
#define MSRreg 6	// Modem Status
#define SCRreg 7  // Scratch register
#define LObaud 0	// Divisor Latch (LSB)
#define HIbaud 1	// Divisor Latch (MSB)
#define RDreg  (-1)

#define CERRADO  0
#define ABIERTO  1
#define OFF	     0
#define TRUE	  1
#define FALSE	  0


/* reduce heaplength and stacklength to make a smaller program in memory */
//extern unsigned _heaplen = 1024;
//extern unsigned _stklen  = 512;

#ifdef __cplusplus
	 #define __CPPARGS ...
#else
	 #define __CPPARGS
#endif

#define NULL 0
#define TRUE 1
#define FALSE 0
#define BOOL  int
#define ULONG unsigned int
#define WORD unsigned int
#define BYTE unsigned char
#define short unsigned int

#define LONIBBLE(w) ((BYTE)(w&0x0f))
#define HINIBBLE(w) (BYTE) (( (WORD)(w) >> 4) & 0x0F)

#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#define MAKEWORD(b, a)  ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKEBYTE(b, a)  ((((BYTE)(a)) | (((BYTE)(b))) << 4))

void ServicioInter (int port);
//void far *ServicioInter (int port);

void (far interrupt *TablaVect[4])(void);

void (far interrupt *TablaIntsRXD[16])(void);
void (far interrupt *TablaIntsRXL[16])(void);
void (far interrupt *TablaIntsMST[16])(void);
void (far interrupt *TablaIntsRLS[16])(void);

void far interrupt IntPort1();
void far interrupt IntPort2();


struct REGPACK reg;
char pic;
char lcr=0;

int byte2=0;
char byte1;

char BaudH;
char BaudL;

const char tabla[]=  {	0xbc,	//0
								0x52,	//1
								0xa8,	//2
								0xaa,	//3
								0x54,	//4
								0x56,	//5
								0x5c,	//6
								0x5e,	//7
								0xa4,	//8
								0xa6,	//9
								0xac,	//A
								0xae,	//B
								0xb4,	//C
								0xb6,	//D
								0xb8,	//E
								0xba	//F
							};


struct sio
{
	char BufferTx[MAX_LEN];
	char BufferRx[MAX_LEN];
	int  cabeza;
	int  cola;
	int  cont;	// Indica cada cuantos bytes ha de generar interrupt.
	int  IntCntRx;
	int  IntCntTx;
	int  abierto;

}ptrsio[NUMPORTS];



int NumPort (int port)
{
	#ifdef STANDARD
		static int nums[]={0x3f8,0x2f8,0x3e8,0x2e8};
	#endif

	#ifdef MOXA
		static int nums[]={0x240,0x248,0x250,0x258};
	#endif
	return ((port>3) ? -1 : (port<0) ? -1 : nums[port]);
}


int MiraVect (int puerto)
{
  #ifdef STANDARD
	static int nums[]={12,11,12,11}; // Para puertos standar
  #endif

  #ifdef MOXA
	static int nums[]={15,15,15,15}; // Para puertos moxa
  #endif

	return ((puerto>3) ? -1 : (puerto<0) ? -1 : nums[puerto]);
}

int sio_getch(int port)
{
	int  byte;
	char temp1;
	char temp2;
	char temp[10];

	byte = ptrsio[port].BufferRx[ptrsio[port].cola];
	if (++ptrsio[port].cola>=MAX_LEN) ptrsio[port].cola=0;
	ptrsio[port].IntCntRx--;

	return byte;
}

int sio_lctrl (int port, int mode)
{
	int puerto = NumPort (port);

	outp (puerto+MCRreg,mode);
	return (0);
}


int sio_putch (int port, int code)
{

	int cont;
	char ier,lcr;
	int puerto = NumPort(port);

	cont=ptrsio[port].IntCntTx;
	if (cont>=MAX_LEN) return (0); // Error, buffer lleno

	ptrsio[port].BufferTx[cont]=code;
	ptrsio[port].IntCntTx++;

	lcr = inp (puerto+CRreg);
	lcr&=0x7f;
	outp (puerto+CRreg,lcr);

	ier=inp (puerto+IERreg);
	ier|=0x3;
	outp (puerto+IERreg,ier);		// Permite interrupciones de TX e inicializa
											// el proceso de transmisi�n.
	return (1);
}


int sio_ofree (int port)
{
	return (MAX_LEN - ptrsio[port].IntCntTx);
}

void PonDLAB (int port, int modo)
{
	char lcr;


	port = NumPort(port);

	lcr = inp (port+CRreg);

	if (modo==0)
		lcr&=0x7f;
	else
		lcr|=0x80;

	outp (port+CRreg,lcr);

}
int sio_cnt_irq (int port, void (far interrupt *func)(),int cont)
{
	TablaIntsRXD[port]= func;
	ptrsio[port].cont = cont;

	return (0);
}

int sio_write (int port, char *buff, int len)
{
	int cont;
	int tmp;
	int n;
	unsigned char ier;
	int puerto= NumPort (port);

	cont=ptrsio[port].IntCntTx;

	if (cont>=MAX_LEN)
		return (0); // Error, buffer lleno


	PonDLAB (port,OFF);
	ier=inp (puerto+IERreg);
	ier&=0xfd;
	outp (puerto+IERreg,ier);		// Quita interrupciones

	len--;

	for (n=0;n<=len;n++)
	{
		ptrsio[port].BufferTx[ptrsio[port].IntCntTx]=buff[n];
		ptrsio[port].IntCntTx++;
	}

	ier=inp (puerto+IERreg);
	ier|=2;
	outp (puerto+IERreg,ier);		// Quita interrupciones
	return (n);
}

void sio_tx(int port,char byte)
{
	char lcr=0;
	char lsr=0;
	int puerto = NumPort(port);


	lcr = inp (puerto+CRreg);
	lcr&=0x7f;
	outp (puerto+CRreg,lcr);

	while (lsr==0)
		lsr=inp (puerto+LSRreg) & 0x20;

	outp (puerto+TXreg,byte);

	lsr=0;

}

/*

void sio_tx(int port,char byte)
{
	unsigned char tmp;
	char lcr=0,lsr,ier;
	int puerto = NumPort(port);

	lcr = inp (puerto+CRreg);
	lcr&=0x7f;
	outp (puerto+CRreg,lcr);

	ier=inp (puerto+IERreg);
	ier&=0xfd;
	outp (puerto+IERreg,ier);		// Inhibe interrupciones de TX

	while (lsr==0)
		lsr=inp (puerto+LSRreg) & 0x20;

	tmp = tabla[HINIBBLE(byte)];
	outp (puerto+TXreg,tmp);
	do
	{
		lsr=inp (puerto+LSRreg) & 0x20;
	}while (lsr==0);


	ier=inp (puerto+IERreg);
	ier|=0x3;
	outp (puerto+IERreg,ier);		// Permite interrupciones de TX e inicializa

	tmp = LONIBBLE(byte);
	outp (puerto+TXreg,tabla[tmp]);

	lsr=0;

}
*/
int sio_open (int port,int ibaud, char modo)
{
	int n;
	int vect;
	unsigned char ier;
	char tmp;
	int baud;
	int puerto = NumPort(port);

	PonDLAB (port,OFF);

	for (n=0;n<10;n++)
	inp (puerto+n);

	sio_flush(port);

	vect = MiraVect (port);
	TablaVect[port]=getvect (MiraVect (puerto));
	setvect (vect,IntPort1);

	lcr = inp (puerto+CRreg);
	lcr&=0x7f;
	outp (puerto+CRreg,lcr);

	ier=inp (puerto+IERreg);
	ier|=0x1;
	outp (puerto+IERreg,ier);		// Permite interrupciones de RX.

	ptrsio[port].abierto = TRUE;

	lcr = inp (puerto+MCRreg);
	lcr&=0xfd;
	lcr|=0x08;
	outp (puerto+MCRreg,lcr);

	switch (ibaud)
	{
		case 110:
			baud = 1047;	//110 b/s
		break;

		case 150:
			baud = 768;		// 150 b/s
		break;

		case 300:
			baud = 384;		// 300 b/s
		break;

		case 600:
			baud = 192;		// 600 b/s
		break;

		case 1200:
			baud = 96;		// 1200 b/s
		break;

		case 2400:
			baud = 48;		// 2400 b/s
		break;

		case 4800:
			baud = 24;		// 4800 b/s
		break;

		case 9600:
			baud = 12;		// 9600 b/s
		break;

		case 19200:
			baud = 6;		// 19200 b/s
		break;

		case 38400:
			baud = 3;		// 38400 b/s
		break;
	}

	tmp = modo & 0x1f;
	tmp|= 0x80;

	BaudH = (baud & 0xff00)>>8;
	BaudL = (baud & 0x00ff);

	outp (puerto+CRreg,tmp);
	outp (puerto+LObaud,BaudL);
	outp (puerto+HIbaud,BaudH);

	tmp&=0x7f;
	outp (puerto+CRreg,tmp);

	#ifdef STANDARD
		lcr = inp (0x21);
		lcr&=0xe7;
		outp (0x21,lcr);
	#endif

	#ifdef MOXA
		lcr = inp (0x21);
		lcr&=0x7f;
		outp (0x21,lcr);
	#endif

	outp (0x20,0x20);

	return (0);
}

// ----------------------- RUTINAS INTERRUPCION --------------------------

void far interrupt IntPort1(void)
{

	unsigned char ch;

	#ifdef STANDARD
	ServicioInter (2);
	ServicioInter (0);
	ServicioInter (1);
	#endif

	#ifdef MOXA
	int n;
	char byte=0,byte1;
	int pic;

	do{

		byte=inp (0x2b0); // Miramos primera placa MOXA

		for (n=0;n<4;n++,byte>>=1) // Comprobamos primera placa
		{
			byte1 = byte & 0x01;
			if (byte1==0) ServicioInter (n);
		}
	} while (inp (0x2b0)!=0xff);


	#endif
	outp (0x20,0x20);
}


void sio_reset(void)
{
	int port,n;


	for (port=0;port<4;port++);
	{
		for (n=0;n<7;n++)
		inp ((NumPort(port))+n);
	}
}

int sio_flush(int port)
{
	ptrsio[port].cabeza=0;
	ptrsio[port].cola=0;;
	return (0);

}


int sio_close (int port)
{

	int puerto = NumPort (port);


	PonDLAB (port,OFF);

	ptrsio[port].cabeza  = 0;
	ptrsio[port].cola	   = 0;
	ptrsio[port].IntCntRx= 0;
	ptrsio[port].IntCntTx= 0;
	ptrsio[port].cont		= 0;
	ptrsio[port].abierto = 0;
	TablaIntsRXD[port]=0;
	TablaIntsRXL[port]=0;
	TablaIntsMST[port]=0;
	TablaIntsRLS[port]=0;

	setmem (ptrsio[port].BufferRx,MAX_LEN,0);
	setmem (ptrsio[port].BufferTx,MAX_LEN,0);

	outp (NumPort(port)+IERreg,0);

	setvect (MiraVect(puerto),TablaVect[port]);

	return (0);
}

void ServicioInter (int port)
{
	int puerto,n;
	int lcr;
	int ier;
	int byte=0,byte1=0;
	int IntPen;
	static char buff[2];
	static int idx;
	unsigned char *tb = tabla;


	puerto = NumPort(port);

	lcr = inp (puerto+CRreg);
	lcr&=0x7f;
	outp (puerto+CRreg,lcr);

	byte = inp (puerto+IIDreg);

	byte1 = byte & INT_RXD;
	if (byte1==INT_RXD)
	{
		byte2=inportb (puerto);

		for (n=0;n<16;n++)
		{
			if (byte2==*tb++)
			{
				buff[idx]=n;
				idx++;
				break;
			}
		}

		if (idx==2)
		{
			byte2=MAKEBYTE (buff[0],buff[1]);
			idx=0;
			ptrsio[port].IntCntRx++;
			ptrsio[port].BufferRx[ptrsio[port].cabeza] = byte2;

			if (++ptrsio[port].cabeza>=MAX_LEN) ptrsio[port].cabeza=0;

			if (ptrsio[port].cont && ptrsio[port].cont<=ptrsio[port].IntCntRx)
			{
				TablaIntsRXD[port]();
			}
		}
	}

	byte1 = byte & INT_TXD;
	if (byte1==INT_TXD)
	{

		if (ptrsio[port].IntCntTx)
		{
			sio_tx (port,ptrsio[port].BufferTx[0]);
			memmove (ptrsio[port].BufferTx,ptrsio[port].BufferTx+1,MAX_LEN);
			ptrsio[port].IntCntTx--;

		}
	}


//		byte1 = byte & INT_RXL;
//		if (byte1==INT_RXL)
//		TablaIntsRXL[port]();

	byte1 = byte & INT_RLS;
	if (byte1==INT_RLS && TablaIntsRLS[port])
	{
		byte1=inportb (puerto+LSRreg);
		TablaIntsRLS[port]();
	}

	if (!byte)
	{
		byte1=inportb (puerto+MSRreg);

		if (TablaIntsMST[port]) TablaIntsMST[port]();
	}

}


