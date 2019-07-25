#ifndef _SIO
#define _SIO

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

#define BAUD_110     0
#define BAUD_150     1
#define BAUD_300     2
#define BAUD_600     3
#define BAUD_1200    4
#define BAUD_2400    5
#define BAUD_4800    6
#define BAUD_9600    7
#define BAUD_19200   8
#define BAUD_38400   9

void UartRst(int port);
void PonDLAB (int modo);

long sio_getch();
void sio_reset(void);
void sio_write (char *buff);
int  sio_lctrl (int mode);
int sio_cnt_irq (void far *fun);
void UartISR ();
void sio_out(char byte);
void sio_open ();

#endif

