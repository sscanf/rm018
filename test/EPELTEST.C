#include "c128.h"
#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <string.h>
#include <alloc.h>

#define LEN_MSG 120 //Largura máxima de los mensajes del DTE


#define MSG_ACT   0xff    //Activación
#define MSG_DES   0xfe    //Desactivación
#define MSG_ACK   0xc0    //ACK
#define MSG_OFF   0xc1    //Mensaje para desconectar el link
#define MSG_NACK  0xc2
#define MSG_PDTS  0xd0    //Petición de datos
#define MSG_CID   0xfd    //Cambio de indicativo
#define MSG_PID   0xe0    //Programación de indicativo
#define MSG_TXD   0xd4    //Envío de datos
#define MSG_PTT   0x01    //PTT

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

const unsigned char ptt[]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0x01,0x0aa,0x3b,0x3c,0x36,0x38,0x0d};

unsigned char test[]= {0xec,0xa1,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0x02,0x0aa,0x32,0x3c,0x36,0x39,0x0d};

unsigned char act[18]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0xff};

unsigned char ack[18]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0xc0};

unsigned char des[18]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0xfe};

unsigned char pdts[160]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0xd0};

unsigned char cid[30]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0xfd};

unsigned char pid[30]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0xe0};

unsigned char txd[160]= {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
0x30,0x32,0xd4};

void MenuMensajes (void);
unsigned char indic[12];
unsigned int StrToHex (char *origen, char *destino);
unsigned int CalculaCRC (BYTE *buffer, int len);
void MsgACT (void);
void MsgACK (void);
int  MsgPDTS (void);
void MsgDES (void);
void MsgCID (void);
void MsgPID (void);
int  MsgTXD (void);
void ShowBuffer (void);
void TxLoop (void);
void RxLoop (void);
int DoDTEMsg(void);
void GenTest(void);

void interrupt IntCom ();
void interrupt IntRxFile ();

unsigned char buffer[1000];
int idx=0,modo=0;
int port,iIndex;
int bRxMsg;

void main (void)
{
	char a;

	port=0;

	sio_open (port,9600,ParityNone|WordLng_8|StopBit_2);
	sio_cnt_irq (port, IntCom, 1);

	while (1)
	{
		MenuMensajes();
/*		clrscr();
		printf ("*****************************************\n");
		printf ("Electr¢nica Barcelona S.L.               \n");
		printf ("Tester modems EPEL                       \n");
		printf ("*****************************************\n\n");
		printf (" 0.- Configuraci¢n puerto\n");
		printf (" 1.- PTT\n");
		printf (" 2.- Generar TEST con AA \n");
		printf (" 3.- Generar TEST con 00 \n");
		printf (" 4.- Generar TEST con FF \n");
		printf (" 5.- Men£ de mensajes\n");
		printf (" 6.- salir\n");

		a=getch();

		switch (a)
		{
			case '0':
				sio_close(port);
				printf ("Introduzca N£mero de puerto (0- COM1 / 1- COM2) ");
				scanf ("%d",&port);
				sio_open (port,9600,ParityNone|WordLng_8|StopBit_1);
				sio_cnt_irq (port, IntCom, 1);
			break;

			case '1':
				sio_write (port,ptt,19);
			break;

			case '2':
				sio_write (port,testaa,19);
			break;

			case '3':
				sio_write (port,test00,19);
			break;

			case '4':
				sio_write (port,testff,19);
			break;

			case '5':
				MenuMensajes();
			break;

			case '6':
				sio_close(port);
				exit(0);
			break;
		}
		*/
	}
}

void MenuMensajes (void)
{
	char ch=0;
	int n;

	while (ch!='9')
	{
		clrscr();
		printf ("*****************************************\n");
		printf ("     Electr¢nica Barcelona S.L.          \n");
		printf ("          MENé PRINCIPAL                 \n");
		printf ("*****************************************\n\n");
		printf ("Indicativo = ");

		for (n=0;n<12;n++)
			printf ("%x",indic[n]);

		printf ("\n\n0.- Seleccionar puerto serie\n");
		printf (" 1.- Definir indicativo\n");
		printf (" 2.- ACT - Activaci¢n\n");
		printf (" 3.- ACK - Acknowledge\n");
		printf (" 4.- PDTS- Petici¢n de datos\n");
		printf (" 5.- DES - Desactivaci¢n\n");
		printf (" 6.- CID - Cambio de indicativo\n");
		printf (" 7.- PID - Programaci¢n indicativo\n");
		printf (" 8.- TXD - Env¡o de datos\n");
		printf (" 9.- LOOP (PALA)\n");
		printf (" A.- LOOP (VAGON)\n");
		printf (" B.- Generar TEST\n");
		printf (" [Esc].- Salir\n\n");
		printf ("[espacio]- Buffer de recepci¢n\n");
		printf (" b.- Borra buffer de recepci¢n\n");
		printf(" s.- binario/ascii\n\n\n");

		ch = getch();

		switch (ch)
		{

			case '0':
				sio_close(port);
				printf ("Introduzca N£mero de puerto (0- COM1 / 1- COM2) ");
				scanf ("%d",&port);
				sio_open (port,9600,ParityNone|WordLng_8|StopBit_2);
				sio_cnt_irq (port, IntCom, 1);
			break;


			case '1':
			{
				char trama[80];
				printf ("Introduzca indicativo en hex. separando bytes mediante comas (m ximo 12 bytes)\n");
				scanf ("%s",trama);
				StrToHex(trama,indic);
			}
			break;

			case '2':
				MsgACT ();
			break;

			case '3':
				MsgACK ();
			break;

			case '4':
				MsgPDTS ();
			break;

			case '5':
				MsgDES ();
			break;

			case '6':
				MsgCID ();
			break;

			case '7':
				MsgPID ();
			break;

			case '8':
				MsgTXD ();
			break;

			case '9':
				TxLoop();
			break;

			case 'a':
			case 'A':
				RxLoop();
			break;

			case 'B':
				GenTest();
			break;


			case 27:
				exit(0);
			break;

			case ' ':
				ShowBuffer();
			break;
			case 'b':
				idx=0;
			break;

			case 's':
				modo=!modo;
			break;
		}
	}
}



void TxLoop (void)
{

	int bt,msg,n;
	sio_cnt_irq (port, IntRxFile, 1);

	printf ("-------- TEST LOOP para mensajes PDTS y TXD (Lado PALA)-----------\n\n");
	printf ("Cuando la PALA env¡a un PDTS al vag¢n, este responde con un TXD.\n");
	printf ("Antes de iniciar el ciclo, se ha de definir el largo del mensaje PDTS\n");
	printf ("y su contenido (pulse cualquier tecla para abortar)\n");

	bt = MsgPDTS();

	while (!kbhit())
	{
		printf ("Enviando PDTS ... ");
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xec);
		sio_putch(port,0xa1);
		sio_write (port,pdts,bt);

		for (n=0;n<2000 && !bRxMsg && !kbhit();n++) delay (1);

		if (n==2000)
			printf ("Sin respuesta\n");
		else
		{

			msg=DoDTEMsg ();


			if (msg==MSG_TXD)
			{
				printf ("Recibido TXD %d ms\n",n);
			}
		}


	}
	getch();
	sio_cnt_irq (port, IntCom, 1);

}

void RxLoop (void)
{

	int bt,msg,n;
	sio_cnt_irq (port, IntRxFile, 1);

	printf ("----------TEST LOOP para mensajes PDTS y TXD (Lado VAGON)-------\n\n");
	printf ("Cuando la PALA env¡a un PDTS al vag¢n, este responde con un TXD.\n");
	printf ("Antes de iniciar el ciclo, se ha de definir el largo del mensaje TXD\n");
	printf ("y su contenido\n (pulse cualquier tecla para abortar)");

	bt = MsgTXD();

	while (!kbhit())
	{
		while(!bRxMsg && !kbhit());

			msg=DoDTEMsg ();


			if (msg==MSG_PDTS)
			{
				printf ("Recibido PDTS\n");
				sio_putch(port,0xff);
				sio_putch(port,0xff);
				sio_putch(port,0xff);
				sio_putch(port,0xff);
				sio_putch(port,0xec);
				sio_putch(port,0xa1);
				while (!sio_write (port,txd,bt));
			}
	}
	while (!kbhit())
	{
		if (sio_ofree(port)>1)
			sio_putch(port,0xaa);
	}
	getch();
	sio_cnt_irq (port, IntCom, 1);

}

void GenTest (void)
{
	char iDatos[100];
	char *sDatos[100];
	char NumBytes[80];
	char b,tmp;
	short crc;

	printf ("Entre secuencia de test (M ximo 1 byte) \n");
	scanf ("%s",sDatos);
	StrToHex (sDatos,iDatos);

	test[15]=iDatos[0];

	crc = CalculaCRC (test+2,14);
	test[16]=HIBYTE(crc);
	b = HINIBBLE (test[16]);
	b+=0x30;
	tmp=b;
	test[17]=HIBYTE(crc);
	b=LONIBBLE (test[17]);
	b+=0x30;
	test[17]=b;
	test[16]=tmp;

	test[18]=LOBYTE(crc);

	b = HINIBBLE (test[18]);
	b+=0x30;
	tmp=b;

	test[19]=LOBYTE(crc);
	b=LONIBBLE (test[19]);
	b+=0x30;
	test[19]=b;
	test[18]=tmp;

	sio_write (port,test,21);
}

void ShowBuffer (void)
{
	int n;
	int tmp;
	char ch;

	clrscr();

	for (n=0;n<idx;n++)
	{
		if (modo)
			cprintf ("%02x ",buffer[n]);
		else
			cprintf ("%c",buffer[n]);

	}

	while (1)
	{
		while (!kbhit()&&tmp==idx);
		if (kbhit())
		{
			ch = getch();
			if (ch==' ')
				return;
		}
	}

	clrscr();

}


void MsgACT (void)
{
	int bt=13,n;

	BYTE tmp,b;
	short crc=0;

	memcpy (act,indic,12);

	crc = CalculaCRC (act,bt);
	act[bt]=HIBYTE(crc);
	b = HINIBBLE (act[bt]);


	b+=0x30;
	tmp=b;

	act[bt+1]=HIBYTE(crc);
	b=LONIBBLE (act[bt+1]);
	b+=0x30;
	act[bt+1]=b;
	act[bt]=tmp;
	bt+=2;

	act[bt]=LOBYTE(crc);
	b = HINIBBLE (act[bt]);
	b+=0x30;
	tmp=b;

	act[bt+1]=LOBYTE(crc);
	b=LONIBBLE (act[bt+1]);
	b+=0x30;
	act[bt+1]=b;
	act[bt]=tmp;
	bt+=2;
	act[bt]=13;
	bt++;
	act[bt]=0;

	while (sio_ofree(port)<bt+6);

		for (n=0;n<20;n++)
			sio_putch(port,0xaa);

		sio_putch(port,0xec);
		sio_putch(port,0xa1);
		sio_write (port,act,bt);
}

void MsgACK (void)
{
	int bt=13;

	BYTE tmp,b;
	short crc=0;

	memcpy (ack,indic,12);

	crc = CalculaCRC (ack,bt);
	ack[bt]=HIBYTE(crc);
	b = HINIBBLE (ack[bt]);


	b+=0x30;
	tmp=b;

	ack[bt+1]=HIBYTE(crc);
	b=LONIBBLE (ack[bt+1]);
	b+=0x30;
	ack[bt+1]=b;
	ack[bt]=tmp;
	bt+=2;

	ack[bt]=LOBYTE(crc);
	b = HINIBBLE (ack[bt]);
	b+=0x30;
	tmp=b;

	ack[bt+1]=LOBYTE(crc);
	b=LONIBBLE (ack[bt+1]);
	b+=0x30;
	ack[bt+1]=b;
	ack[bt]=tmp;
	bt+=2;
	ack[bt]=13;
	bt++;
	ack[bt]=0;
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
	sio_putch(port,0xec);
	sio_putch(port,0xa1);
	sio_write (port,ack,bt);
}


int MsgPDTS (void)
{
	unsigned int bt,len;

	char iDatos[100];
	char *sDatos[100];
	char NumBytes[80];

	BYTE tmp,b;
	short crc=0;

	memcpy (pdts,indic,12);

	printf ("total bytes (max 99)= ");
	scanf ("%s",NumBytes);
	if (strlen (NumBytes)<2)
	{
		NumBytes[1]=NumBytes[0];
		NumBytes[0]='0';
	}

	pdts[13]=NumBytes[0];
	pdts[14]=NumBytes[1];

	printf ("Entre Datos (En Hex. separado por comas)\n");
	scanf ("%s",sDatos);

	len=StrToHex (sDatos,iDatos);

	memcpy (pdts+15,iDatos,len);

	bt=len+15;
	crc = CalculaCRC (pdts,bt);

	pdts[bt]=HIBYTE(crc);
	b = HINIBBLE (pdts[bt]);


	b+=0x30;
	tmp=b;

	pdts[bt+1]=HIBYTE(crc);
	b=LONIBBLE (pdts[bt+1]);
	b+=0x30;
	pdts[bt+1]=b;
	pdts[bt]=tmp;
	bt+=2;

	pdts[bt]=LOBYTE(crc);
	b = HINIBBLE (pdts[bt]);
	b+=0x30;
	tmp=b;

	pdts[bt+1]=LOBYTE(crc);
	b=LONIBBLE (pdts[bt+1]);
	b+=0x30;
	pdts[bt+1]=b;
	pdts[bt]=tmp;
	bt+=2;
	pdts[bt]=13;
	bt++;
	pdts[bt]=0;
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
	sio_putch(port,0xec);
	sio_putch(port,0xa1);
	sio_write (port,pdts,bt);

	return bt;
}

void MsgDES (void)
{
	int bt=13;

	BYTE tmp,b;
	short crc=0;

	memcpy (des,indic,12);

	crc = CalculaCRC (des,bt);
	des[bt]=HIBYTE(crc);
	b = HINIBBLE (des[bt]);


	b+=0x30;
	tmp=b;

	des[bt+1]=HIBYTE(crc);
	b=LONIBBLE (des[bt+1]);
	b+=0x30;
	des[bt+1]=b;
	des[bt]=tmp;
	bt+=2;

	des[bt]=LOBYTE(crc);
	b = HINIBBLE (des[bt]);
	b+=0x30;
	tmp=b;

	des[bt+1]=LOBYTE(crc);
	b=LONIBBLE (des[bt+1]);
	b+=0x30;
	des[bt+1]=b;
	des[bt]=tmp;
	bt+=2;
	des[bt]=13;
	bt++;
	des[bt]=0;
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
	sio_putch(port,0xec);
	sio_putch(port,0xa1);
	sio_write (port,des,bt);
}

void MsgPID (void)
{
	unsigned int bt,len;

	char iDatos[100];
	char *sDatos[100];

	BYTE tmp,b;
	short crc=0;

	memcpy (pid,indic,12);

	printf ("Entre Nuevo indicativo (En Hex. separado por comas)\n");
	scanf ("%s",sDatos);

	len=StrToHex (sDatos,iDatos);

	memcpy (pid+13,iDatos,len);

	bt=len+13;
	crc = CalculaCRC (pid,bt);

	pid[bt]=HIBYTE(crc);
	b = HINIBBLE (pid[bt]);


	b+=0x30;
	tmp=b;

	pid[bt+1]=HIBYTE(crc);
	b=LONIBBLE (pid[bt+1]);
	b+=0x30;
	pid[bt+1]=b;
	pid[bt]=tmp;
	bt+=2;

	pid[bt]=LOBYTE(crc);
	b = HINIBBLE (pid[bt]);
	b+=0x30;
	tmp=b;

	pid[bt+1]=LOBYTE(crc);
	b=LONIBBLE (pid[bt+1]);
	b+=0x30;
	pid[bt+1]=b;
	pid[bt]=tmp;
	bt+=2;
	pid[bt]=13;
	bt++;
	pid[bt]=0;
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
	sio_putch(port,0xec);
	sio_putch(port,0xa1);
	sio_write (port,pid,bt);
}

void MsgCID (void)
{
	unsigned int bt,len;

	char iDatos[100];
	char *sDatos[100];

	BYTE tmp,b;
	short crc=0;

	memcpy (cid,indic,12);

	printf ("Entre Nuevo indicativo (En Hex. separado por comas)\n");
	scanf ("%s",sDatos);

	len=StrToHex (sDatos,iDatos);

	memcpy (cid+13,iDatos,len);

	bt=len+13;
	crc = CalculaCRC (cid,bt);

	cid[bt]=HIBYTE(crc);
	b = HINIBBLE (cid[bt]);


	b+=0x30;
	tmp=b;

	cid[bt+1]=HIBYTE(crc);
	b=LONIBBLE (cid[bt+1]);
	b+=0x30;
	cid[bt+1]=b;
	cid[bt]=tmp;
	bt+=2;

	cid[bt]=LOBYTE(crc);
	b = HINIBBLE (cid[bt]);
	b+=0x30;
	tmp=b;

	cid[bt+1]=LOBYTE(crc);
	b=LONIBBLE (cid[bt+1]);
	b+=0x30;
	cid[bt+1]=b;
	cid[bt]=tmp;
	bt+=2;
	cid[bt]=13;
	bt++;
	cid[bt]=0;
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
	sio_putch(port,0xec);
	sio_putch(port,0xa1);
	sio_write (port,cid,bt);
}



int MsgTXD (void)
{
	unsigned int bt,len;

	char iDatos[100];
	char *sDatos[100];
	char NumBytes[80];

	BYTE tmp,b;
	short crc=0;

	memcpy (txd,indic,12);

	printf ("total bytes (max 99)= ");
	scanf ("%s",NumBytes);
	if (strlen (NumBytes)<2)
	{
		NumBytes[1]=NumBytes[0];
		NumBytes[0]='0';
	}

	txd[13]=NumBytes[0];
	txd[14]=NumBytes[1];

	printf ("Entre Datos (En Hex. separado por comas)\n");
	scanf ("%s",sDatos);

	len=StrToHex (sDatos,iDatos);

	memcpy (txd+15,iDatos,len);

	bt=len+15;
	crc = CalculaCRC (txd,bt);

	txd[bt]=HIBYTE(crc);
	b = HINIBBLE (txd[bt]);


	b+=0x30;
	tmp=b;

	txd[bt+1]=HIBYTE(crc);
	b=LONIBBLE (txd[bt+1]);
	b+=0x30;
	txd[bt+1]=b;
	txd[bt]=tmp;
	bt+=2;

	txd[bt]=LOBYTE(crc);
	b = HINIBBLE (txd[bt]);
	b+=0x30;
	tmp=b;

	txd[bt+1]=LOBYTE(crc);
	b=LONIBBLE (txd[bt+1]);
	b+=0x30;
	txd[bt+1]=b;
	txd[bt]=tmp;
	bt+=2;
	txd[bt]=13;
	bt++;
	txd[bt]=0;

		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
		sio_putch(port,0xff);
	sio_putch(port,0xec);
	sio_putch(port,0xa1);
	sio_write (port,txd,bt);
	return bt;
}

unsigned int StrToHex (char *origen, char *destino)
{

	char *ptr=0xff;
	int i=0;

	ptr=origen;

	while (ptr>1)
	{
		sscanf (ptr,"%x",destino+i);
		ptr = strchr(ptr,',');
		ptr++;
		i++;
	}
	return i;
}

int DoDTEMsg(void)
{
	WORD crc,crc2;
	int tmp,tmp1;
	int crc_l;
	int crc_h;
	int n;

	//Primero convertimos el CRC16 en HEX ya que nos lo envían en Cutre-ASCII

	tmp = buffer[iIndex]-0x30;   //SemiByte bajo CRC  (byte bajo)
	tmp1= buffer[iIndex-1]-0x30; //SemiByte alto CRC (byte bajo)
	crc_l = MAKEBYTE (tmp1,tmp);  //Componemos el byte

	tmp = buffer[iIndex-2]-0x30; //SemiByte bajo CRC (Byte alto)
	tmp1= buffer[iIndex-3]-0x30; //SemiByte alto CRC (Byte alto)
	crc_h = MAKEBYTE (tmp1,tmp);  //Componemos el byte
	crc = MAKEWORD (crc_h,crc_l); //Componemos un WORD con los dos bytes

	crc2 = (WORD) CalculaCRC (buffer,iIndex-3);     //Calculamos el CRC

	if (crc2!=crc)
		goto fin;   //El CRC es incorrecto

//      for (n=0;n<12 ;n++ )
//         indic[n]=msg[n];  //Guardamos el indicativo recibido


	bRxMsg = FALSE;   //Quitamos flag de mensaje recibido.
	return buffer[12];

fin:
	bRxMsg = FALSE;   //Quitamos flag de mensaje recibido.
	return FALSE;

}


unsigned int CalculaCRC (BYTE *buffer, int len)
{
	int n,bit;
	WORD crc16;
	char tmp;
	crc16=0xffff;

	for (n=0;n<len;n++)
	{
		crc16^=buffer[n];

		for (bit=0;bit<8;bit++)
		{
			tmp=crc16&1;
			if (tmp)
			{
				crc16=crc16>>1;
				crc16^=(WORD)0xa001;
			}
			else
				crc16=crc16>>1;

		}
	}
	return crc16;
}

void interrupt IntCom ()
{

	BYTE byte;
	byte = sio_getch (port);
	buffer[idx]=byte;
	idx++;

	if (idx>1000)
	{
		memcpy (buffer,buffer+160,1000);
		idx-=160;
	}

}

void interrupt IntRxFile ()
{
	static BOOL bStart=FALSE;
	static unsigned char ant;

	unsigned char __byte = sio_getch (port);
	//Mientras no se esté procesando ningún mensaje.

	if (bRxMsg ==FALSE)
	{
		if (bStart==TRUE)
		{
			buffer[iIndex]=__byte;
			iIndex++;
		}

		//Cada vez que recibe un 0xeca1 resetea el contador del buffer
		if (__byte==0xa1 && ant==0xec)
		{
			bStart=TRUE;
			iIndex=0;
		}

		if (__byte==0x0d && bStart==TRUE)        //Final de mensaje?
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