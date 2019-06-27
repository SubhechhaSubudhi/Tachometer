
#include <avr/io.h>
#include <string.h>
#include<avr/interrupt.h>

unsigned long int currtime; unsigned long int prevtime = 0;
unsigned int long millis_count = 0;
int c = 0;
unsigned int rpm;
unsigned int reqrpm = 100;
float x = 255;
int ctt = 0, cc = 0, rev = 0, dif, prevdif = reqrpm;

#define E   PC5
#define RS   PC4


void display(char string[16], char LineNo);
void displaybyte(char D);
void dispinit(void);
void epulse(void);
void delay_ms(unsigned int de);

//=================================================================
//        Main Function
//=================================================================
int main(void)
{
  Serial.begin(9600);
  DDRB |= (1 << PB5) | (1 << PB1) | (1 << PB2); //for motors and led

  TCCR1A |= (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
  TCCR1B |= (1 << CS10) | (1 << WGM12);

  DDRD &= ~(1 << PD2); //INPUT FOR INTO
  PORTB |= (1 << PB5);
  TCCR0A = (1 << COM0A1)|(WGM01<<1); //CTC MODE
  TCCR0B = (1 << CS01) | (1 << CS00); //PRESCALER 64
  OCR0A = 249;
  TIMSK0 |= (1 << OCF0A); //INTERRUPT ON REACHING THE OCR0A VALUE
  sei();
  EIMSK |= (1 << INT0); //enablling the ext interrupt in pin d2
  EICRA = (1 << ISC01); //FALLING EDGE
  sei();


  DDRC = 0x3F;  //Set LCD Port Direction

  delay_ms(500);  //Initiaize LCD
  dispinit();
  delay_ms(200);
  lcdclear();

  display("required rpm", 1);
  display("TACHOMETER", 2);
  delay_ms(2000);

  lcdclear();
  display("RPM=", 1);

  OCR1A = 255;
  PORTB &= ~(1 << PB2);
  _delay_ms(1000);


  while (1);
}

//=================================================================
//        LCD Display Initialization Function
//=================================================================
void dispinit(void)
{
  int count;
  char init[] = {0x43, 0x03, 0x03, 0x02, 0x28, 0x01, 0x0C, 0x06, 0x02, 0x02};

  PORTC &= ~(1 << RS);         // RS=0
  for (count = 0; count <= 9; count++)
  {
    displaybyte(init[count]);
  }
  PORTC |= 1 << RS;  //RS=1
}


//=================================================================
//        Enable Pulse Function
//=================================================================
void epulse(void)
{
  PORTC |= 1 << E;
  delay_ms(1); //Adjust delay if required
  PORTC &= ~(1 << E);
  delay_ms(1); //Adjust delay if required
}


//=================================================================
//        Send Single Byte to LCD Display Function
//=================================================================
void displaybyte(char D)
{
  //data is in Temp Register
  char K1;
  K1 = D;
  K1 = K1 & 0xF0;
  K1 = K1 >> 4; //Send MSB

  PORTC &= 0xF0;
  PORTC |= (K1 & 0x0F);
  epulse();

  K1 = D;
  K1 = K1 & 0x0F; //Send LSB
  PORTC &= 0xF0;
  PORTC |= K1;
  epulse();
}

//=================================================================
//        Display Line on LCD at desired location Function
//=================================================================
void display(char string[16], char LineNo)
{
  int len, count;

  PORTC &= ~(1 << RS);         // RS=0 Command Mode

  if (LineNo == 1)
  {
    displaybyte(0x80);   //Move Coursor to Line 1
  }
  else
  {
    displaybyte(0xC0);   //Move Coursor to Line 2
  }
  PORTC |= (1 << RS);         // RS=1 Data Mode

  len = strlen(string);
  for (count = 0; count < len; count++)
  {
    displaybyte(string[count]);
  }
}


//=================================================================
//        Delay Function
//=================================================================
void delay_ms(unsigned int de)
{
  unsigned int rr, rr1;
  for (rr = 0; rr < de; rr++)
  {

    for (rr1 = 0; rr1 < 30; rr1++) //395
    {
      asm("nop");
    }

  }
}
void printrpm(int rpm)
{
  char c[16];
  itoa(rpm, c, 10);
  display(c, 2);
}


void lcdclear()
{
  PORTC &= ~1 << RS;
  epulse();
  displaybyte(0x10);
  PORTC |= 1 << RS;
  epulse();
  _delay_ms(10);
}
ISR(TIMER0_COMPA_vect)
{
  millis_count++;
  if ((millis_count - ctt == 2000))
  {

    if ((rev - cc) == 0)
    {
      lcdclear();
      rpm = 0;
      display("RPM=", 1);
      printrpm(0);
      dif = rpm - reqrpm;
      increase(dif);
    }
    ctt = millis_count;


    rev = 0;
  }

}
ISR(INT0_vect)//whenever  there is a falling edge this will be performed
{
  rev++;
  if (c == 0)
  {
    prevtime = millis_count;
    //Serial.print("currtime=");
    //Serial.println(currtime);
  }
  c++;
  if (c == 2)
  {
    currtime = millis_count;
    int time = currtime - prevtime;


    int rpm = 1 * (60000 / (time));

    lcdclear();

    printrpm(rpm);
    dif = reqrpm - rpm;
    float proportional = 0.5 * dif;
    float integral = (prevdif + dif) * 0.9;
    float differential = (dif - prevdif) * 0.3;
    Serial.println(proportional) ;
    Serial.print(" ");
    Serial.print(integral);
    Serial.print(" "); 
    Serial.print(differential);
    Serial.print(" "); 
    Serial.print(x - proportional + integral + differential);  
    if (((x - proportional + integral + differential) < 255) && ((x - proportional + integral + differential) > 0))
      if (dif != 0)x = x - proportional + integral + differential;
    OCR1A = x;
    prevdif = dif;


    /*if(rpm>reqrpm)
      {
      decrease(dif);
      //x=x-1;
      OCR1A=x;
      PORTB&=~(1<<PB2);
      }
      else if(rpm<reqrpm)
      {
      increase(dif);
      //x=x+1;
      OCR1A=x;
      PORTB&=~(1<<PB2);
      }*/

    c = 0;
  }


}
void decrease(int dif)
{
  if (x > 0)
  {
    if (dif > 10)
      if (x > 10)x = x - 10;
      else if (dif <= 10)
        if (x > 1)x = x - 1;
  }
  else
    x += 10;
}
void increase(int dif)
{
  dif = (-1) * dif;
  if (x < 255)
  {
    if (dif > 10)
      if (x < 245)x = x + 10;
      else if (dif <= 10)
        if (x < 254)x = x + 1;
    Serial.println(x);
  }
}


