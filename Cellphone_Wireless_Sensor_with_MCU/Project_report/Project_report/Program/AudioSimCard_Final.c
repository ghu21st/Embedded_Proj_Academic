//Prog: AudioSimCard2, Audio channel signal sending &recieving (200,400,800Hz)
//Author: Gang Hu; Date: August, 2006;
//Revision: 9, Sept., 2006

//Object: Sending/receiving data by cellpone. To reduce the latency caused by traditional
//data communcation on cellpone(modem, GPRS, etc), this project create a SimCard which apply 16
//bit MCU/DSP to act as audio signal sampler, generator and data communication agent between PC
//and cellpone.It can also intercommunicate very well between ASimCardA and ASimCardB.In receiver, //sample audio signal(ADC) and transfer the data to PC to analyze the frequency by the use of FFT &
// decoding to restore the data sent by another module; or vice versa, it can encode command
// by PC and send code to ASimcard, then the card generates corresponded audio frequency and 
//sending to audio line-out (DAC) to cellpone or to another ASimCard by sending discrete audio
// sinewave points (32 points for 200Hz, 16 points for 400Hz, and 8 points for 800Hz).

//Peripheral: LED1-4; switch button S1, S2; audio output by DP1 and OP1, audio input by DP2 
//				and OP2. MCU Fosc: 20MHz*4=80MHz, 20 MIPs.
//Pins configuration: S1:RA11,S2:RD8,LED1:RD0,LED2:RD1,LED3:RD2,LED4:RD3,DA0(audio_in):RB9,
//	DA1:RB10, DA2:RB11, DA3:RB12,AD0(audio_in):RB2,AD1(potentiometer):RB3,AD2:RB4,AD3:RB5
//	AD4:RB8

//Communication: PC A->ASimcard A-> Cellpone(sender mode), Cellphone->SimCard B->PC (receiver mode)
//UART(Comm.)Command: '1'--400Hz, '2'--800Hz, '3'--1600Hz, '4'--end command, '5'--Request next 
//command, '7'--Receiving mode, '8'--Sending mode, '0'--error command

//Function: 
//---'MC' mode: S1 pressed,LED1 on,U2ART receive from PC,audio signal sending to cell phone;
//---'CM' mode: S1 pressed again,LED2 on,receive audio signal from cell phone,U2ART send to PC
//			S1: change 'MC' (sender mode, '8') to 'CM' (receiver mode, '7'), or vice versa.
//---A/D or D/A in use: S2 pressed, enable & LED3 flash, pressed again, disable 
//		also can work as freqency selection button(from 200Hz to 1.6KHz) for test
//---Error handling: any error occure,LED4 blinking,flash freq. can controlled by AD1(potentiomer)  
//Note: 'MC'--Comm. to Cell phone(sender mode); 'CM'---cell phone to Comm.() (receiver mode)
//
#include "p30F4013.h"
//
//--------------------------------------------------------------------------------
//Variables &Functions defination-------------------------------------------------
#define FCY 20000000 	// Fcy=Fosc/4=OSC*PLL/4=20MHz(PLL=8/2); cycle=0.05us

#define S1	PORTAbits.RA11 //digital input for switch 1, S1=0 pressed, S1=1 open
#define S2	PORTDbits.RD8	 //digital input for switch 2

#define AD0_Sel 02		//AN2, Line_in, 150us strictly
#define AD1_Sel 03		//AN3, potentiometer on board, (300us changable...)
#define AD2_Sel 04		//AN4, external sensor1, (225us changable...)
#define AD3_Sel 05		//AN5, external sensor2, (225us changable...)
#define AD4_Sel 08		//AN8, external sensor3, (225us changable...)

#define DA0_Sel	PORTBbits.RB9 //Audio output DP channel D/A0
#define DA1_Sel	PORTBbits.RB10 // D/A 1
#define DA2_Sel	PORTBbits.RB11 // D/A 2
#define DA3_Sel	PORTBbits.RB12 // D/A 3

#define LED1 PORTDbits.RD0	//LED1
#define LED2 PORTDbits.RD1	//LED2
#define LED3 PORTDbits.RD2	//LED3
#define LED4 PORTDbits.RD3	//LED4

#define U2Baud	113626		//under 20MIPs, for up to 7KHz respones(2 bytes) or 14KHz(1byte)
#define U2BUFFERSIZE 2		//can be 1 or 2 bytes to be sent/recive, U2ART buffer setting

#define THRESHOLDDRIFT	50	//define the threshold drift value for audio data start flag
#define THRESHOLD_BAL_CNT 10000 //# of points for calculate the dynamic threshold balance 
#define THRESHOLD_MAX 155	//240 for ASimCard, 150-160 for cellphone communication
#define THRESHOLD_MIN 100	//80 for ASimCard,95-105 for cellphone for communication

#define SENDDELAYLIMIT 192	//133, 266(ok), 200(good), 133(ok) >20ms(133),166(ok)
#define RCVPARSEWAIT 16		//64, 32(ok),
#define RCVUARTWAIT 8		//32, 16(ok),
#define RCVPARSEUARTWAIT 0  //16(ok), 0(good)
#define RCVUARTCOUNT 95		//100(good), 90
#define RCVFINISHDELAY 28	//>80-64=16, 32(good), 64(ok)

#define TESTWAIT 133
//---------------------------------
unsigned int index_P;	//sinewave table index pointer
unsigned int index_P2;	//sinewave table index pointer for 600Hz
unsigned int Out_Freq_Sel;	//0---200Hz, 1---400Hz, 2---800Hz
unsigned int Audio_Sel;	//selection of Audio sending(=0) or receiving(=1)
unsigned int ADScan_Sel;//selection of extern AD scaning and sampling, =0 disable, =1 enable
unsigned int Audio_Request_Checkpoint;
unsigned int temp;

unsigned int ADCVal_Linein,ADCVal_1,ADCVal_2,ADCVal_3,ADCVal_4; //ADC sample value final result
unsigned int ADCVal_CON[4];	//ADC extern sample calculation dimension
unsigned char Val_Linein, Val_AD1, Val_AD2, Val_AD3, Val_AD4; //final ADC value for the 5 AD port
const unsigned int channelTable[]={3, 4, 5, 8}; //extern A/D channel assign,AN3,4,5,8

unsigned int U2TXCount, U2RXCount;	//U2ART Tx,Rx counter
unsigned int uiU2TXBUF[U2BUFFERSIZE];	//defualt '0','1' for U2ART sending 
unsigned int uiU2RXBUF[U2BUFFERSIZE];
unsigned int U2RXVal_1, U2RXVal_2;

unsigned int i_AD_Send_Flag;	//for A/D sending to cellphone
unsigned char ADC_BCD[2];		//store the 2 4-bit HEX code of A/D sampling result 
unsigned int i_ADC_Status=0;
unsigned char ch_Mode=0B10000000;

unsigned int ui_Send_Cmd;	//the command # received from U2ART, 1,2,or 3
//test...
unsigned int ui_Freq_test=0x31;	//0x31=416Hz, 0x2=833Hz
unsigned int ui_Test_Flag=1;
//end test...

const int sineTable[]={127, 152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 
						233, 217, 198, 176, 152, 127, 102, 78, 56, 37, 21, 9, 2, 0,
						2, 9, 21, 37, 56, 78, 102};
const int sineTable600[]={127, 191, 238, 255, 238, 191, 127, 63, 16, 0, 16, 63};

unsigned int iErrFlag;	//if error occure, indicator LED4 light up &blinking

struct{
	unsigned	Timer1:1;
	unsigned	Timer2:1;
	unsigned	Timer1_Lock:1;
	unsigned	Timer2_Lock:1;
	unsigned	Button1_Lock:1;
	unsigned	Button2_Lock:1;
	unsigned	SW1_Pressed:1;
	unsigned	SW2_Pressed:1;
	unsigned	U2ART_WaitCmd:1;	//U2ART module wait for next command, 0-No, 1-yes
	unsigned	:1;
	unsigned 	:1;
	unsigned 	:1;
	unsigned	:1;
	unsigned 	:1;
	unsigned 	:1;
	unsigned	:1;
}Flags;

unsigned int uiCount0, uiCount1,uiCount2,uiCount8,uiCount16,
				uiCount,uiCount125,uiCount500,uiCount_V;//timing variables
unsigned int uiCount300u, uiCount600u, uiCount1200u;//for 300us, 600us, 1200us timing variables
unsigned int ui300_Flag, ui600_Flag, ui1200_Flag;	//audio point change flag

unsigned int i_Send_StatusM=0;
unsigned int ui_Delay_Cnt=0;
unsigned int i_RCV_StatusM=0;
unsigned int ui_RCV_Loop_Cnt=0;
unsigned int ui_Audio_RCV_Cnt=0;
unsigned long ui_Parse_UART_Cnt=0;
unsigned int ui_RCV_UART_Cnt=0;
unsigned int ui_RCV_FIN_Delay=0;
//------------------------------------------
void Init(void);	//General whole system initialize except independant module 
void __attribute__((__interrupt__)) _T1Interrupt(void);
void __attribute__((__interrupt__)) _U2TXInterrupt(void);

int OneFiftyMicroSecRoutine(void);
int ThreeHundredMicroSecRoutine(void);
int SixHundredMicroSecRoutine(void);
int TwelveHundredMicroSecRoutine(void);
int OneMilliSecRoutine(void);
int TwoMilliSecRoutine(void);
int EightMilliSecRoutine(void);
int SixteenMilliSecRoutine(void);
int TwoHundredMilliSecRoutine(void);
int HalfSecRoutine(void);
int VariableTimeRoutine(unsigned int uiIn);

int CheckSwitch(void);
void SW1_Pressed(void);
void SW2_Pressed(void);
void SW_Stat_Show(void);
void LED_Action1(void);
void LED_ACtion2(void);		//error flag indicator on LED4, Freq. controlled by AD1

void ADC_SR(unsigned int iNum);
void Audio_Send(void);
void Audio_TimingSend(unsigned int iFreq);
void Audio_Receive(void);

//void MC_Mode_Proc(void);
//void CM_Mode_Proc(void);
int A_Parse_Proc(unsigned int iAVal);
int Audio_Data_Gen(unsigned int iAFreq);
//int A_Flag_Send(void);
void Sender_Status_Machine(void);
void Receiver_Status_Machine(void);
void Sender_Machine_Init(void);
void Receiver_Machine_Init(void);
unsigned int ADC_Send_Machine(void);

void Init_U2ART(void);
void U2ART_Send(unsigned int uiSend1, unsigned int uiSend2);
void U2ART_Receive(void);

//error traps...
void _ISR_OscillatorFail(void);
void _ISR_AddressError(void);
void _ISR_StackError(void);
void _ISR_MathError(void);

//unsigned int iTestFlag1;
//----------------------------------------------------------------------------
//Initialization -------------------------------------------------
void Init(void)
{
	//Variables init...
	index_P=0;
	temp=0;
	Out_Freq_Sel=1;	//default audio output 
	ADScan_Sel=1;	//default: ADs scaning &sampling enabled
	Audio_Sel=1; 	//default is audio receiving, 'CM' mode
	i_AD_Send_Flag=0; //default no A/D port value sending to cell phone
	iErrFlag=0;	

	Init_U2ART();//Audio_Sel=0, 'MC', so U2ART is in receiving mode

	uiCount0=0; uiCount1=0; uiCount2=0; uiCount8=0;
	uiCount16=0; uiCount125=0; uiCount500=0;
	uiCount300u=0; uiCount600u=0; uiCount1200u=0;
	ui300_Flag=0; ui600_Flag=0; ui1200_Flag=0;

	for(temp=0; temp<4; temp++){	//init the ADCVal_CON dimension
		ADCVal_CON[temp]=0;
	}
	for(temp=0; temp<U2BUFFERSIZE; temp++){
		uiU2TXBUF[temp]=0;
		uiU2RXBUF[temp]=0;

	}	
	ADCVal_Linein=0; ADCVal_1=0; ADCVal_2=0; ADCVal_3=0; ADCVal_4=0; 
	Val_Linein=0; Val_AD1=0; Val_AD2=0; Val_AD3=0; Val_AD4=0;	
	U2RXVal_1=0; U2RXVal_2=0;

	ui_Send_Cmd=1;

	//Flags init...
	Flags.Timer1=0;
	Flags.Timer2=0;
	Flags.Timer1_Lock=0;
	Flags.Timer2_Lock=0;
	Flags.Button1_Lock=0;
	Flags.Button2_Lock=0;
	Flags.SW1_Pressed=0;
	Flags.SW2_Pressed=0;
	Flags.U2ART_WaitCmd=0;

	//interrupt & priority init...
		//Timer1:7, SPI1:6, A/D:5, U2ART:4
	IPC0=0x7000;	//set the Timer1 has the highest priority
	IPC2=0x5006;	//A/D=5, U1ART disabled, SPI1=6;
	IPC6=0x0044;	//SPI2 disabled, U2ART Tx=4 and Rx=4

	INTCON1bits.NSTDIS=1;	//interrupt nested disabled
//	INTCON1bits.NSTDIS=0;	//interrupt nested enabled

	//Port init...
	PORTA=0xffff;
	TRISA=0xffff;	//RA11 digital input for switch button S1

	ADPCFG=0x1e00;	//set RB9,RB10,RB11,RB12 as digital I/0, B0001 1110 0000 0000 = 0x1e00
	PORTB=0x1e00; 	//set B0001 1110 0000 0000=0x1e00, RB9,10,11,12 are high level
	TRISB=0x013f;	//0x013f, RB8, RB5, RB4, RB3, RB2, RB1, RB0 are analog input, other output

	PORTD=0x0;		//default: all LEDs off, low level (RD0-RD3)
	TRISD=0x0300;	//RD9, RD8(for S2) are digital input, others output (include for LEDs)
	
	//Timer1 init...
	TMR1=0;
	PR1=1499;		//set 75us per cycle, 75/.05=1500
	IFS0bits.T1IF=0;	//clear Timer1 flag
	IEC0bits.T1IE=1;	//enable Timer1
	T1CON=0x8000;	//start Timer1

	//SPI init...
	IFS0bits.SPI1IF=0;	//clear interrupt flag
	IEC0bits.SPI1IE=0; 	//SPI interrupt 
	SPI1CON=0x0535; 	//master mode, 4:1&16:1,for 20MIPs, 313KHz output
	SPI1STAT=0x8000;	//enable SPI (include clear overflow bit)

	//AD0 for audio init...
//	ADCON1bits.ADON=0;	//turn A/D off for config the A/D
	ADPCFG=0xFEC3;		//AN2,3,4,5,8 as analog input, others are digital output
	ADCHS=AD0_Sel;		//AN2 selected for Line_in input
	ADCON1=0x00E0;		//internal counter(auto start sampling)
	ADCSSL=0x0;			//No scan channel
	ADCON3=0x041e;		//after 3 or 4 Tad sampling, convert begin(ADCS=26, Tad=675ns)
							//or Min Tad=770ns, ADCS=30, Tad=775ns
//	ADCON2=0x0004;		//2 samples on one channel
	ADCON2=0;			//One channel one sampling
	ADCON1bits.ADON=1;	//turn A/D on

	//Receiver and sender status machine init....
	Receiver_Machine_Init();
	Sender_Machine_Init();

	//for testing..
//	iTestFlag1=0;
}
//----------------------------------------------------------
//Interrupt service--------------------------------
void __attribute__((__interrupt__)) _T1Interrupt(void) //Timer1 ISR
{
	IFS0bits.T1IF=0;	//clear interrupt flag a must;
	Flags.Timer1=1;		//set the timer1 active flag
}

void __attribute__((__interrupt__)) _U2TXInterrupt(void)	//U2ART Tx ISR
{
	//
	IFS1bits.U2TXIF=0;	//clear the interrupt flag
	U2TXREG=uiU2TXBUF[0];
	IEC1bits.U2TXIE=0;		//must be cleared by user, otherwise, unusal char are sent


}
//---------------------------------------------------------------------
//Timing related procedures....
//------------------------------------------------------------
int OneFiftyMicroSecRoutine(void)
{	//150us timer, 2*75us=150us
	if(uiCount0>=1){
		uiCount0=0;
		return 1;
	}else{
		++uiCount0;
		return 0;
	}
}

int ThreeHundredMicroSecRoutine(void)
{	//300us timer, 4*75us=300us
	if(uiCount300u>=3){
		uiCount300u=0;
		return 1;
	}else{
		++uiCount300u;
		return 0;
	}
}

int SixHundredMicroSecRoutine(void)
{	//600us timer, 8*75us=600us
	if(uiCount600u>=7){
		uiCount600u=0;
		return 1;
	}else{
		++uiCount600u;
		return 0;
	}
}

int TwelveHundredMicroSecRoutine(void)
{	//1200us timer, 16*75us=1200us
	if(uiCount1200u>=15){
		uiCount1200u=0;
		return 1;
	}else{
		++uiCount1200u;
		return 0;
	}
}

int OneMilliSecRoutine(void)
{	//1ms timer, 12*75us=0.9ms
	if(uiCount1>12){
		uiCount1=0;
		return 1;
	}else{
		++uiCount1;
		return 0;
	}
}

int TwoMilliSecRoutine(void)
{	//2ms timer, 26*75us=1.95ms
	if(uiCount2>25){
		uiCount2=0;
		return 1;
	}else{
		++uiCount2;
		return 0;
	}
}

int EightMilliSecRoutine(void)
{	//8ms timer, 106*75us=7.95ms
	if(uiCount8>105){
		uiCount8=0;
		return 1;
	}else{
		++uiCount8;
		return 0;
	}
}

int SixteenMilliSecRoutine(void)
{	//16ms timer, 212*75us=15.9ms
	if(uiCount16>211){
		uiCount16=0;
		return 1;
	}else{
		++uiCount16;
		return 0;
	}
}

int OneHundredMilliSecRoutine(void)
{	//125ms timer, 1666*75us=124.95ms
	if(uiCount125>1660){
		uiCount125=0;
		return 1;
	}else{
		++uiCount125;
		return 0;
	}
}

int HalfSecRoutine(void)
{	//500ms timer, 6666*75us=499.95ms
	if(uiCount500>6647){
		uiCount500=0;
		return 1;
	}else{
		++uiCount500;
		return 0;
	}
}

int VariableTimeRoutine(unsigned int uiIn)
{	//variable time settings for A/D1 
	if(uiCount_V>uiIn){
		uiCount_V=0;
		return 1;
	}else{
		++uiCount_V;
		return 0;
	}
} 

//---------------------------------------------------------------------------
//Peripheral actions go here...
//-------------------------------------------------------
int CheckSwitch(void) //return: 1---button 1 pressed, 2--- button 2 pressed
{	//Self-lock: press the button down, count 5ms, then signal effective;
	//           wait for the button up, then check new button action. 
	//for Button S1
	static unsigned int iS1Count=0;
	static unsigned int iS2Count=0;
	static int iButton1Status=0; //0---button up, 1---button pressed(last 3ms)
	static int iButton2Status=0; //0---button up, 1---button pressed(last 3ms)
	int iRet1=0;
	int iRet2=0;

	//check if the button self-lock enabled, if true, exit this function
	if((Flags.Button1_Lock)||(Flags.Button2_Lock)){
		return 0;
	}
	
	//check button S1 pressed (3ms)
	if((!S1)&&(!iButton1Status)){
		if(++iS1Count>=3){
			iS1Count=0;
			iButton1Status=1;
			Flags.SW1_Pressed=1;
			iRet1=1;
			
			//vvvvvvButton S1 pressed, vvvvvvvvvvvvvvvvvvvvvvv
			SW1_Pressed();
			//^^^^^^end actions^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		}else{
			iRet1=0;
		}
	}	
		//wait button 1 up (3ms)
	if((S1) && (iButton1Status)){
		if(++iS1Count>=3){
			iS1Count=0;
			iButton1Status=0; //release the self-lock
			Flags.SW1_Pressed=0;
	//all S1 button up action go from here....
			//vvvvvvvvvvv S1 up vvvvvvvvvvvvvvvvvv

			//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		}
	}
		
	//check button S2 pressed (3ms)
	if((!S2) && (!iButton2Status)){
		if(++iS2Count>=3){
			iS2Count=0;
			iButton2Status=1;
			Flags.SW2_Pressed=1;
			iRet1=1;

			//vvvvvvButton S2 pressed vvvvvvvvvvvvvvvvvvvvv
			SW2_Pressed();
			//^^^^^^^end actions^^^^^^^^^^^^^^^^^^^^^^^^^^^
		}else{
			iRet2=0;
		}
	}
		//wait button 2 up (3ms)
	if((S2) && (iButton2Status)){
		if(++iS2Count>=3){
			iS2Count=0;
			iButton2Status=0;
			Flags.SW2_Pressed=0;
	
	//all S2 button up actions go from here....
		//vvvvvvvvvvvvv S2 up vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		//active the AD sending mahcine to cell phone
			i_AD_Send_Flag=1;
		//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		}
	}
	
	return (iRet1+iRet2);
}

void SW1_Pressed(void)
{
	Flags.Button1_Lock=1;	//set the button lock, stop buttons checking
	//
	//Audio mode changing in the SW1 action, loop sending mode and receiving mode
	Audio_Sel=!Audio_Sel;
	
	//reset both receiver and sender status machine
	Receiver_Machine_Init();
	Sender_Machine_Init();

	//for testing...
//	iTestFlag1=1;

	//
	Flags.Button1_Lock=0; 	//unlock the button self-lock
}

void SW2_Pressed(void)
{
	int i=0;
	//enable or disable the extern A/D sampling. Note: only in sending mode 
	Flags.Button1_Lock=1;	//set the button lock, stop buttons checking

//send the A/D 1 value to cell phone
	 //sample the A/D 3 only (potentiometer)
	ADCHS=AD1_Sel;		//AN3 selected 
	ADCON1bits.ADON=1;	//turn A/D on
		//each time one sampling
	ADCON1bits.SAMP=1;	//manual start, after 4 Tad, the sampling begin
	for(i=0; i<80; i++); //delay for sampling begin
	while(!ADCON1bits.DONE);
	for(i=0; i<20; i++); //delay for sampling begin
	ADCVal_1=ADCBUF0;	//12 bit
	Val_AD1=(unsigned char)(ADCVal_1>>4); //get the high 8 bit's A/D value
 	
	//disassemble the A/D output
	ADC_BCD[0] = (unsigned char)(Val_AD1>> 4);
	ADC_BCD[0] = ADC_BCD[0] & 0x0F;
	if (ADC_BCD[0] > 9){
		ADC_BCD[0] = ADC_BCD[0] + 0x37;
	}else{ 
		ADC_BCD[0] = ADC_BCD[0] + 0x30;
	}
	
	ADC_BCD[1] = (unsigned char)(Val_AD1) & 0x0F;
	if (ADC_BCD[1] > 9){
		ADC_BCD[1] = ADC_BCD[1] + 0x37;
	}else{
		ADC_BCD[1] = ADC_BCD[1] + 0x30;
	}

	//only accept 7 bit for sending to cell phone
	ADC_BCD[0]=ADC_BCD[0]<<1; 
	ADC_BCD[1]=ADC_BCD[1]<<1; 

	//clear the CONTROL variables
	i_ADC_Status=0;
	ch_Mode=0B10000000;

	//enter into the sender mode
	Audio_Sel=0;
	Receiver_Machine_Init();
	Sender_Machine_Init();

	//		
	Flags.Button1_Lock=0; 	//unlock the button self-lock
}
//----------------------------------------------------------
//LEDs action-----------------------------------
void SW_Stat_Show(void)
{
	//set the LED1 on and LED2 off if it is 'MC' mode, else LED1 off, LED2 on
	if(!Audio_Sel){		//=0, 'MC' mode, PC->SimCard->cell
		LED1=LED1|1;
		LED2=LED2&0;
	}else{				//=1, 'CM' mode, cell->SimCard->PC
		LED1=LED1&0;
		LED2=LED2|1;
	}
}

void LED_Action1(void)
{	//LED1,3 action...
	//
	switch(Out_Freq_Sel){	//loop the selection of audio freq. from 200Hz, 400Hz to 800Hz
		case 0:
			LED1=LED1|1;
			LED2=LED2&0;
			LED3=LED3&0;
			break;
		case 1:
			LED1=LED1&0;
			LED2=LED2|1;
			LED3=LED3&0;
			break;
		case 2:
			LED1=LED1&0;
			LED2=LED2&0;
			LED3=LED3|1;
			break;
		default:
			iErrFlag=1;
	}
	//
}

void LED_Action2(void)
{	//LED2 action is controlled by A/D module enable/disable switch, light up LED3
	//
	LED3=!LED3; 	//LED3, A/D or D/A module enable indicator
	//
}	
//-------------------------------------------------
//A/D module-----------------------------
void ADC_SR(unsigned int iNum)//150us*4=600us sampling cycle, each 4 times sampling then average
{	//under 20MIPs,sample 4 channels A/D,ok.iNum the total number AD channel,should >=1 or <=4
	unsigned int iCHL;		//set the A/D channel selection
	static unsigned int iCnt=0;	//record the entering times 
	
	if(iNum<0||iNum>4){
		iErrFlag=1;
		return;
	}


	ADCON1bits.ADON=1;
	//
	if(iCnt<4){			//sample 4 times then get the average as the final A/D result
		++iCnt;
		for(iCHL=0; iCHL<iNum; iCHL++){
			ADCHS=channelTable[iCHL];
			ADCON1bits.SAMP=1;	//manual start, after 4 Tad, the sampling begin
			while(!ADCON1bits.DONE);
			ADCVal_CON[iCHL]+=ADCBUF0;
		}
	}else{
		iCnt=0;
		//get the 12 bits ADs value...
		ADCVal_1=ADCVal_CON[0]>>2;
		ADCVal_2=ADCVal_CON[1]>>2;
		ADCVal_3=ADCVal_CON[2]>>2;
		ADCVal_4=ADCVal_CON[3]>>2;
		for(iCHL=0; iCHL<iNum; iCHL++){
			ADCVal_CON[iCHL]=0;
		}		
		//get the 8 bits ADs value...
		Val_AD1=(char)(ADCVal_1>>4);
//		Val_AD2=(char)(ADCVal_2>>4);
//		Val_AD3=(char)(ADCVal_3>>4);
//		Val_AD4=(char)(ADCVal_3>>4);
	}
}
//-----------------------------------------------
//Audio module-----------------------
void Audio_Send(void)		//SPI clock:>6.4KHz (150us)
{	//
	static unsigned int iFlag1=0;
	//
	temp=0x1300+sineTable[index_P]; //fetch the sine value, start from index_P=0
							// 0x1300, 0x13 for DP command, 0x00 for sine table
	switch(Out_Freq_Sel-1){	//Out_Freq_Sel= 1: 416Hz; 2: 800Hz; 3: 605Hz; 4: 1666Hz; 5: 3.3KHz
		case 0:			
			index_P+=2;	//416Hz (16 points)
			break;
		case 1:
			index_P+=4;	//833Hz (8 points)
			break;
		case 2:
			index_P2+=1;	//605Hz (12 points, change sinetable)
			break;
		case 3:
			index_P+=8;	//1666Hz (4 points)
			break;
		case 4:
			if(iFlag1==0){//3332Hz (2 points)
				iFlag1=1;
				temp=0x13e0;	//good value for pass the low pass filter
			}else{
				iFlag1=0;
				temp=0x1320;	//optimized for passing through the low pass filter
			}
			break;
				
		default:		//default 200Hz
			index_P+=1;

	}
	if(index_P>31)
		index_P=0;
	if(index_P2>11)
		index_P2=0;
	//
	Audio_TimingSend(temp);		
}

void Audio_TimingSend(unsigned int uiSPI_Command)
{	//SPI operation, iSPI_Command = 0x1300 + Freq_sample_point;
	unsigned int SPIBuf_Tmp=0; 
	//
	IFS0bits.SPI1IF=0;
	while(SPI1STATbits.SPIRBF)//check if the buffer is full before sending
	SPIBuf_Tmp=SPI1BUF; //force read from SPI buffer to clear it
	SPI1STATbits.SPIROV=0;	//clear overflow error flag for SPI
	IFS0bits.SPI1IF=0;
	DA0_Sel=DA0_Sel&0;		//enable the DA0 SPI for audio output, and start DP action
	SPI1BUF=uiSPI_Command;	//set the data for sending 
	while(!IFS0bits.SPI1IF);//wait until the SPI writting finished
	DA0_Sel=1; 				//disable DA0, and end DP action
}

void Audio_Receive(void)
{	//Line_in input processing, sample 2 times on one channel and average the result as final
	//AD on Audio_receive is only one byte
	//AD0 initialzation for Line_in input
	int i;

	ADCHS=AD0_Sel;		//AN2 selected for Line_in input
	ADCON1bits.ADON=1;	//turn A/D on
	//
	ADCVal_Linein=0;

	//each time one sampling
	ADCON1bits.SAMP=1;	//manual start, after 4 Tad, the sampling begin
	for(i=0; i<50; i++); //delay for sampling begin
	while(!ADCON1bits.DONE);
	for(i=0; i<50; i++); //delay for sampling begin
	ADCVal_Linein=ADCBUF0;

	//
	Val_Linein=(char)(ADCVal_Linein>>4);//get the final value of line_in from AD module(1 byte)
//	Val_Linein=(char)(ADCBUF0>>4);//get the final value of line_in from AD module(1 byte)
}
//----------------------------------------------------
//U2ART module------------------------------
void Init_U2ART(void)	//init U2ART Tx 
{
	U2MODE=0x0;			//before init or reset, disable the U2ART, clear all bits
	U2BRG=(FCY/16)/U2Baud-1; //for MCU=20MIPs, rate=113KHz, U2BRG=10
//	U2BRG=10;
 	U2STA=0x8000;			//slow interrupt setting, for fast =0x0
//	U2TXCount=0;

	IFS1bits.U2RXIF=0;
	IEC1bits.U2RXIE=0;	//disabled U2ART RX interrupt, by the use of manual check
	U2RXCount=0;

	IFS1bits.U2TXIF=0;
	IEC1bits.U2TXIE=1;	//enable the UART TX interrupt

	//added...........................
	U2STAbits.UTXEN=1;
    U2STAbits.UTXISEL = 1;  //Setup UART1 transmitter to interrupt
                                //when a character is transferred to the
                                //Transmit Shift register and as result,
                                //the transmit buffer becomes empty.
	//end..............................


//	U2MODEbits.LPBACK=1;	//Loop back test, temporarily********************
	U2MODEbits.UARTEN=1;	//enable the U2ART module, 8 bits, no parity, 1 stop bit
}

void U2ART_Send(unsigned int uiSend1, unsigned int uiSend2)
{	//uiSend1 is the first byte to be sent, uiSend2 is the second. They sent together
	//only one byte to send each time strictly.

//	U2STAbits.UTXEN=0;
	uiU2TXBUF[0]=uiSend1;
//	uiU2TXBUF[1]=uiSend2;
//	U2TXCount=0;
	IFS1bits.U2TXIF=0;
	IEC1bits.U2TXIE=1;

	U2STA=0;			//clear the U2 status reg. and reset the interrupt mode

    U2STAbits.UTXISEL = 1;  //Setup UART1 transmitter to interrupt
	U2STAbits.UTXEN=1;

}

void U2ART_Receive(void)	
{	//manual check the U2ART RX regularly, each time one byte receiving
	unsigned int iRead[4]; unsigned int iP=0;
	//
	if(U2STAbits.OERR){
		iErrFlag=1;		//indicate overflow of the RX buffer
	}
	while(U2STAbits.URXDA){
		iRead[iP]=U2RXREG;
		iP++;
	}
	
	U2RXVal_1=iRead[0];
	iP=0;
}
//-----------------------------------------------------------------------------
int A_Parse_Proc(unsigned int iAVal)	//Audio data output judgement, dynamic thresholding method
{	//
	static float f_Val_Sum=0.0;
	static unsigned int iCal_Cnt=0;
	static unsigned int iMax=177; 
	static unsigned int iMin=77;
	static unsigned int iThreshold_Bal=127;	
		
	//get the dynamic threshold balance(every THRESHOLD_BAL_CNT), max, and min limit value
	if(iCal_Cnt>=THRESHOLD_BAL_CNT){
		iCal_Cnt=0;
		iThreshold_Bal=(unsigned int)(f_Val_Sum/(float)THRESHOLD_BAL_CNT);
		iMax=iThreshold_Bal+THRESHOLDDRIFT;
		iMin=iThreshold_Bal-THRESHOLDDRIFT;	
	}else{
		iCal_Cnt++;
		f_Val_Sum+=(float)iAVal;
	}

	//identify if the audio value over the threshold
//	if((iAVal>iMax)||(iAVal<iMin)){
	if((iAVal>THRESHOLD_MAX)||(iAVal<THRESHOLD_MIN)){
		return 1;
	}
	return 0;
}

int Audio_Data_Gen(unsigned int iAFreq)	//for generating the audio data to send with lead Freq.
{	//0---not finished yet; 1---finished
	static unsigned int iLeadCnt=0;	//3.3KHz, lead freq. counter, 4T (8 times)
	static unsigned int iAudioSendCnt=0; //audio data counter
	static unsigned int iAGenFlag=0;
	//
		if(iAudioSendCnt>79){	//send 80 points data freq. 
			iAudioSendCnt=0;
			return 1;
		}else{
			iAudioSendCnt++;
			Out_Freq_Sel=iAFreq;
			Audio_Send();
		}
//	}

	return 0;
}

//----------------------------------------------------------
void Sender_Machine_Init(void)
{
	i_Send_StatusM=0;
	ui_Delay_Cnt=0;
	//
	i_ADC_Status=0;
	ch_Mode=0B10000000;
}

void Receiver_Machine_Init(void)
{
	i_RCV_StatusM=0;
	ui_RCV_Loop_Cnt=0;
	ui_Audio_RCV_Cnt=0;
	ui_RCV_UART_Cnt=0;
	ui_Parse_UART_Cnt=0;
	ui_RCV_FIN_Delay=0;
}

unsigned int ADC_Send_Machine(void)	//A/D data sending by audio encoding from 400Hz(0) and 800Hz(1)
{	//send the A/D value to another machine by cell phone interface
	unsigned char chTmp=0;
	//
	if(i_ADC_Status<7){		//0-6 bit, first byte
		chTmp=ADC_BCD[0]&ch_Mode;
	}else if((i_ADC_Status>6)&&(i_ADC_Status<14)){ //7-13 bit, second byte
		chTmp=ADC_BCD[1]&ch_Mode;
	}else if((i_ADC_Status>13)&&(i_ADC_Status<21)){ //14-20 bit, send the ending codes '1111111'
		chTmp=0x32; 
	}else if(i_ADC_Status>20){ //send the last freq. and finish all the AD audio data sending
		i_AD_Send_Flag=0;
		i_ADC_Status=0;
		ch_Mode=0B10000000;
		
		//back to receiver mode ('CM')
		Audio_Sel=1;
		Receiver_Machine_Init();
		Sender_Machine_Init();

		return 0; 
	}
	//
	ch_Mode=ch_Mode>>1;
	i_ADC_Status++;

	if((i_ADC_Status==7)||(i_ADC_Status==14)){
		ch_Mode=0B10000000;
	}
	//
	if(chTmp){
		return 0x32;	//1, 800Hz
	}else{
		return 0x31;	//0, 400Hz
	}
}

void Sender_Status_Machine(void)//for 'MC' mode main audio transfering process(status machine)
{
	unsigned int uiTmp;

	//Status Machine for sender start from here.............
	switch(i_Send_StatusM){
		case 0:		//start, M0, request command from PC
			//
			i_Send_StatusM=1;
			if(!i_AD_Send_Flag){
				U2ART_Send(0x30+5, 0);
			}
			break;

		case 1:		//M1, communicate with PC
			if(!i_AD_Send_Flag){
				U2ART_Receive();
			}else{
				U2RXVal_1=ADC_Send_Machine();
			}
			//
			if((U2RXVal_1<0x30)||(U2RXVal_1>0x39)){	//non-command, invalid
				i_Send_StatusM=1;
				break;
			}
			//command processing, 0x31:400Hz, 0x32:800Hz, 7:'CM', 8:'MC'
			uiTmp = U2RXVal_1 - 0x30;
			if(uiTmp==4){
				i_Send_StatusM=1;
			}else if((uiTmp>=1)&&(uiTmp<=3)){
				ui_Send_Cmd=uiTmp;
				i_Send_StatusM=2;
			}else if(uiTmp==7){	//'CM' (receiver mode)
				Audio_Sel=1;
				Receiver_Machine_Init();
				Sender_Machine_Init();
				U2ART_Send(0x30+7, 0);
			}else if(uiTmp==8){	//'MC' (sender mode)
				Audio_Sel=0;
				Receiver_Machine_Init();
				Sender_Machine_Init();
			}else{
				i_Send_StatusM=0;
			}	
			break;

		case 2:		//M2, generate audio data and send
			if(Audio_Data_Gen(ui_Send_Cmd)){
				i_Send_StatusM=3;
			}else{
				i_Send_StatusM=2;
			}
			break;

		case 3:		//M5, delay procedure for sending
			if(ui_Delay_Cnt<SENDDELAYLIMIT){
				ui_Delay_Cnt++;
				i_Send_StatusM=3;
				break;
			}else{
				ui_Delay_Cnt=0;	
				i_Send_StatusM=0;		
			}
			break;

		default:	//default, start from begining
			i_Send_StatusM=0;
		
	}
}

void Receiver_Status_Machine(void)//for 'CM' mode main audio receiving process(status machine)
{	//iReset=1, reset machine; =0, execute the machine
	unsigned int uiTmp;
	//
	//Status machine for receiver start from here....
	switch(i_RCV_StatusM){
		case 0:		//M0, start
			Audio_Receive();
			if(A_Parse_Proc(Val_Linein)){
				if(ui_Parse_UART_Cnt>RCVPARSEUARTWAIT){
					i_RCV_StatusM=2;
					ui_Audio_RCV_Cnt=0;
					ui_RCV_Loop_Cnt=0;
					ui_Parse_UART_Cnt=0;
				}else{
					i_RCV_StatusM=0;
					ui_Parse_UART_Cnt++;
				}
			}else{
				//
				if(ui_RCV_Loop_Cnt>RCVPARSEWAIT){
					i_RCV_StatusM=1;
					ui_RCV_Loop_Cnt=0;
				}else{
					i_RCV_StatusM=0;
					ui_RCV_Loop_Cnt++;
				}

				ui_RCV_UART_Cnt++;
			}
			break;
		
		case 1:		//M1, 
			U2ART_Receive();
			uiTmp=U2RXVal_1-0x30;
			if(uiTmp==7){	//change to 'CM' mode
				Audio_Sel=1;
				Receiver_Machine_Init();
				Sender_Machine_Init();
			}else if(uiTmp==8){
				Audio_Sel=0;	//change to 'MC' mode
				Receiver_Machine_Init();
				Sender_Machine_Init();
			}
			i_RCV_StatusM=0;
			break;		

		case 2:		//M2, 
			U2ART_Send(0x30+5, 0);		//report PC that audio data start, ready to transfer
			i_RCV_StatusM=3;
			ui_RCV_UART_Cnt=0;
			break;
			
		case 3:		//M3,
			if(ui_Audio_RCV_Cnt>RCVUARTWAIT){
				ui_Audio_RCV_Cnt=0;
				i_RCV_StatusM=4;
			}else{
				ui_Audio_RCV_Cnt++;

				Audio_Receive();
				if(Val_Linein==52){ 		//'4'
					Val_Linein=51;	
				}else if(Val_Linein==53){	//'5'
					Val_Linein=54;
				}else if(Val_Linein==55){	//'7'
					Val_Linein=54;
				}else if(Val_Linein==56){	//'8'
					Val_Linein=57;
				}
				i_RCV_StatusM=3;
				U2ART_Send(Val_Linein, 0);
				//
				if(ui_RCV_UART_Cnt>RCVUARTCOUNT){
					i_RCV_StatusM=0;
					ui_RCV_UART_Cnt=0;
					ui_Parse_UART_Cnt=0;
				}else{
					ui_RCV_UART_Cnt++;
				}
			}
			break;

		case 4:		//M4,
			U2ART_Receive();
			uiTmp=U2RXVal_1-0x30;
			if(uiTmp==7){	//change to 'CM' mode
				Audio_Sel=1;
				Receiver_Machine_Init();
				Sender_Machine_Init();
			}else if(uiTmp==8){
				Audio_Sel=0;	//change to 'MC' mode
				Receiver_Machine_Init();
				Sender_Machine_Init();
			}else if(uiTmp==4){	//successful analyse from PC, return and restart
				ui_RCV_UART_Cnt=0;
				ui_Parse_UART_Cnt=0;
				i_RCV_StatusM=5;
			}else{
				i_RCV_StatusM=3;
			}
			break;		

		case 5:		//M5,
			if(ui_RCV_FIN_Delay>RCVFINISHDELAY){
				ui_RCV_FIN_Delay=0;	
				i_RCV_StatusM=0;		
			}else{
				ui_RCV_FIN_Delay++;
				i_RCV_StatusM=5;
			}
			break;

		default:	//default, start from begining
			i_RCV_StatusM=0;
	}
}

//----------------------------------------------------------------------------------
int main(void)
{	
	static unsigned int iTmp1=0; 
	static unsigned int iTmp2=0; 
	//
	Init();	//initialize all peripheral and variables
	//
	while (1){
		if(!Flags.Timer1)	
			continue;
		Flags.Timer1=0;

	//Task1: Send out the audio signal or Recieve the audio signal and processing
		//Audio Freq.: 200Hz ->1, 400Hz -> 2, 800Hz -> 3
		if(OneFiftyMicroSecRoutine()){

			//first judge 2 modes, 'CM' or 'MC'; 
			if(Audio_Sel){	//=1, 'CM' mode(Cellphone to PC Comm.)
				//
				Receiver_Status_Machine();

			}else{			//=0, 'MC' mode (PC Comm. to Cell phone)
				//
				Sender_Status_Machine();

			}
		}

//testing........................................
	if(ThreeHundredMicroSecRoutine()){
	}

	if(SixHundredMicroSecRoutine()){
//		Audio_Receive();
//		U2ART_Send(Val_Linein, 0);
	}
//end testing..................................
	
		//Task2: check switch button (1ms)---------------
		if(OneMilliSecRoutine()){
			//T2 actions...
			CheckSwitch();
		}
	//Task3: Update switches status show on LEDs(16ms)
		if(SixteenMilliSecRoutine()){
			//T3 actions...
			SW_Stat_Show();
		}

	//Task4: Update error indicator LED4 status 1(125ms)-------------
		if(OneHundredMilliSecRoutine()){
			//T4 actions...
			if(iErrFlag){
				LED4=!LED4;
			}else{
				LED4=LED4&0;
			}
		}

	//Task5: Update LED status 2(500ms)-------------
		if(HalfSecRoutine()){

		}

	//Task6: Update LED status by variable timing mode....
//		if(VariableTimeRoutine(Val_Linein<<3)){	//for test audio_receive
		if(VariableTimeRoutine(U2RXVal_1<<3)){ //for test loopback of U2ART
//		if(VariableTimeRoutine(Val_AD1<<3)){ //indicate the A/D or D/A module is in use
			//T6 actions...
			if(ADScan_Sel){
				LED3=!LED3;	//flash freq. is controlled by A/D1 module(potentiometer)
			}else{
				LED3=LED3&0;
			}
		}
	}
	//
	return 0;
}	

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Error traps***************************************
void _ISR_OscillatorFail(void)
{	//oscillator fail, most serious, LED4 indicator light up, no flash
	LED4=LED4&1;	
	while(1);
}

void _ISR_AddressError(void)
{	//Addr. Generation error, second most serious, LED4 indicator light up, no flash
	LED4=LED4&1;
	while(1);
}

void _ISR_StackError(void)
{	//Stack error, soft trap, LED4 lightup, flashing
	iErrFlag=1;
}

void _ISR_MathError(void)
{	//Math error, soft trap, LED4 lightup, flashing
	iErrFlag=1;
}
//The End ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	
//================================================================================
//test*************, each pressed, change the audio frequency
//		if(!Audio_Sel){				//check if it is in the audio sending mode

/*			if(++Out_Freq_Sel>3){	//looping three audio freq. sending
				Out_Freq_Sel=1;
			}
			if(++ui_Send_Cmd>3){
				ui_Send_Cmd=1;
			}
*/
/*			if(++ui_Freq_test>0x32){
				ui_Freq_test=0x31;
			}
*/
//			ui_Test_Flag=!ui_Test_Flag;
//			U2ART_Send(ui_Freq_test, 0);//Send Audio_Receive data to PC Comm.		
//		}
//--------------------------------------------------------------------------
//	int usTmp=0x30+Out_Freq_Sel;
//	U2ART_Send(usTmp, 0);//Send Audio_Receive data to PC Comm.		
	//
	//each time two sampling
//	IFS0bits.ADIF=0;
//	ADCON1bits.ASAM=1;
//	while(!IFS0bits.ADIF);	//wait for the interrupt 
//	ADCON1bits.ASAM=0;
//	ADCVal_Linein=ADCBUF0+ADCBUF1;
//	ADCVal_Linein=ADCVal_Linein>>1;	//average the result
	
//----------------------------------------------------------------------

			//test******************************
//			if(ui_Test_Flag){
//				U2RXVal_1=0x32;	//test 800Hz
//				U2RXVal_1=ui_Freq_test;
//				ui_Test_Flag=0;
//			}
			//end test*************************

//--------------------------------------------------------------------
/*
	switch(i_ADC_Status){
		case 0:
			chTmp=ADC_BCD[0]&0B10000000
			break;
		case 1:
			chTmp=ADC_BCD[0]&0B01000000
			break;
		case 2:
			chTmp=ADC_BCD[0]&0B00100000
			break;
		case 3:
			chTmp=ADC_BCD[0]&0B00010000
			break;
		case 4:
			chTmp=ADC_BCD[0]&0B00001000
			break;
		case 5:
			chTmp=ADC_BCD[0]&0B00000100
			break;
		case 6:
			chTmp=ADC_BCD[0]&0B00000010
			break;
		case 7:
			chTmp=ADC_BCD[1]&0B10000000
			break;
		case 8:
			chTmp=ADC_BCD[1]&0B01000000
			break;
		case 9:
			chTmp=ADC_BCD[1]&0B00100000
			break;
		case 10:
			chTmp=ADC_BCD[1]&0B00010000
			break;
		case 11:
			chTmp=ADC_BCD[1]&0B00001000
			break;
		case 12:
			chTmp=ADC_BCD[1]&0B00000100
			break;
		case 13:
			chTmp=ADC_BCD[1]&0B00000010
			break;
			//attach the end symbol '0000000'
		case 14:
			
		case 15:
		case 16:
		case 17:
		case 18
*/
//-----------------------------------------------------------------
/*
int A_Flag_Send(void)		//for sending the 3.3KHz audio flag
{	//0---not finished yet; 1---finished
	static unsigned int ui_SendF_Cnt=0;
	//
	if(ui_SendF_Cnt>AFLAGLIMIT){//larger than 4T to ensure receiving, >9; 5T--good, 27=3*3T,for safe
		ui_SendF_Cnt=0;
		return 1;
	}else{
		ui_SendF_Cnt++;
		Out_Freq_Sel=5;		//pick 3.3Khz
		Audio_Send();
	}
	return 0;
}
*/
//--------------------------------------------------------------
/*	//AD1 - AD4 initialization for AD1 to AD4 input 
//	ADCON1bits.ADON=0;	//
	ADPCFG=0xFEC3;		//AN2,3,4,5,8 analog, others digital
	ADCON1=0x00E0; 		//internal counter start and end sampling
	ADCSSL=0x00;
	ADCON3=0x041e;		//3 or 4 Tad sampling, convert begin(for minTad=667ns,ADC=26, Tad=675ns)
							//or if minTad=770ns, ADC=30, Tad=775ns
	ADCON2=0;			//One channel one sampling
//	ADCON1bits.ADON=1;
*/
//-----------------------------------------------------------------
//	if(!iAGenFlag){
//		if(iLeadCnt<=8){
//			iLeadCnt++;
//			if(iLeadCnt==8){
//				iAGenFlag=1;
//			}
//			Out_Freq_Sel=5;
//			Audio_Send();
//		}
//	}else{
//---------------------------------------------------------------------
//			iAGenFlag=0;
//			iLeadCnt=0;

		//clear the audio index after finish 32 point sending(especially for 600Hz, 24points)
//			index_P=0;
//			index_P2=0;
//--------------------------------------------------------------------
//				Audio_Receive();
//				U2ART_Send(Val_Linein, 0);
//				Out_Freq_Sel=5;
//				Audio_Send();
//---------------------------------------------------------------------
	
					//for testing ***************************
//				Audio_Data_Gen(ui_Send_Cmd);
//				A_Flag_Send();

//				Out_Freq_Sel=1;
//				Audio_Send();

/*			if(iTmp1<SENDDELAYLIMIT){
				iTmp1++;
			}else{
				Audio_Receive();
				U2ART_Send(Val_Linein, 0);
				if(iTmp2<TESTWAIT){
					iTmp2++;
				}else{
					iTmp1=0;
					iTmp2=0;	
				}
			}
*/
				//sample interal potentiometer A/D, give LED3 flashing as a indication
//				ADC_SR(1);		//at least put 1, no more than 4 total channel can be scaned
//--------------------------------------------------------------------------
			//T5 actions...
/*				//test ...Every 0.5s, in the sending mode, the audio freq. output changed
			if(!Audio_Sel){				//check if it is in the audio sending mode
				if(++Out_Freq_Sel>3){	//looping three audio freq. sending
					Out_Freq_Sel=1;
				}
			}
*/
//-----------------------------------------------------------------------------
