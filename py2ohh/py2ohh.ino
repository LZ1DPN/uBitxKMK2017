/*
This entire program is taken from Jason Mildrum, NT7S and Przemek Sadowski, SQ9NJE.
There is not enough original code written by me (AK2B) to make it worth mentioning.
http://nt7s.com/
http://sq9nje.pl/
http://ak2b.blogspot.com/
Changed some lines to be OK according new library from NT7S - py2ohh Miguel
http://py2ohh.w2c.com.br/
uBitx mod - 25.09.2017 LZ1DPN
*/

#include <rotary.h>
#include <si5351.h>
#include <Wire.h>
#include <LiquidCrystal.h>


#define F_MIN        1000000UL               // Lower frequency limit
#define F_MAX        16000000000UL
#define SECOND_OSC   (57000000l)

#define ENCODER_A    3                      // Encoder pin A
#define ENCODER_B    2                      // Encoder pin B
#define ENCODER_BTN  11
#define LCD_RS        5
#define LCD_E         6
#define LCD_D4        7
#define LCD_D5        8
#define LCD_D6        9
#define LCD_D7        10

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);       // LCD - pin assignement in
Si5351 si5351;
Rotary r = Rotary(ENCODER_A, ENCODER_B);
volatile uint32_t LSB = 999830000ULL;
volatile uint32_t USB = 999630000ULL;
volatile uint32_t bfo = 999850000ULL;   //start in usb
//These USB/LSB frequencies are added to or subtracted from the vfo frequency in the "Loop()"
//In this example my start frequency will be 14.20000 plus 9.001500 or clk0 = 23.2015Mhz
volatile uint32_t vfo = 700000000ULL / SI5351_FREQ_MULT; //start freq - change to suit
volatile uint32_t radix = 100;    //start step size - change to suit
boolean changed_f = 0;
String tbfo = "";

//------------------------------- Set Optional Features here --------------------------------------
//Remove comment (//) from the option you want to use. Pick only one
#define IF_Offset //Output is the display plus or minus the bfo frequency
//#define Direct_conversion //What you see on display is what you get
//#define FreqX4  //output is four times the display frequency
//--------------------------------------------------------------------------------------------------


/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_CW)
    set_frequency(1);
  else if (result == DIR_CCW)
    set_frequency(-1);
}
/**************************************/
/* Change the frequency               */
/* dir = 1    Increment               */
/* dir = -1   Decrement56
/**************************************/
void set_frequency(short dir)
{
  if (dir == 1)
    vfo += radix;
  if (dir == -1)
    vfo -= radix;

  if(vfo > F_MAX)
     vfo = F_MAX;
  if(vfo < F_MIN)
     vfo = F_MIN;

  changed_f = 1;
}
/**************************************/
/* Read the button with debouncing    */
/**************************************/
boolean get_button()
{
  if (!digitalRead(ENCODER_BTN))
  {
    delay(20);
    if (!digitalRead(ENCODER_BTN))
    {
      while (!digitalRead(ENCODER_BTN));
      return 1;
    }
  }
  return 0;
}

/**************************************/
/* Displays the frequency             */
/**************************************/
void display_frequency()
{
  uint16_t f, g;

  lcd.setCursor(3, 0);
  f = vfo / 1000000;     //variable is now vfo instead of 'frequency'
  if (f < 10)
    lcd.print(' ');
  lcd.print(f);
  lcd.print('.');
  f = (vfo % 1000000) / 1000;
  if (f < 100)
    lcd.print('0');
  if (f < 10)
    lcd.print('0');
  lcd.print(f);
  lcd.print('.');
  f = vfo % 1000;
  if (f < 100)
    lcd.print('0');
  if (f < 10)
    lcd.print('0');
  lcd.print(f);
  lcd.print("Hz");
  lcd.setCursor(0, 1);
  lcd.print(tbfo);
  //Serial.println(vfo + bfo);
  //Serial.println(tbfo);

}

/**************************************/
/* Displays the frequency change step */
/**************************************/
void display_radix()
{
  lcd.setCursor(9, 1);
  switch (radix)
  {
    case 1:
      lcd.print("    1");
      break;
    case 10:
      lcd.print("   10");
      break;
    case 100:
      lcd.print("  100");
      break;
    case 1000:
      lcd.print("   1k");
      break;
    case 10000:
      lcd.print("  10k");
      break;
    case 100000:
      //lcd.setCursor(10, 1);
      lcd.print(" 100k");
      break;
      case 1000000:
      lcd.setCursor(9, 1);
      lcd.print("1000k"); //1MHz increments
      break;
  }
  lcd.print("Hz");
}


void setup()
{
  Serial.begin(19200);
  lcd.begin(16, 2);                                                    // Initialize and clear the LCD
  lcd.clear();
  Wire.begin();

  si5351.set_correction(1); //**mine. There is a calibration sketch in File/Examples/si5351Arduino-Jason
  //where you can determine the correction by using the serial monitor.

  //initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 25000000L, 0); //If you're using a 27Mhz crystal, put in 27000000 instead of 0
  // 0 is the default crystal frequency of 25Mhz.

  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);

  Serial.println("*Fix PLL\n");
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.output_enable(SI5351_CLK2, 1);
  
#ifdef IF_Offset
//BFO
  si5351.set_freq(bfo, SI5351_CLK0); 
  tbfo = "LSB";

//SECOND OSCILATOR
  si5351.set_freq(SECOND_OSC * 100ULL, SI5351_CLK1); 

//VFO
  volatile uint32_t vfoT = (vfo * SI5351_FREQ_MULT) + 4500000000;
  si5351.set_freq(vfoT, SI5351_CLK2);
  delay(2000);
  
// Set CLK levels
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_8MA); //you can set this to 2MA, 4MA, 6MA or 8MA
  si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_8MA); //be careful though - measure into 50ohms
  si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_8MA); //
 
  Serial.println("*Enable PLL Output\n");
  Serial.println("*Si5351 ON\n");
  delay(10);
#endif

#ifdef Direct_conversion
  si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_CLK0);
#endif

#ifdef FreqX4
  si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4, SI5351_CLK0);
#endif

  pinMode(ENCODER_BTN, INPUT_PULLUP);
  PCICR |= (1 << PCIE2);           // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  display_frequency();  // Update the display
  display_radix();
}


void loop()
{
  // Update the display if the frequency has been changed
  if (changed_f)
  {
    display_frequency();

#ifdef IF_Offset
//VFO
    si5351.set_freq((vfo * SI5351_FREQ_MULT) + 4500000000, SI5351_CLK2);

//BFO
if (changed_f){
    if (vfo >= 10000000ULL & tbfo != "USB")
    {
      bfo = USB;
      tbfo = "USB";
      si5351.set_freq( bfo, SI5351_CLK0);
      Serial.println("We've switched from LSB to USB");
    }
    else if (vfo < 10000000ULL & tbfo != "LSB")
    {
      bfo = LSB;
      tbfo = "LSB";
      si5351.set_freq( bfo, SI5351_CLK0);
      Serial.println("We've switched from USB to LSB");
    }
}
#endif

#ifdef Direct_conversion
    si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_CLK0);
    tbfo = "";
#endif

#ifdef FreqX4
    si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4, SI5351_CLK0);
    tbfo = "";
#endif

    changed_f = 0;
  }

  // Button press changes the frequency change step for 1 Hz steps
  if (get_button())
  {
    switch (radix)
    {
      case 1:
        radix = 10;
        break;
      case 10:
        radix = 100;
        break;
      case 100:
        radix = 1000;
        break;
      case 1000:
        radix = 10000;
        break;
      case 10000:
        radix = 100000;
        break;
      case 100000:
        radix = 1000000;
        break;
    }
    display_radix();
  }
}

