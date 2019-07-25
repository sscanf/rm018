#define SQL		  PORTB.2   	 //SQL
#define TXEN1    3             //Tx Enable
#define TXEN2	  4             //Rx Enable
#define PTT      5

#define ECON_ON  PORTB.TXEN2=FALSE //Economizador
#define ECON_OFF PORTB.TXEN2=TRUE  //Economizador

#define RECEPCION		PORTB.TXEN1=FALSE;PORTB.TXEN2=FALSE;
#define TRANSMISION	PORTB.TXEN1=TRUE;PORTB.TXEN2=TRUE;

void RxMode (void);
void TxMode (void);
void LoadEnable (void);
void SetRadioTX (BOOL);
BOOL IsSqlActive ();
void test (char);
