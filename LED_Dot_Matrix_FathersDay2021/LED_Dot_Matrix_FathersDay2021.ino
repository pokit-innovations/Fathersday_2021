#include "LedControl.h"
#include "font5x7.h"

/**************************************/
/* Fathersday 2021                    */
/*                                    */
/* By Stephen Geary                   */
/*                                    */
/**************************************/

// requires a compatible 5x7 font (included in package)
// requires MAX7219 driver library found at https://www.arduinolibraries.info/libraries/led-control
// install MAX7219 driver library by downloading .ZIP and selecting [Sketch-->Include Library-->Add .ZIP Library...] and browse for the downladed .ZIP
// if arduino is a dodgey one off ebay then you will also need to install CH340 Drivers from here https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all to interface with arduino nano
// you may also need to select old bootloader [Tools-->Processor...-->ATmega328P (Old Bootloader)]

/******************************************/
/* Desk Nameplate Wiring Diagram          */
/*                                        */
/*         5V ---------VCC                */
/*         Gnd---------Gnd                */
/*   NANO  D12---------Din  MAX7219       */
/*         D10---------CS     #1          */
/*         D11---------Clk                */
/*                                        */
/*                                        */
/*     MCU <----VCC----VCC----            */
/*            --Gnd----Gnd----            */
/*   MAX7219  --Din    Do-----   MAX7219  */
/*     #1     --CS-----CS-----      #2    */
/*            --Clk----Clk----            */
/*                                        */
/*       same as above for 2-->3          */
/*                                        */
/******************************************/

 
LedControl lc1=LedControl(12,11,10,3);  // Pins: DIN,CLK,CS, # of Display connected

unsigned long delayTime=1000;  // Delay between Frames

#define SCREENS 3      // number of screens
#define Brightness 0   // Brightness level [0-15]
#define DEBUG 0        // debug mode (not used)

byte ScrollingText[1000];                                                                  // framebuffer
char DaD_Msg[] = "  I  DaD I";       // Main message [displayed 2/3 cycles]
byte Length = 0;                                                                           // Length of message stored here


byte SGDigital[SCREENS*8] = {      
   B01111110,B01111100,B11111100,
   B11111110,B11111110,B11111110,
   B11100000,B11100000,B11000110,
   B11100000,B11100000,B11000110,
   B01111110,B11101110,B11000110,
   B00001110,B11100110,B11000110,
   B11111110,B11111110,B11111110,
   B11111100,B01111100,B11111100
  };

byte Draw_Heart_1[] =
{
   B00000000,
   B01100110,  
   B11111111,
   B11111111,
   B01111110,
   B00111100,
   B00011000,
   B00000000
};

byte Draw_Heart_2[] =
{
   B00000000,
   B00000000,  
   B00100100,
   B01111110,
   B00111100,
   B00011000,
   B00000000,
   B00000000
};

/**************************************/
/*                                    */
/*Draw Character function             */
/*                                    */
/*Function to draw 5x7 font bitmaps to*/
/*framebuffer. The Characters need to */
/*be rotated 90 degrees to be         */
/*displayed                           */
/*                                    */
/**************************************/

void drawChar(int16_t x, int16_t y, unsigned char c) 
{
  for (int8_t i=0; i<=5; i++ ) {                     // loop through Character data 
    uint8_t line;                                    // store line data
    if (i == 5)                                      // the last line is always blank
      line = 0x0;
    else 
      line = pgm_read_byte(Font5x7+((c-32)*5)+i);    // Character codes are offset by 32 indicies, read from the PGMData page where the characters are stored
    for (int8_t j = 0; j<=7; j++) {                  // loop through each 
          if(bitRead(line, j))bitSet(ScrollingText[x+(j+y)*Length], 7-i); // nobody knows how this bit works (rotates characters 90 degrees)
    }
  }
}


/**************************************/
/*                                    */
/*Display Scroll Function             */
/*                                    */
/*Function to display to the screens  */
/*from the framebuffer. and scroll    */
/*                                    */
/**************************************/

void DisplayScroll(byte in[], int frame, byte len)
{
  for (int j = 0; j < 3; j++){   // loop through the screens
    for (int i = 0; i < 8; i++)  // loop through the rows of the screen
    {
      lc1.setRow(j,i,(in[j+frame/8+i*len]<<(frame%8) | in[j+frame/8+1+i*len]>>(8-(frame%8)))); // sliding window magic!!
    }
  }
}


/**************************************/
/*                                    */
/*Set Text Function                   */
/*                                    */
/*Function to load the text from a    */
/*string into the framebuffer.        */
/*                                    */
/**************************************/

void setText(char text[], byte len)
{

  Length = len;
  
  for (int i = 0; i < Length * 8; i++) ScrollingText[i] = 0x00; // Clear the framebuffer

  for (int i = 0; i < Length; i++)drawChar(i, 1, text[i]);      // Load characters into framebuffer one at a time while incrementing storage location

}


/**************************************/
/*                                    */
/*Draw Heart Function                 */
/*                                    */
/*Function to load a fixed width      */
/*bitmap into the framebuffer.        */
/*                                    */
/**************************************/

void setDrawHeart(byte x, byte h, byte l)
{
  for (int i = 0; i < 8; i++){           // Loop through 8x8 bitmap
    if(h<l/2 == 0)ScrollingText[i*Length+x] = Draw_Heart_1[i]; // copy to framebuffer at location [x]
    else ScrollingText[i*Length+x] = Draw_Heart_2[i]; 
  }
}

/**************************************/
/*Setup Function                      */
/*                                    */
/*Boilerplate to setup serial &       */
/*displays, and set the default text  */
/*                                    */
/**************************************/

void setup()
{
  Serial.begin(115200);                 // set up seraL for any debugging
  
  for (int i = 0; i < 3; i++){          // for each display
    lc1.shutdown(i,false);              // Wake up displays
    lc1.setIntensity(i,Brightness);     // Set Brightness levels
    lc1.clearDisplay(i);                // Clear Displays
  }
  
  DisplayScroll(SGDigital, 0, 3);        // display SGD Logo

  
  setText(DaD_Msg, sizeof(DaD_Msg)-1);   // load the full message into the framebuffer
  setDrawHeart(3,0,1);                   // draw the heart into the framebuffer
  
  delay(delayTime*5);                    // wait
}


int frame = 0; // frame the framebuffer is currently displaying

/**************************************/
/*main loop Function                  */
/*                                    */
/*increment through frames and display*/
/*framebuffer. Cycle through messages */
/*                                    */
/**************************************/

void loop()
{
 DisplayScroll(ScrollingText, frame, Length); // display frame
 delay(delayTime/50);                         // single frame delay
 setDrawHeart(3,frame%15, 15);                // draw the heart into the framebuffer
 frame++;
 if(frame > (Length-3)*8){                    // at end of message
  frame = 1;                                  // reset frame count
 }
}
