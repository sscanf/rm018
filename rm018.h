#define FALSE 0
#define TRUE  1
#define NULL  0

#define R_COUNTER      0x0001
#define PRESCALER_64   0x0000
#define PRESCALER_32   0x1000

#define FRAME   0xeca1
#define EOF     13 //Campo de final de mensaje para el DTE
#define LEN_MSG 96 //Largura máxima de los mensajes del DTE

// Definición de mensajes

#define MSG_ACT   0xff    //Activación
#define MSG_DES   0xfe    //Desactivación
#define MSG_ACK   0xc0    //ACK
#define MSG_NACK  0xc1    //NACK
#define MSG_OFF   0xc1    //Mensaje para desconectar el link
#define MSG_PDTS  0xd0    //Petición de datos
#define MSG_CID   0xfd    //Cambio de indicativo
#define MSG_PID   0xe0    //Programación de indicativo
#define MSG_TXD   0xd4    //Envío de datos
#define MSG_PTT   0x01    //PTT
#define MSG_TEST  0x02    //Test


#define CTS       2  //Clear To Send (RS232)
#define LE        5  //Load Enable PLL RX

#define LOCAL    PORTC.1
#define WAKEUP	  PORTC.0

#define DATA 0x03bc
#define STATUS DATA+1

#define READ  0x180  /* 1 10 A8 A7A6A5A4 A3A2A1A0 */
#define EWEN  0x130  /* 1 001 1XXX XXXX */
#define ERASE 0x1c0 /* 1 11 A8 A7A6A5A4 A3A2A1A0 */
#define WRITE 0x140 /* 1 01 A8 A7A6A5A4 A3A2A1A0 */
#define ERAL  0x120  /* 1 00 1  0A6A5A4 A3A2A1A0 */
#define WRAL  0x110  /* 1 00 0  1A6A5A4 A3A2A1A0 */
#define EWDS  0x100  /* 1 00 0  0A6A5A4 A3A2A1A0 */

#define ADDR_REQ_ON     PORTC.2 = TRUE;
#define ADDR_REQ_OFF    PORTC.2 = FALSE;
#define SUB_MODE_ON     PORTB.7 = TRUE;
#define SUB_MODE_OFF    PORTB.7 = FALSE;
#define RTS_ACTIVE      PORTC.0 = FALSE;
#define RTS_INACTIVE    PORTC.0 = TRUE;
#define MODEM_RESET     PORTB.5 = TRUE;delay_ms(100);PORTB.5=FALSE;
#define SIO_RESET       PORTC.2 = TRUE;NOP();NOP();NOP();PORTC.2=FALSE;
#define RELE_ON			PORTB.7=TRUE;
#define RELE_OFF		   PORTB.7=FALSE;

#define TIME   10
#define BUSY   0
