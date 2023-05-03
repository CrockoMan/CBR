/*****************************************************
Project : Honda CBR-600 корректор, для отключение отсечки ЭБУ
Version : 
Date    : 
Author  : 
Company : 
Comments: CBR 


Chip type           : ATtiny13
Clock frequency     : 9,600000 MHz             CKSEL0 SUT0 CKDIV8 WDTON BODLEVEL1 - ON !!!
Memory model        : Tiny
External RAM size   : 0
Data Stack size     : 16
*****************************************************/

#include <tiny13.h>
#include <delay.h>

#define SetBit(x)    |= (1<<x) 
#define ClearBit(x)  &=~(1<<x) 
#define InvertBit(x) ^=(1<<x)

#define IsBitSet(port, bit)   ((port)  & (1 << (bit)))
#define IsBitClear(port, bit) (~(port) & (1 << (bit)))

#define INT_ENABLE()    GIMSK = 0b01000000 
#define INT_DISABLE()   GIMSK = 0b01000000 
#define INT_FALLING()   MCUCR = 0b00000010 
#define INT_RISING()    MCUCR = 0b00000011
#define INT_FLAG_EN()   GIFR  = 0b01000000
#define INT_FLAG_DIS()  GIFR  = 0b00000000

#define FALLING_MODE    0
#define RISING_MODE     1

#define MAX_SPEED       39              // Таймер 40,064Гц; работа в штатном режиме до V = 39*40,064 = 156 км/ч

volatile unsigned char Mode =RISING_MODE, nCounter=0, nSpeed=0;

interrupt [EXT_INT0] void ext_int0_isr(void)
{
    INT_DISABLE();
    INT_FLAG_DIS();

    if(Mode == RISING_MODE)    nCounter=nCounter+1; // Расчет скорости

    if( nSpeed<=MAX_SPEED )             // Скорость меньше заданной
    {
        if(Mode == RISING_MODE)         // Пропускаем импульсы напрямую
        {
            PORTB SetBit(0);
            PORTB SetBit(2);
            INT_FALLING();
            Mode = FALLING_MODE;
        }
        else
        {
            PORTB ClearBit(0);
            PORTB ClearBit(2);
            INT_RISING();
            Mode = RISING_MODE;
        }
    }
    else                                // Делим импульсы пополам
    {
        INT_RISING();
        Mode = RISING_MODE;
        PORTB InvertBit(0);
        PORTB InvertBit(2);
    }

    INT_FLAG_EN();
    INT_ENABLE();    
}


interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
//    #asm("wdr");
    nSpeed=nCounter;
    nCounter=0;
}




void main(void)
{
unsigned int i=0;
char cCount[]={0,0};

#pragma optsize-
CLKPR=0x80;
CLKPR=0x00;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif


//PORTB = (1<<1);             // PB1 -> PullUp для кнопки на Int0 
//DDRB  = (1<<0);             // PB0 -> Выход
//PORTB = 0b111110;             // PB1 -> PullUp для кнопки на Int0 и всех, кроме PB0 
PORTB = 0b111000;             // PB1 -> PullUp для кнопки на Int0 и всех, кроме PB0 
DDRB  = 0b000101;             // PB0, PB1 -> Выход, остальные вход

PORTB ClearBit(2);
PORTB ClearBit(0);
for(i=0; i<10; i++)
{
    PORTB InvertBit(2);     // При включении моргнуть 5 раз
    PORTB InvertBit(0);     // При включении моргнуть 5 раз
    delay_ms(50);
    if( IsBitClear(PINB, 1) )        cCount[0]=cCount[0]+1;
    else                             cCount[1]=cCount[1]+1;
    
}
PORTB SetBit(2);

// Watchdog OSC/1024
#pragma optsize-
WDTCR=0x39;
WDTCR=0x29;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

//--------------------------------------------- Проверка на альтернативный режим работы
if(cCount[0]==5 && cCount[1]==5)               // Альтернативный режим
{

#asm("sei")
    while(1)                // Цикл альтернативного режима
    {
        #asm("wdr")
        PORTB InvertBit(0);
        PORTB InvertBit(2);
        delay_us(500);
    }
}
//--------------------------------------------- Конец проверки на альтернативный режим

if( IsBitClear(PINB, 1) )
{
    PORTB ClearBit(0);
    PORTB ClearBit(2);
    Mode = RISING_MODE;
}
else
{
    PORTB SetBit(0);
    PORTB SetBit(2);
    Mode = FALLING_MODE;
}

//TIMER0  40,064Hz (0,2%)
TCCR0B = 0x00; //stop
OCR0A = 0xEA;
OCR0B = 0xEA;
TCNT0 = 0x16; //set count
TCCR0A = 0x00; 
TCCR0B = 0x05; //start timer

//GIMSK = 0b01000000;                 // INT0 enable
//MCUCR = 0b00000011;                 // Прерывание по переднему фронту
//GIFR  = 0b01000000;                 // INT0 flag enable
INT_FLAG_EN();
if(Mode ==RISING_MODE)  INT_RISING();
else                    INT_FALLING();
INT_ENABLE();

TIMSK0 = 0x02;                      //timer interrupt
/*
// Watchdog OSC/1024
#pragma optsize-
WDTCR=0x39;
WDTCR=0x29;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif
*/
#asm("sei")

while (1)
      {
        #asm("wdr");
      };
}
