
//Created by Jery BAranowski and Karol Skorulski

#define F_CPU 16000000L
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define VREF 5.0

/* hardware SPI */
#define MOSI (1<<PB5)	//   <---- A (SER IN)
#define SCK (1<<PB7)	//   <---- SHIFT CLOCK (SC)
#define LT (1<<PB3)		//	 <---- LATCH CLOCK (LT)
#define LT_ON PORTB |= LT
#define LT_OFF PORTB &= ~LT

/*Maszyna Stanu*/
enum Stan {OCZEKIWANIE,DESZCZ, PLASZCZYZNA,WOOPWOOP,HOOPHOOP,WYSWIETLTEKST,GRA};
volatile enum Stan stan;
enum Skret {xp90,xm90,yp90,ym90,zp90,zm90,nic};
volatile enum Skret skret=nic;
enum Polozenie {wWezu,pozaWezem};

/*Predkosc odswie¿ania*/
const int czasWyswietlania=50; 

/*Stan warstwy*/
//kazda tablica reprezentuje warstwe
//ka¿da liczba reprezantuje jeden rejestr
//domyslnie wszystkie zapalone
uint8_t warstwa1[8]={255,255,255,255,255,255,255,255};
uint8_t warstwa2[8]={255,255,255,255,255,255,255,255};
uint8_t warstwa3[8]={255,255,255,255,255,255,255,255};
uint8_t warstwa4[8]={255,255,255,255,255,255,255,255};
uint8_t warstwa5[8]={255,255,255,255,255,255,255,255};
uint8_t warstwa6[8]={255,255,255,255,255,255,255,255};
uint8_t warstwa7[8]={255,255,255,255,255,255,255,255};
uint8_t warstwa8[8]={255,255,255,255,255,255,255,255};

/*Zmienne pomocnicze*/

volatile uint8_t bufor; // bufor s³uzacy do odbierania i nadawania
volatile uint8_t bufor1; //bufor pomocniczy
volatile uint8_t flaga=0; //flaga sygnalizujaca odebranie bajtu
uint8_t counter=0; //licznik pomagajacy prawidlowo zapelniac wartswty
uint8_t numerWarstwy=0;
uint8_t mlodszySter=0; // przechowuje parametr trybu np. liczbe iteracji animacji lub poziom trudnoœci gry snake
uint8_t starszySter=0; //przechowuje rzadany tryb kostki
char buforTekst[255];
char *pBuforTekst;

/*Spis funkcji*/
////Inicjatory////
void InicjujSPI();//inicjuje SPI
void InicjujIO(); // inicjowanie portow IO, Domyslnie ustawia gorna warstwe( warstwa 1) jako swiecaca.
void InicjujUSART(); // inicjacja USART
void InicjujTimer();//ustawia timer 0 i 1



//////Komunikacja i interfejsy/////
void WyslijSPI( uint8_t bajt );//funkcja wysylajaca 8 bitow przez spi
void WyslijSPI64(uint8_t warstwa[]) ;// funkcja wysylajaca 64 bity u¿ywajac w petli Wyslij_SPI. Zapelnia warstwe.
void WyslijUART2(uint8_t liczba);
void WyslijUART3(int liczba );//wysyla znako

uint8_t OdbierzUART (); // odbiera jeden bajt
void WyslijStanKostki();//zwraca tablice danych diód kostki
void SterowanieManualne();

/////Funkcje generujace animacje/////
void Deszcz (int liczba_iteracji);
void Plaszczyzna(int liczba_iteracji);
void Miganie();
void MiganieZTranslatorem(int liczba_iteracji);
void SterowanieManualne (); // funkcja prze³¹czaj¹ce kostke w tryb "nas³uchu: od komputera. Wyœwietli 64 bajty przes³ane przez usart-usb.
void WoopWoop(int liczba_iteracji);
void HoopHoop(int liczba_iteracji);
void WoopSelektor(uint8_t numerWoop, uint8_t stanWoop);
void HoopSelektor(uint8_t numerHoop, uint8_t stanHoop);
void WyswietlZnak(double czasKlatki,char znak);
void Czysc(); //zeruje rejestry wyswietlacza

uint8_t czasWoopWoop=63;
/////Funkcje obs³uguj¹ce animacje////
void ZapalXY(uint8_t z);
void ZapalXZ(uint8_t y);
void ZapalYZ(uint8_t x);
void ZmienDiode(uint8_t x, uint8_t y, uint8_t z,uint8_t stan); //ustawia diode
uint8_t DiodaBinarnie(uint8_t x);////zwraca bajt zapalajacy dan¹ diodê 

/////Zapalanie kostki/////
void WyswietlKostke();//Funkcja kontener. Korzysta z Wyslij_SPI i Wyslij_SPI_64 by wczytac dane do rejestrow, zapala kolejne warstwy w odstepie czasowym zadeklarowanym w stalej publicznej "odstep_czasowy"
void WyswietlKlatke(double czasMs);
void WyswietlTekst(uint8_t iloscZnakow);
////Funkcje gry//////
void GenerujJedzenie(uint8_t waz[512][3], uint8_t jedzenie[3],unsigned int dlugoscWeza);
void MacierzxMacierz(int8_t M1[3][3],int8_t M2[3][3] );
void MinMacierzxMacierz(int8_t M1[3][3],int8_t M2[3][3] );

/////Deklaracja czcionek/////

static const uint8_t ASCIItrans[][8] = {
	{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},//          0 0
	{0x8,0x8,0x8,0x8,0x8,0x0,0x8,0x0},//          1 !
	{0x14,0x14,0x14,0x0,0x0,0x0,0x0,0x0},//       2 "
	{0x14,0x14,0x3e,0x14,0x3e,0x14,0x14,0x0},//   3 #
	{0x8,0x3c,0xa,0x1c,0x28,0x1e,0x8,0x0},//      4 $
	{0x6,0x26,0x10,0x8,0x4,0x32,0x30,0x0},//      5 %
	{0xc,0x12,0xa,0x4,0x2a,0x12,0x2c,0x0},//      6 &
	{0xc,0x8,0x4,0x0,0x0,0x0,0x0,0x0},//          7 '
	{0x10,0x8,0x4,0x4,0x4,0x8,0x10,0x0},//        8 (
	{0x4,0x8,0x10,0x10,0x10,0x8,0x4,0x0},//       9 )
	{0x0,0x8,0x2a,0x1c,0x2a,0x8,0x0,0x0},//      10 *
	{0x0,0x8,0x8,0x3e,0x8,0x8,0x0,0x0},//        11 +
	{0x0,0x0,0x0,0x0,0xc,0x8,0x4,0x0},//         12 ,
	{0x0,0x0,0x0,0x3e,0x0,0x0,0x0,0x0},//        13 -
	{0x0,0x0,0x0,0x0,0x0,0xc,0xc,0x0},//         14 .
	{0x0,0x20,0x10,0x8,0x4,0x2,0x0,0x0},//       15 /
	{0x1c,0x22,0x32,0x2a,0x26,0x22,0x1c,0x0},//  16 0
	{0x8,0xc,0x8,0x8,0x8,0x8,0x1c,0x0},//        17 1
	{0x1c,0x22,0x20,0x10,0x8,0x4,0x3e,0x0},//    18 2
	{0x3e,0x10,0x8,0x10,0x20,0x22,0x1c,0x0},//   19 3
	{0x10,0x18,0x14,0x12,0x3e,0x10,0x10,0x0},//  20 4
	{0x3e,0x2,0x1e,0x20,0x20,0x22,0x1c,0x0},//   21 5
	{0x18,0x4,0x2,0x1e,0x22,0x22,0x1c,0x0},//    22 6
	{0x3e,0x20,0x10,0x8,0x4,0x4,0x4,0x0},//      23 7
	{0x1c,0x22,0x22,0x1c,0x22,0x22,0x1c,0x0},//  24 8
	{0x1c,0x22,0x22,0x3c,0x20,0x10,0xc,0x0},//   25 9
	{0x0,0xc,0xc,0x0,0xc,0xc,0x0,0x0},//         26 :
	{0x0,0xc,0xc,0x0,0xc,0x8,0x4,0x0},//         27 ;
	{0x10,0x8,0x4,0x2,0x4,0x8,0x10,0x0},//       28 <
	{0x0,0x0,0x3e,0x0,0x3e,0x0,0x0,0x0},//       29 =
	{0x4,0x8,0x10,0x20,0x10,0x8,0x4,0x0},//      30 >
	{0x1c,0x22,0x20,0x10,0x8,0x0,0x8,0x0},//     31 ?
	{0x1c,0x22,0x20,0x2c,0x2a,0x2a,0x1c,0x0},//  32 @
	{0x1c,0x22,0x22,0x22,0x3e,0x22,0x22,0x0},//  33 A
	{0x1e,0x22,0x22,0x1e,0x22,0x22,0x1e,0x0},//  34 B
	{0x1c,0x22,0x2,0x2,0x2,0x22,0x1c,0x0},//     35 C
	{0xe,0x12,0x22,0x22,0x22,0x12,0xe,0x0},//    36 D
	{0x3e,0x2,0x2,0x1e,0x2,0x2,0x3e,0x0},//      37 E
	{0x3e,0x2,0x2,0x1e,0x2,0x2,0x2,0x0},//       38 F
	{0x1c,0x22,0x2,0x3a,0x22,0x22,0x3c,0x0},//   39 G
	{0x22,0x22,0x22,0x3e,0x22,0x22,0x22,0x0},//  40 H
	{0x1c,0x8,0x8,0x8,0x8,0x8,0x1c,0x0},//       41 I
	{0x38,0x10,0x10,0x10,0x10,0x12,0xc,0x0},//   42 J
	{0x22,0x12,0xa,0x6,0xa,0x12,0x22,0x0},//     43 K
	{0x2,0x2,0x2,0x2,0x2,0x2,0x3e,0x0},//        44 L
	{0x22,0x36,0x2a,0x2a,0x22,0x22,0x22,0x0},//  45 M
	{0x22,0x22,0x26,0x2a,0x32,0x22,0x22,0x0},//  46 N
	{0x1c,0x22,0x22,0x22,0x22,0x22,0x1c,0x0},//  47 O
	{0x1e,0x22,0x22,0x1e,0x2,0x2,0x2,0x0},//     48 P
	{0x1c,0x22,0x22,0x22,0x2a,0x12,0x2c,0x0},//  49 Q
	{0x1e,0x22,0x22,0x1e,0xa,0x12,0x22,0x0},//   50 R
	{0x3c,0x2,0x2,0x1c,0x20,0x20,0x1e,0x0},//    51 S
	{0x3e,0x8,0x8,0x8,0x8,0x8,0x8,0x0},//        52 T
	{0x22,0x22,0x22,0x22,0x22,0x22,0x1c,0x0},//  53 U
	{0x22,0x22,0x22,0x22,0x22,0x14,0x8,0x0},//   54 V
	{0x22,0x22,0x22,0x2a,0x2a,0x2a,0x14,0x0},//  55 W
	{0x22,0x22,0x14,0x8,0x14,0x22,0x22,0x0},//   56 X
	{0x22,0x22,0x22,0x14,0x8,0x8,0x8,0x0},//     57 Y
	{0x3e,0x20,0x10,0x8,0x4,0x2,0x3e,0x0},//     58 Z
	{0x1c,0x4,0x4,0x4,0x4,0x4,0x1c,0x0},//       59 [
	{0x0,0x2,0x4,0x8,0x10,0x20,0x0,0x0},//       60 '\'
	{0x1c,0x10,0x10,0x10,0x10,0x10,0x1c,0x0},//  61 ]
	{0x8,0x14,0x22,0x0,0x0,0x0,0x0,0x0},//       62 ^
	{0x0,0x0,0x0,0x0,0x0,0x0,0x3e,0x0},//        63 _
	{0x4,0x8,0x10,0x0,0x0,0x0,0x0,0x0},//        64 '
	{0x0,0x0,0x1c,0x20,0x3c,0x22,0x3c,0x0},//    65 a
	{0x2,0x2,0x1a,0x26,0x22,0x22,0x1e,0x0},//    66 b
	{0x0,0x0,0x1c,0x2,0x2,0x22,0x1c,0x0},//      67 c
	{0x20,0x20,0x2c,0x32,0x22,0x22,0x3c,0x0},//  68 d
	{0x0,0x0,0x1c,0x22,0x3e,0x2,0x1c,0x0},//     69 e
	{0x18,0x24,0x4,0xe,0x4,0x4,0x4,0x0},//       70 f
	{0x0,0x3c,0x22,0x22,0x3c,0x20,0x1c,0x0},//   71 g
	{0x2,0x2,0x1a,0x26,0x22,0x22,0x22,0x0},//    72 h
	{0x8,0x0,0xc,0x8,0x8,0x8,0x1c,0x0},//        73 i
	{0x10,0x0,0x18,0x10,0x10,0x12,0xc,0x0},//    74 j
	{0x2,0x2,0x12,0xa,0x6,0xa,0x12,0x0},//       75 k
	{0xc,0x8,0x8,0x8,0x8,0x8,0x1c,0x0},//        76 l
	{0x0,0x0,0x16,0x2a,0x2a,0x22,0x22,0x0},//    77 m
	{0x0,0x0,0x1a,0x26,0x22,0x22,0x22,0x0},//    78 n
	{0x0,0x0,0x1c,0x22,0x22,0x22,0x1c,0x0},//    79 o
	{0x0,0x0,0x1e,0x22,0x1e,0x2,0x2,0x0},//      80 p
	{0x0,0x0,0x2c,0x32,0x3c,0x20,0x20,0x0},//    81 q
	{0x0,0x0,0x1a,0x26,0x2,0x2,0x2,0x0},//       82 r
	{0x0,0x0,0x1c,0x2,0x1c,0x20,0x1e,0x0},//     83 s
	{0x4,0x4,0xe,0x4,0x4,0x24,0x18,0x0},//       84 t
	{0x0,0x0,0x22,0x22,0x22,0x32,0x2c,0x0},//    85 u
	{0x0,0x0,0x22,0x22,0x22,0x14,0x8,0x0},//     86 v
	{0x0,0x0,0x22,0x22,0x2a,0x2a,0x14,0x0},//    87 w
	{0x0,0x0,0x22,0x14,0x8,0x14,0x22,0x0},//     88 x
	{0x0,0x0,0x22,0x22,0x3c,0x20,0x1c,0x0},//    89 y
	{0x0,0x0,0x3e,0x10,0x8,0x4,0x3e,0x0},//      90 z
	{0x10,0x8,0x8,0x4,0x8,0x8,0x10,0x0},//       91 {
	{0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x0},//         92 |
	{0x4,0x8,0x8,0x10,0x8,0x8,0x4,0x0},//        93 }
	{0x0,0x0,0x0,0x2c,0x12,0x0,0x0,0x0}//        94 ~
	};


int main( void )
{
	
	sei();
	InicjujUSART();
	InicjujSPI();
	InicjujIO();
	InicjujTimer();
	Czysc();

uint8_t liczbaZnakow=3;

buforTekst[0]=43;
buforTekst[1]=46;
buforTekst[2]=50;

	while(1)
	{
		
	switch(stan)
	{
	case OCZEKIWANIE:
	Miganie();
	break;
	
	case DESZCZ:
	Deszcz(60);
	break;
	
	case PLASZCZYZNA:
	Plaszczyzna(60);
	break;
	
	case WOOPWOOP:
	WoopWoop(60);
	break;
	
	case HOOPHOOP:
	HoopHoop(60);
	break;
	
	case WYSWIETLTEKST:
	WyswietlTekst(liczbaZnakow);
	break;
	
	case GRA:
	Snake();
	break;
	
	default:
	stan=OCZEKIWANIE;
	break;
	}
}
}

void WyslijSPI( uint8_t bajt )
{
	SPDR = bajt;
	while( !(SPSR & (1<<SPIF)) );
	
}
void InicjujSPI(void) {
	//zadeklarowanie portów spi jako wyjœæ
	DDRB|=_BV(PB4);
	PORTB|=_BV(PB4);
	DDRB |= (MOSI|SCK|LT);
	SPCR |= (1<<SPE)|(1<<MSTR); // w³¹cz SPI i ustaw Master
	SPCR|=_BV(SPR0); //preskaler 4,
}
void WyslijSPI64( uint8_t warstwa[] )
{
	for (int i=0;i<8;i++)
	{
		WyslijSPI(warstwa[i]);
	}

	LT_ON;// zbocze narastajace nakazuje rejestrom ustawiæ rz¹dane napiêcia na nó¿kach
	LT_OFF;
	
}
void InicjujIO()
{
	//Wyjœcia do tranzystorów npn
	DDRC=0b11111111;
	PORTC&=~(_BV(PC0)|_BV(PC1)|_BV(PC2)|_BV(PC3)|_BV(PC4)|_BV(PC5)|_BV(PC6)|_BV(PC7));
	//Wyjœcia do diód
	DDRD|=_BV(PD6)|_BV(PD7);
	//PORTD|=_BV(PD6);
	
}
void WyswietlKostke()
{
	PORTC&=~_BV(PC7);
	WyslijSPI64(warstwa1);
	PORTC|=_BV(PC0);
	_delay_us(czasWyswietlania);
	
	PORTC&=~_BV(PC0);
	WyslijSPI64(warstwa2);
	PORTC|=_BV(PC1);
	_delay_us(czasWyswietlania);
	
	PORTC&=~_BV(PC1);
	WyslijSPI64(warstwa3);
	PORTC|=_BV(PC2);
	_delay_us(czasWyswietlania);
	
	PORTC&=~_BV(PC2);
	WyslijSPI64(warstwa4);
	PORTC|=_BV(PC3);
	_delay_us(czasWyswietlania);
	
	PORTC&=~_BV(PC3);
	WyslijSPI64(warstwa5);
	PORTC|=_BV(PC4);
	_delay_us(czasWyswietlania);
	
	PORTC&=~_BV(PC4);
	WyslijSPI64(warstwa6);
	PORTC|=_BV(PC5);
	_delay_us(czasWyswietlania);
	
	PORTC&=~_BV(PC5);
	WyslijSPI64(warstwa7);
	PORTC|=_BV(PC6);
	_delay_us(czasWyswietlania);
	
	PORTC&=~_BV(PC6);
	WyslijSPI64(warstwa8);
	PORTC|=_BV(PC7);
	_delay_us(czasWyswietlania);
}
void WyswietlKlatke(double czasMs)
{
	
	int x=(int)(czasMs/0.064);
	TCNT1H=0;
	TCNT1L=0;
	while(TCNT1<x)
	{
		WyswietlKostke();
	}

}
void InicjujUSART(void)
{

	
	#define BAUD 57600      //tutaj podaj ¿¹dan¹ prêdkoœæ transmisji
	#include <util/setbaud.h> //linkowanie tego pliku musi byæ
	//po zdefiniowaniu BAUD
	
	//ustaw obliczone przez makro wartoœci
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	#if USE_2X
	UCSRA |=  (1<<U2X);
	#else
	UCSRA &= ~(1<<U2X);
	#endif
	
	
	//Ustawiamy pozosta³e parametry modu³ USART
	UCSRC |= (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);  //bitów danych: 8
	//bity stopu:  1
	//parzystoœæ:  brak
	//w³¹cz nadajnik i odbiornik
	
	UCSRB |= (1<<TXEN) | (1<<RXEN)|(1<<RXCIE);
}
void wyslijUART2(uint8_t liczba)
{
	
	while(!(UCSRA & _BV(UDRE))); //wysyla
	UDR=liczba;
	bufor=0;

}
void WyslijUART3(int liczba)
{
	char *buforPointer;
	char buforfunkcji[8];
	for(uint8_t i=0;i<8;i++)
	{
		buforfunkcji[i]=0;
	}
	
	
	buforPointer=buforfunkcji;
	
	sprintf(buforPointer,"%d",liczba);
	strcat(buforPointer,"\n");
	strcat(buforPointer,"\r");
	for(uint8_t i=0;i<7;i++)
	{	
	while(!(UCSRA & _BV(UDRE))); //wysyla
	UDR=buforfunkcji[i];
	}

}
ISR(USART_RXC_vect)
{
	//przerwanie generowane po odebraniu bajtu
	bufor = UDR;   //zapamiêtaj odebran¹ liczbê
	switch(bufor)
	{
		case 1:
		stan=OCZEKIWANIE;
		break;
		
		case 2:
		stan=DESZCZ;
		break;
		
		case 3:
		stan=PLASZCZYZNA;
		break;
		
		case 4:
		stan=WOOPWOOP;
		break;
		
		case 5:
		stan=HOOPHOOP;
		break;
		
		case 6:
		stan=WYSWIETLTEKST;
		break;
		
		case 7:
		stan=GRA;
		break;
		
		case 88:
		skret=yp90;
		break;
		
		case 22:
		skret=ym90;
		break;
		
		case 44:
		skret=zp90;
		break;
		
		
		case 66:
		skret=zm90;
		break;
		
		default:
		stan=OCZEKIWANIE;
		break;
	}
	
	bufor1=bufor;
	flaga++;
}
void InicjujTimer()
{
	TIMSK|=(_BV(TOIE1)|_BV(TOIE0));// uruchomienie timera 1 i timera 0
	TCCR1B=(_BV(CS12)|_BV(CS10));// presacaler chyyyba 1024
	TCCR0=(_BV(CS02)|_BV(CS10)); //prescaler 1024

}
ISR(TIMER1_OVF_vect)
{
	PORTD=PORTD^_BV(PD7);
}
ISR(TIMER0_OVF_vect)
{
	PORTD=PORTD^_BV(PD6);
	//Heartbeat
	
}
void WyslijStanKostki()
{
	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa1[i]);
	}
	
	
	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa2[i]);
	}

	
	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa3[i]);
	}

	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa4[i]);
	}
	
	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa5[i]);
	}
	
	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa6[i]);
	}

	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa7[i]);
	}
	
	for(int i=0;i <8;i++)
	{
		WyslijUART3(warstwa8[i]);
	}
	
	
	
}
void Miganie()
{
	while(stan==OCZEKIWANIE)
	{
		
		for(int i=0;i<8;i++)
		{
			warstwa1[i]=0;
			warstwa2[i]=0;
			warstwa3[i]=0;
			warstwa4[i]=0;
			warstwa5[i]=0;
			warstwa6[i]=0;
			warstwa7[i]=0;
			warstwa8[i]=0;
		}
		WyswietlKlatke(500);
		
		for(int i=0;i<8;i++)
		{
			warstwa1[i]=255;
			warstwa2[i]=255;
			warstwa3[i]=255;
			warstwa4[i]=255;
			warstwa5[i]=255;
			warstwa6[i]=255;
			warstwa7[i]=255;
			warstwa8[i]=255;
		}
		WyswietlKlatke(500);
	}
}
void MiganieZTranslatorem(int liczba_iteracji)
{
	for(int i=0;i<liczba_iteracji;i++)
	{
		for (int i=0; i<8;i++)
		{
			for (int j=0;j<8;j++)
			{
				for(int k=0;k<8;k++)
				{
					ZmienDiode(i,j,k,0);
				}
			}
		}
		WyswietlKlatke(250);
		
		for (int i=0; i<8;i++)
		{
			for (int j=0;j<8;j++)
			{
				for(int k=0;k<8;k++)
				{
					ZmienDiode(i,j,k,1);
				}
			}
		}
		WyswietlKlatke(250);
	}
}
void ZmienDiode(uint8_t x, uint8_t y, uint8_t z,uint8_t stan)
{	
	if(stan==1)
	{
	switch(z)
	{
	case 0:
	warstwa1[y]|=DiodaBinarnie(x);
	break;
	case 1:
	warstwa2[y]|=DiodaBinarnie(x);
	break;
	case 2:
	warstwa3[y]|=DiodaBinarnie(x);
	break;
	case 3:
	warstwa4[y]|=DiodaBinarnie(x);
	break;
	case 4:
	warstwa5[y]|=DiodaBinarnie(x);
	break;
	case 5:
	warstwa6[y]|=DiodaBinarnie(x);
	break;
	case 6:
	warstwa7[y]|=DiodaBinarnie(x);
	break;
	case 7:
	warstwa8[y]|=DiodaBinarnie(x);
	break;
	}
	}
	if(stan==0)
	{
		switch(z)
		{
		case 0:
		warstwa1[y]&=~DiodaBinarnie( x);
		break;
		case 1:
		warstwa2[y]&=~DiodaBinarnie( x);
		break;
		case 2:
		warstwa3[y]&=~DiodaBinarnie( x);
		break;
		case 3:
		warstwa4[y]&=~DiodaBinarnie( x);
		break;
		case 4:
		warstwa5[y]&=~DiodaBinarnie( x);
		break;
		case 5:
		warstwa6[y]&=~DiodaBinarnie( x);
		break;
		case 6:
		warstwa7[y]&=~DiodaBinarnie( x);
		break;
		case 7:
		warstwa8[y]&=~DiodaBinarnie( x);
		break;
		}
	}
}
uint8_t DiodaBinarnie(uint8_t x)
{
	
	
	switch(x)
	{
	case 0:
	return 0b00000001;
	case 1:
	return 0b00000010;
	case 2:
	return 0b00000100;
	case 3:
	return 0b00001000;
	case 4:
	return 0b00010000;
	case 5:
	return 0b00100000;
	case 6:
	return 0b01000000;
	case 7:
	return 0b10000000;
	default:
	return 0b00000000;
	}
	
}
void Deszcz (int liczba_iteracji)
{
	int x;
	int y;
	srand(TCNT0);//aktualny stan timera
	Czysc();
	
	for(int k=0;k<liczba_iteracji;k++)
	{
		if(stan!=DESZCZ) break;
		x=rand()%7;
		y=rand()%7;
		ZmienDiode(x,y,7,1);
		WyswietlKlatke(100);
		for(int i=0;i<8;i++)
		{
			if(stan!=DESZCZ) break;
			warstwa1[i]=warstwa2[i];
			warstwa2[i]=warstwa3[i];
			warstwa3[i]=warstwa4[i];
			warstwa4[i]=warstwa5[i];
			warstwa5[i]=warstwa6[i];
			warstwa6[i]=warstwa7[i];
			warstwa7[i]=warstwa8[i];
			
		}
		ZmienDiode(x,y,7,0);
	}
}
void Plaszczyzna(int liczba_iteracji)
{
	Czysc();
	
	for(int i=0;i<liczba_iteracji;i++)
	{
		if(stan!=PLASZCZYZNA) break;
		//Plaszczyzna XY
		for(uint8_t z=0;z<8;z++)//zmien wysokosc
		{
			if(stan!=PLASZCZYZNA) break;
			ZapalXY( z);
		}
		for(uint8_t z=0;z<8;z++)//zmien wysokosc
		{
			if(stan!=PLASZCZYZNA) break;
			ZapalXY(7- z);
		}
		//Plaszczyzna XZ
		for(uint8_t y=0;y<8;y++)//zmien wysokosc
		{
			if(stan!=PLASZCZYZNA) break;
			ZapalXZ( y);
		}
		for(uint8_t y=0;y<8;y++)//zmien wysokosc
		{
			if(stan!=PLASZCZYZNA) break;
			ZapalXZ(7- y);
		}
		//Plaszczyzna YZ
		for(uint8_t x=0;x<8;x++)//zmien wysokosc
		{
			if(stan!=PLASZCZYZNA) break;
			ZapalYZ( x);
		}
		for(uint8_t x=0;x<8;x++)//zmien wysokosc
		{
			if(stan!=PLASZCZYZNA) break;
			ZapalYZ(7- x);
		}
		
	}
	
}
void ZapalXY(uint8_t z)
{
	for(uint8_t x=0;x<8;x++)//zapal plaszczyzne xy
	{
		for(uint8_t y=0 ;y<8 ;y++)
		{
			ZmienDiode(x,y,z,1);
		}
	}
	WyswietlKlatke(60);
	for(uint8_t x=0;x<8;x++)//zgas plaszczyzne xy
	{
		for(uint8_t y=0;y<8;y++)
		{
			ZmienDiode(x,y,z,0);
		}
	}
}
void ZapalXZ(uint8_t y)
{
	for(uint8_t x=0;x<8;x++)//zapal plaszczyzne xz
	{
		for(uint8_t z=0;z<8;z++)
		{
			ZmienDiode(x,y,z,1);
		}
	}
	WyswietlKlatke(60);
	for(uint8_t x=0;x<8;x++)//zgas plaszczyzne xz
	{
		for(uint8_t z=0;z<8;z++)
		{
			ZmienDiode(x,y,z,0);
		}
	}
}
void ZapalYZ(uint8_t x)
{
	for(uint8_t y=0;y<8;y++)//zapal plaszczyzne yz
	{
		for(uint8_t z=0;z<8;z++)
		{
			ZmienDiode(x,y,z,1);
		}
	}
	WyswietlKlatke(60);
	for(uint8_t y=0;y<8;y++)//zgas plaszczyzne yz
	{
		for(uint8_t z=0;z<8;z++)
		{
			ZmienDiode(x,y,z,0);
		}
	}
}
void WoopWoop(int liczba_iteracji)
{
	Czysc();
	srand(TCNT0);
	uint8_t numerWoop1=0;
	uint8_t numerWoop2=0;
	for(int i=0;i<=liczba_iteracji;i++)
	{
		if(stan!=WOOPWOOP) break;
		 numerWoop1=rand()%8;
		numerWoop2=rand()%8;
		WoopSelektor(numerWoop1,1);
		WoopSelektor(numerWoop2,0);
	}
	
	
}
void WoopSelektor(uint8_t numerWoop, uint8_t stanWoop)
{
	switch(numerWoop)
	{
	case 0:
	//rozpalanie od  0,0,0 do 7,7,7
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 1:
	//rozpalanie od  7,7,7 do 0,0,0
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=7 ;x>=8-n;x--)
		{
			for(int8_t y=7 ;y>=8-n;y--)
			{
				for(int8_t z=7 ;z>=8-n;z--)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	

	case 2:
	//rozpalanie od  0,7,0 do 7,0,7
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=7 ;y>=8-n;y--)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 3:
	//rozpalanie od  7,0,7 do 0,7,0
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=7 ;x>=8-n;x--)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=7 ;z>=8-n;z--)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 4:
	//rozpalanie od  7,0,0 do 0,7,7
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=7 ;y>=8-n;y--)
			{
				for(int8_t z=7 ;z>=8-n;z--)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 5:
	//rozpalanie od  0,7,7 do 7,0,0
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=7 ;x>=8-n;x--)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 6:
	//rozpalanie od  7,7,0 do 0,07
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=7 ;x>=8-n;x--)
		{
			for(int8_t y=7 ;y>=8-n;y--)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 7:
	//rozpalanie od  0,0,7 do 7,7,0
	for(int8_t n=1;n<=8;n++)
	{
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=7 ;z>=8-n;z--)
				{
					ZmienDiode(x,y,z,stanWoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	}
}
void HoopHoop(int liczba_iteracji)
{
	Czysc();
	srand(TCNT0);
	uint8_t numerWoop1=0;
	uint8_t numerWoop2=0;
	for(int i=0;i<=liczba_iteracji;i++)
	{
		if(stan!=HOOPHOOP) break;
		numerWoop1=rand()%8;
		numerWoop2=rand()%8;
		WoopSelektor(numerWoop1,1);
		HoopSelektor(numerWoop2,1);
		Czysc();
		WyswietlKlatke(czasWoopWoop);
	}
	
	
}
void HoopSelektor(uint8_t numerHoop, uint8_t stanHoop)
{
	
	switch(numerHoop)
	{
	case 0:
	//gaszenie  0,0,0 do 7,7,7
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 1:
	//gaszenie od  7,7,7 do 0,0,0
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		for(int8_t x=7 ;x>7-n;x--)
		{
			for(int8_t y=7 ;y>7-n;y--)
			{
				for(int8_t z=7 ;z>7-n;z--)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	

	case 2:
	//rozpalanie od  0,7,0 do 7,0,7
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=7 ;y>7-n;y--)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 3:
	//rozpalanie od  7,0,7 do 0,7,0
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		for(int8_t x=7 ;x>7-n;x--)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=7 ;z>7-n;z--)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 4:
	//rozpalanie od  7,0,0 do 0,7,7
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=7 ;y>7-n;y--)
			{
				for(int8_t z=7 ;z>7-n;z--)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 5:
	//rozpalanie od  0,7,7 do 7,0,0
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		for(int8_t x=7 ;x>7-n;x--)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 6:
	//rozpalanie od  7,7,0 do 0,07
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		for(int8_t x=7 ;x>7-n;x--)
		{
			for(int8_t y=7 ;y>7-n;y--)
			{
				for(int8_t z=0;z<n;z++)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
	
	case 7:
	//rozpalanie od  0,0,7 do 7,7,0
	for(int8_t n=8;n>=1;n--)
	{
		Czysc();
		for(int8_t x=0;x<n;x++)
		{
			for(int8_t y=0;y<n;y++)
			{
				for(int8_t z=7 ;z>7-n;z--)
				{
					ZmienDiode(x,y,z,stanHoop);
				}
			}
		}
		WyswietlKlatke(czasWoopWoop);
	}
	break;
}
	
}
void Czysc()
{
	for(uint8_t i=0;i<8;i++)
	{
		warstwa1[i]=0;
		warstwa2[i]=0;
		warstwa3[i]=0;
		warstwa4[i]=0;
		warstwa5[i]=0;
		warstwa6[i]=0;
		warstwa7[i]=0;
		warstwa8[i]=0;
	}
}

void WyswietlTekst(uint8_t iloscZnakow)
{
	double czasKlatki=100;
	for(uint8_t i=0; i<iloscZnakow;i++)
	{
		if(stan!=WYSWIETLTEKST) break;
		WyswietlZnak(czasKlatki,buforTekst[i]);
	}
}

void WyswietlZnak(double czasKlatki,char znak)
{
	
	for(uint8_t i=0;i<12;i++)
	{
	if(stan!=WYSWIETLTEKST) break;
		
		Czysc();
		if(i<8)
		{
			
			Laduj(i,znak);
			
		}
		if(i>0 && i<9)
		{
			Laduj(i-1,znak);
		}
		if(i>1 && i<10)
		{
			Laduj(i-2,znak);
		}
		WyswietlKlatke(czasKlatki);
	}
}
void Laduj(uint8_t n, char znak)
{
	unsigned char out = znak & 1;
	if(n<4)
	{
	warstwa8[n]=(uint8_t)ASCIItrans[znak][0];
	warstwa7[n]=(uint8_t)ASCIItrans[znak][1];
	warstwa6[n]=(uint8_t)ASCIItrans[znak][2];
	warstwa5[n]=(uint8_t)ASCIItrans[znak][3];
	
	warstwa4[n]=(uint8_t)ASCIItrans[znak][4];
	warstwa3[n]=(uint8_t)ASCIItrans[znak][5];
	warstwa2[n]=(uint8_t)ASCIItrans[znak][6];
	warstwa1[n]=(uint8_t)ASCIItrans[znak][7];
	}
	else
	{
		
		warstwa8[n]=(uint8_t)ASCIItrans[znak][0];
		warstwa7[n]=(uint8_t)ASCIItrans[znak][1];
		warstwa6[n]=(uint8_t)ASCIItrans[znak][2];
		warstwa5[n]=(uint8_t)ASCIItrans[znak][3];
		
		warstwa4[n]=(uint8_t)ASCIItrans[znak][4];
		warstwa3[n]=(uint8_t)ASCIItrans[znak][5];
		warstwa2[n]=(uint8_t)ASCIItrans[znak][6];
		warstwa1[n]=(uint8_t)ASCIItrans[znak][7];
	}
}

void Snake()
{
	srand(TCNT0);
	unsigned int dlugoscWeza = 1;
	uint8_t waz[512][3];
	
	int8_t jedzenie[3];
	int8_t glowaPom[3];
	int8_t uGlowy[3][3]={{1,0,0},{0,1,0},{0,0,1}};

	//inicjuj polozenie glowy
	for(uint8_t i=0;i<3;i++)
	{
		waz[0][i]=4;
	}

	//inicjuj geometrie

	int8_t Ry[3][3]={{0,0,1},{0,1,0},{-1,0,0}};
	int8_t Rz[3][3]={{0,-1,0},{1,0,0},{0,0,1}};
	enum Polozenie polozenie=wWezu;
				
	//petla gry
	while(stan==GRA)
	{
		//laduj weza na kostke
		Czysc();
		for(unsigned int i=0;i<dlugoscWeza;i++)
		{
			ZmienDiode(waz[i][0],waz[i][1],waz[i][2],1);
		}
	
		//Genruj jedzenie
	

	while(polozenie==wWezu)
	{
		jedzenie[0]=rand()%7;
		jedzenie[1]=rand()%7;
		jedzenie[2]=rand()%7;
		
		for(unsigned int k=0;k<dlugoscWeza;k++)
		{
			
			if((jedzenie[0]==waz[k][0])&&(jedzenie[1]==waz[k][1])&&(jedzenie[2]==waz[k][2]))
			{
				polozenie=wWezu;
				break;
				
			}
			else
			{
				polozenie=pozaWezem;
			}
			
		}
	
	}
		
	ZmienDiode(jedzenie[0],jedzenie[1],jedzenie[2],1);
		
		//wyswietle weza i jedznie
		WyswietlKlatke(1000);
		
		//wykonaj skret
		switch(skret)
		{
			
			case yp90:
			MacierzxMacierz(Ry,uGlowy);
			skret=nic;
			break;
			
			case ym90:
			MinMacierzxMacierz(Ry,uGlowy);
			skret=nic;
			break;
			
			case zp90:
			MacierzxMacierz(Rz,uGlowy);
			skret=nic;
			break;
			
			case zm90:
			MinMacierzxMacierz(Rz,uGlowy);
			skret=nic;
			break;
			
	
			default:
			break;
		}
		
		//przesun glowe 
		for(uint8_t i=0;i<3;i++)
		{
		glowaPom[i]=waz[0][i]+uGlowy[i][0];
		if(glowaPom[i]>7) glowaPom[i]=0;
		if(glowaPom[i]<0) glowaPom[i]=7;
		}
		
		//sprawdz czy zjadl
		if(jedzenie[0]==glowaPom[0]&&jedzenie[1]==glowaPom[1]&&jedzenie[2]==glowaPom[2])
		{
			polozenie=wWezu;
			dlugoscWeza++;		
		}
		else
		{
			polozenie=pozaWezem;
		}
		
		//sprawdŸ czy nie zjad³ siebie
		for(int i=1;i<dlugoscWeza;i++)
		{
			if(waz[i][0]==glowaPom[0]&&waz[i]==glowaPom[1]&&waz[i]==glowaPom[2])
			{
				stan=OCZEKIWANIE;
			}
		}
		
		
		//przesun cialo -zacznij od drugiego elementu bo g³owa oddzielnie
		for(int i=1;i<dlugoscWeza;i++)
		{
			for(int j=0;j<3;j++)
			{
				waz[i][j]=waz[i-1][j];
			}
		}
		
		for(int j=0;j<3;j++)
		{
			waz[0][j]=glowaPom[j];
		}
		
			
	}
	
}

void GenerujJedzenie(uint8_t waz[512][3], uint8_t jedzenie[3],unsigned int dlugoscWeza)
{
	//generuj jedzenie
	enum Polozenie polozenie =wWezu;
	while(polozenie==wWezu)
	{
		jedzenie[0]=rand()%7;
		jedzenie[1]=rand()%7;
		jedzenie[2]=rand()%7;
		for(unsigned int k=0;k<dlugoscWeza;k++)
		{
			
			if(jedzenie[0]==waz[k][0]&&jedzenie[1]==waz[k][1]&&jedzenie[2]==waz[k][2])
			{
				stan=wWezu;
				break;
			}
			else
			{
				stan=pozaWezem;
			}
			
		}
	}
	ZmienDiode(jedzenie[0],jedzenie[1],jedzenie[2],1);
}
void MacierzxMacierz(int8_t M1[3][3],int8_t M2[3][3] )
{
	int8_t tabC[3][3];
	for(int i=0; i<3; i++)
	{
		for(int j=0; j<3; j++)
		{
			tabC[i][j]=0;
			for(int k=0; k<3; k++)
			{
				tabC[i][j] += M1[i][k] * M2[k][j];
			}
		}
	}	
	for(int i=0;i<3;i++)
	{
		for(int j=0;j<3;j++)
		{
			M2[i][j]=tabC[i][j];
		}
	}
	
}
void MinMacierzxMacierz(int8_t M1[3][3],int8_t M2[3][3] )
{
	int8_t tabC[3][3];
	for(int i=0; i<3; i++)
	{
		for(int j=0; j<3; j++)
		{
			tabC[i][j]=0;
			for(int k=0; k<3; k++)
			{
				tabC[i][j] += M1[i][k] * M2[k][j];
			}
		}
	}
	for(int i=0;i<3;i++)
	{
		for(int j=0;j<3;j++)
		{
			M2[i][j]=-tabC[i][j];
		}
	}
	
}