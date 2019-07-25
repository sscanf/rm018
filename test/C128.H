#ifndef __MOXA
#define __MOXA 1

/*      MODEM LINE STATUS       */
#define S_CTS           0xfe
#define S_DSR           0x02
#define S_RI            0x04
#define S_CD            0x08

/*      MODEM CONTROL setting   */
#define C_DTR           0x01
#define C_RTS           0x02
#define ABIERTO  1
#define CERRADO  0
#define MAX_LEN  512

#define INT_MST   0x00  // Interrupci¢n por modem status
#define INT_TXD   0x02  // Interrupci¢n por tx data
#define INT_RXL   0x06  // Interrupci¢n por rx line status
#define INT_RXD   0x04  // Interrupci¢n por rx data
#define INT_RLS   0xc0

#define WordLng_5  0x0
#define WordLng_6  0x1
#define WordLng_7  0x2
#define WordLng_8  0x3

#define ParityNone 0x0
#define ParityEven 0x18
#define ParityOdd  0x8

#define StopBit_1  0x0
#define StopBit_2  0x4


void UartRst(int port);
void PonDLAB (int port ,int modo);

void sio_reset(void);
int sio_getports (void);
int sio_open (int,int,char);
int sio_close (int port);
int sio_getch(int port);
int sio_linput (int port, char *buff, int len, int term);
int sio_putch (int port, int code);
int sio_putb (int port,char *buff, int len);
int sio_write (int port,char *buff, int len);
int sio_lstatus (int port);
int sio_cnt_irq (int port, void (interrupt (*func)()),int cont);
int  sio_modem_irq (int port, void (interrupt (*func)()));
int sio_ofree (int port);
int sio_oqueue (int port);
char sio_rx(int port);
int  sio_lctrl (int port, int mode);

void UartRst(int port);
void sio_end(void);
int  sio_ioctl (int port,char init);
void sio_tx(int port,char byte);
int sio_flush (int);
int sio_overlap (int port, int mode);
int sio_ifree (int port);

#endif