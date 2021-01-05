/*
IMSAI Guy 2020
Sinewave Generator
0 to 30 MHz

first shift register
Data D0/D7
second shift register
0 A4
1 A3
2 A2
3 A1
4 A0
5 RST
6 RD
7 WR

*/
//==============================================================
#define latchPin 2  // HC595 latch
#define clockPin 3  // HC595 clock
#define dataPin  4  // HC595 data
#define Switches 5  // switches input pin
#define led     13  // on board arduino LED
#define W_CLK    6  //  AD9850 module word load clock pin (CLK)
#define FQ_UD    7  //  freq update pin (FQ)
#define DATA     8  //  serial data load pin (DATA)
#define RESET    9  //  reset pin (RST) 
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }
#define bounce 400  // switch bounce delay
//==============================================================
int bits  = 0;      // bit in byte pointer
int digit = 0;      // current digit selected
int SwitchSelected = 9;
float freq=10000;   //frequency in hertz
const int flipbits[8] = {0, 4, 2, 6, 1, 5, 3, 7}; //flip A0 A1 A2 for PCB layout
long inc_digit[8] = {1,10,100,1000,10000,100000,1000000,10000000};

//messages to display
char blank[9]   = "        ";
char num_asc[9] = "        ";
char mess0[9]   = "ImsaiGuy";
char mess1[9]   = "Enter Hz";
char mess2[9]   = " Entered";
char mess3[9]   = "   Fonts";


//==============================================================
void setup() 
{ pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(Switches, INPUT_PULLUP);
  //for AD9850 DDS chip
  pinMode(FQ_UD, OUTPUT);
  pinMode(W_CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(RESET, OUTPUT); 
  // initialize AD9850 DDS chip
  pulseHigh(RESET);
  pulseHigh(W_CLK);
  pulseHigh(FQ_UD);  // this pulse enables serial mode on the AD9850 - Datasheet page 12.
  
  digitalWrite(latchPin, LOW);
  digitalWrite(dataPin,  LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(led, LOW); //led only used for debug

  // Reset 8 digit LED Display    
      shiftOut(dataPin, clockPin, MSBFIRST, B11000011); //reset low
      shiftOut(dataPin, clockPin, MSBFIRST, 0x00);
   serial_latch();
      shiftOut(dataPin, clockPin, MSBFIRST, B11100011); //reset high
      shiftOut(dataPin, clockPin, MSBFIRST, 0x00);
   serial_latch();
   display(mess0);  //sign on message
   delay(2000);
   display(mess1);
   delay(2000);
   
   digit=0;
}

//==============================================================
void loop() 
//main loop is polling to see if a switch is pressed
{ ReadSwitches();
switch (SwitchSelected) {
    case 0: //enter
      sendFrequency(freq);     //send frequency to DDS chip
      display(mess2);          //say you did it
      delay(bounce);              //ugly switch bounce fix
      break;
    case 1: //up arrow
      freq=freq+inc_digit[digit];        //increment digit
      if(freq>30000000) freq=30000000;   //max frequency
      delay(bounce);              //ugly switch bounce fix
      break;
    case 2: //right arrow
      digit=digit-1;           //decrement digit select
      if(digit<0) digit=0;     //limit right movement to 0 digit
      updateShiftRegister(0x5e,7-flipbits[digit]);
      delay(bounce);              //ugly switch bounce fix
      break;
    case 3: //left arrow
      digit++;                  //increment digit select
      if(digit>7) digit=7;      //limit left movement to 7 digit
      updateShiftRegister(0x5e,7-flipbits[digit]);
      delay(bounce);               //ugly switch bounce fix
      break;
    case 4: //down arrow
      freq=freq-inc_digit[digit]; //decrement digit
      if(freq<0) freq=freq+inc_digit[digit];
      if(freq>30000000) freq=30000000;
      delay(bounce);           //ugly switch bounce fix
      break;
    case 5: // #3 button 
      freq=0;               //zero frequency
      digit=0;
      break;
    case 6: // #2 button
      display(mess0);       //sign on message
      delay(1000);
      break;
    case 7: // #1 button
      display(mess3);       //display font test routine
      delay(1000);
      displayfont();
      break;
    default: 
      disp_freq();          //update display if no key detected
      break;
  }
}
//==============================================================
//display frequency
void disp_freq()
{     dtostrf(freq,8, 0, num_asc);  //convert 'freq' (float) to 8 char string 0 decimals
      display(num_asc);
}
//==============================================================
// display character 'data' at LED character location 'address'
void updateShiftRegister(int data, int address)
{     //lower write pin
      bits =  B01100011 | (address << 2);
      shiftOut(dataPin, clockPin, MSBFIRST, bits );
      shiftOut(dataPin, clockPin, MSBFIRST, data);
   serial_latch();
      //raise write pin
      bits =  B11100011 | (address << 2); 
      shiftOut(dataPin, clockPin, MSBFIRST, B11100011);
      shiftOut(dataPin, clockPin, MSBFIRST, data);
   serial_latch();
}
//==============================================================
//latch data into HC595
void serial_latch()
{  digitalWrite(latchPin, HIGH);  //high going pulse
   digitalWrite(latchPin, LOW);
}
//==============================================================
//display LED message string
void display(char mess[9])
{ for (int j=0; j<8; j++)         //LED digits addresses 0 to 7
  { updateShiftRegister(mess[j],flipbits[j]);
  }
}
//==============================================================
//demo routine to display all characters
void displayfont() {
  for (int i=0; i <= 0x80; i++)   //character set in LED display
  { for (int j=0; j<8; j++)       //LED display address 0 to 7
    { updateShiftRegister(i,flipbits[j]);
    }
    delay(200);
  }
}
//==============================================================
//scan switches
int ReadSwitches()
{ SwitchSelected = 9;
  for (int i=0; i <= 8; i++)
  { bits=0xFF;
    bitClear(bits, i);
    shiftOut(dataPin, clockPin, MSBFIRST, B11100011);
    shiftOut(dataPin, clockPin, MSBFIRST, bits);
    serial_latch();
    int SwitchState = digitalRead(Switches);
    if (SwitchState == LOW) 
    { SwitchSelected = i;
      break;
    }
  }
}
//==============================================================
// frequency calc from datasheet page 8 = <sys clock> * <frequency tuning word>/2^32
void sendFrequency(double frequency)
  {  //125 MHz clock.  You can make 'slight' tuning variations here by adjusting the clock frequency.
    int32_t freq = frequency * 4294967295/125000000;  
    for (int b=0; b<4; b++, freq>>=8) 
    { tfr_byte(freq & 0xFF);
    }
  tfr_byte(0x000);   // Final control byte, all 0 for 9850 chip
  pulseHigh(FQ_UD);  // Done!  Should see output
}
//==============================================================
// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void tfr_byte(byte data)
{  for (int i=0; i<8; i++, data>>=1) 
   { digitalWrite(DATA, data & 0x01);
     pulseHigh(W_CLK);   //after each bit sent, CLK is pulsed high
   }
}
