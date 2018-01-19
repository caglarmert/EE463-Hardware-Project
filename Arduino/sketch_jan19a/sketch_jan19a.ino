/*
 _____       _           _____            
|  __ \     | |         / ____|           
| |__) |   _| |___  ___| |  __  ___ _ __  
|  ___/ | | | / __|/ _ \ | |_ |/ _ \ '_ \ 
| |   | |_| | \__ \  __/ |__| |  __/ | | |
|_|    \__,_|_|___/\___|\_____|\___|_| |_|
------- Version 1 - August 2013 ----------
For Programmable Device Interface: PDI-1 - 128 x 64 LCD (21 x 8 chars) a char is  6x8 pixels   
Created by RMCybernetics - http://www.rmcybernetics.com/
Written by Richard Morrow
DESCRIPTION:
  Simple square wave generator with adjustable frequency and duty.
  LCD Menu based control
  Output oscilations will continue in the background allowing normal program execution.
  Output is on 2 pins, one normal, one inverted.
CONTROLS:
  Use the encoder to navigate the menu and adjust values. Pressing the encoder selects/sets an item
  Output on/off using SW4(D7)
  Increment multiplier select is SW3(D6)
KNOWN BUGS/LIMITATIONS: 
  Time values are truncated, not rounded
  Library has 125ns resolution which will limit accuracy of duty% setting at v.high frequencies
        
* This code is provided AS IS and without warranty of any kind *
*/
// I/O PORT LIST AND CONNECIONS
//  0(RX)   - 
//  1(TX)   -  
//  2       <-  ENCODER A -------------------- TODO: Knob 729-6220
//  3       <~  ENCODER B
//  4       <-  SW1
//  5       <~  SW2
//  6       <~  SW3
//  7       <~  SW4
//  8       <-  ENCODER SWITCH
//  9       ~>  = SIGNAL OUTPUT =
//  10      ~>  = INVERTED SIGNAL OUTPUT =
//  11      ~
//  12      -  
//  13      -  
//  A0      - 
//  A1      - 
//  A2      - 
//  A3      -  
//  A4      ->  SDA
//  A5      ->  SCL
//  A6      -  
//  A7      -  

// Thanks to all the authors of these great librarys
#include <Encoder.h>             // Encoder library http://www.pjrc.com/teensy/td_libs_Encoder.html
#include <Wire.h>          
#include <SPI.h>
#include <I2C_KS0108C_GLCD.h>    // LCD Screen Library http://www.rmcybernetics.com/projects/code/index.htm
#include <NanoPWMac.h>           // Nanoseconf PWM Library http://www.rmcybernetics.com/projects/code/index.htm

// PORT DEFINITIONS
#define ENCODER_A 2
#define ENCODER_B 3
#define ENCODER_SW 8
#define SW1 4
#define SW2 5
#define SW3 6
#define SW4 7


// Variables
I2C_KS0108C_GLCD lcd;

// ENCODER VARIABLES
Encoder ENCODER(3, 2);    // Set up encoder using didital pins 3 and 2 (these two use an interrupt)
unsigned int encoderInc[5] = {1, 10, 100, 1000, 10000}; // multiplication factor of encoder when adjusting values. Declared array size should be one greater than number of elements
int maxMulti = 4;         // max array position selectable from above
int encoderIncIndex = 0;  //index position of increment values

// PUSH SWITCH VARIABLES
boolean encSwP = 0;         // Previous state of encoder push switch
boolean startSwP = 0;       // Previous state of encoder push switch
boolean multiSwP = 0;       // Previous state of multiplier push switch

// MENU VARIABLES
char* menuText[]=    {"FREQ MODE", "FREQUENCY", "DUTY %", "TIME DISPLAY"};         // This array holds the list of menu items to show. 8 max
const int numMenuItems = (sizeof(menuText)/sizeof(char *)); // This holds the number of menu items for use later
int selectedMenuItem = 0;                                   // The currently selected menu item
int selectedMenuItemPrev = 0;                               // The previously selected item
int optionValPrev = 0;                                      // The previous option value

// MENU OPTION VARIABLES
char* optFreqModes[] = {"Hz", "kHz"};                       // Selectable options for menu item 0
char* optWidthModes[] = {"off", "ns", "us", "ms"};          // Selectable options for menu item 3
long optionValue[numMenuItems] = {0,1000,50,0};             // Stores a value to represent the selected option of each menu item. Value here is the default startup value.
unsigned long optionValuePrev[numMenuItems] = {0,0,0,0};             // The previously selected item
const int optionValueMin[numMenuItems] = {0,0,0,0};         // MINIMUM allowed option value
unsigned long optionValueMax[numMenuItems] = {1,2000000,100,3};      // MAXIMUM allowed value
boolean valAdj = false;                                     // Flag if true, encoder should adjust values not menu position

// SIGNAL OUTPUT VARIABLES
boolean outputOn = false;         // Flag for if signal output is on or off

// BITMAP IMAGES
byte rmcLogo[] PROGMEM = {
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0xf8,0xc,0xe4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,
  0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,
  0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xe4,0x8c,0x38,0xe0,0x0,0x0,0x0,
  0x0,0xf8,0xc,0xe4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,
  0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,
  0xf4,0xf4,0xf4,0xf4,0xf4,0xf4,0xe4,0xcc,0x98,0x30,0x60,0xc0,0x80,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x7f,0xc0,0x9f,0xbf,0xbf,0xbf,0xbf,0x9f,0xc1,0x7d,0x5,0x5,0x5,0x5,0x5,0x5,
  0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,
  0x5,0x85,0xfd,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0x0,0x0,
  0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0xfd,0x5,0x5,0x5,0x5,0x5,
  0x5,0x5,0x5,0x5,0x5,0xfd,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0xfd,0x5,
  0xd,0x19,0x33,0x67,0xcf,0x9f,0x3f,0x7f,0xff,0xff,0xfe,0xfc,0xf9,0xf3,0xe6,0xcc,
  0x98,0x30,0xe0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0xc0,0x60,0x30,0x98,0xcc,0xe6,
  0xf3,0xf9,0xfc,0xfe,0xff,0x7f,0x3f,0x9f,0xcf,0x67,0x31,0x18,0xf,0x0,0x0,0x0,
  0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,
  0x0,0x0,0x0,0x0,0x0,0x1,0x3,0x6,0xc,0xf9,0x3,0xff,0xff,0xff,0xff,0xff,
  0xff,0x0,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x80,0xc0,0x60,0x30,0x98,0xcc,0xe6,0xf3,0xf9,0xfc,0xfe,0xff,0xff,0xff,0x1f,
  0x4f,0xe7,0xb3,0x19,0xc,0x6,0x3,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,
  0xff,0x0,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0x80,
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xc0,0x60,0x30,0x98,0xcc,0xe6,
  0xf3,0xf9,0xfc,0x7e,0x3f,0x9f,0xcf,0x6f,0x2f,0x2f,0x6f,0xcf,0x9f,0x3f,0x7f,0xff,
  0xfe,0xfc,0xf9,0xf3,0xe6,0xcc,0x98,0x30,0x60,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,
  0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,
  0xff,0x0,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7f,0xc1,0x9c,0xbe,0xbe,0xbe,
  0xbe,0xbe,0xbe,0xbe,0xbe,0xbe,0xbe,0xbe,0xbe,0xbe,0xbe,0xbe,0xbf,0x9f,0xcf,0x67,
  0x33,0x19,0xc,0x6,0x3,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x3,0x6,0xc,
  0x19,0x33,0x67,0x4f,0xdf,0x9f,0xbf,0xbf,0xbe,0x9c,0xc1,0x7f,0x0,0x0,0x0,0x0,
  0x0,0x7f,0xc0,0x9f,0xbf,0xbf,0xbf,0xbf,0x9f,0xc0,0x7f,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x7f,0xc0,0x9f,0xbf,0xbf,0xbf,0xbf,0x9f,0xc0,0x7f,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7f,0xc0,0x9f,0xbf,0xbf,0xbf,0xbf,
  0x9f,0xc0,0x7f,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf0,0xf8,0x18,0x18,0x18,0x18,
  0x18,0x18,0x0,0x0,0x0,0xf8,0x80,0x80,0x80,0x80,0x80,0xf8,0x0,0x0,0x0,0xf8,
  0x18,0x18,0x18,0x38,0x30,0xe0,0x0,0x0,0x0,0xf0,0xb8,0x98,0x98,0x80,0x80,0x80,
  0x0,0x0,0x0,0xc0,0xf8,0x18,0x18,0x18,0x98,0xf8,0x78,0x0,0x0,0x0,0xf8,0x18,
  0x18,0x38,0x38,0x30,0xe0,0x0,0x0,0x0,0xf0,0x90,0x98,0x98,0x88,0x88,0x80,0x0,
  0x0,0x0,0xf8,0xe0,0x60,0x60,0x60,0x20,0x20,0x20,0x0,0x0,0x0,0xf8,0x0,0x0,
  0x0,0xf8,0x18,0x18,0x18,0x18,0x18,0x18,0x0,0x0,0x0,0xf0,0xb8,0x98,0x98,0x98,
  0x88,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf,0x1f,0x18,0x18,0x18,0x18,
  0x18,0x18,0x0,0x0,0x0,0x19,0x9,0xd,0x7,0x3,0x1,0x1,0x0,0x0,0x0,0x1f,
  0x1b,0x1b,0x1b,0x1b,0x1b,0x1f,0x0,0x0,0x0,0x1f,0x19,0x19,0x19,0x19,0x19,0x19,
  0x0,0x0,0x0,0x3,0x1f,0x1c,0x4,0x6,0x7,0xc,0x18,0x0,0x0,0x0,0x1f,0x0,
  0x0,0x0,0x0,0x0,0x1f,0x0,0x0,0x0,0x1f,0x19,0x19,0x19,0x19,0x19,0x19,0x0,
  0x0,0x0,0xf,0x1f,0x18,0x18,0x18,0x18,0x18,0x18,0x0,0x0,0x0,0x1f,0x0,0x0,
  0x0,0x1f,0x18,0x18,0x18,0x18,0x18,0x18,0x0,0x0,0x0,0x11,0x19,0x19,0x19,0x19,
  0x19,0x1f,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0  
};

void setup() {  
// SET OUTPUTS  
  pinMode(9, OUTPUT);    // Set PWM outputs
  pinMode(10, OUTPUT); 
// SET INPUTS
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  pinMode(ENCODER_SW, INPUT);
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(SW4, INPUT);
// ENABLE PULLUP RESISTORS
  digitalWrite(ENCODER_A, HIGH);
  digitalWrite(ENCODER_B, HIGH);
  digitalWrite(ENCODER_SW, HIGH);
  digitalWrite(SW1, HIGH);
  digitalWrite(SW2, HIGH);
  digitalWrite(SW3, HIGH);
  digitalWrite(SW4, HIGH);
  
// INITILIZE LCD
  lcd.begin ();
  TWBR = ((16000000 / 400000L) - 16) / 2;// Increase I2C speed to 400kHz instead of the default 100kHz
  
  lcd.blit (rmcLogo, sizeof rmcLogo); // Draw logo bitmap
  delay(2000);
  lcd.clear (0, 0, 128, 64, 0); // Set Whole screen Blank
  lcd.gotoxy (1, 0);            // Move cursor to x,y position
  for (selectedMenuItem = 0; selectedMenuItem < numMenuItems; selectedMenuItem++)  {
    UpdateOptionCursor(false);
  }
  selectedMenuItem = 0;
  UpdateMenuCursor(true);              // Draws list of menu items (true draws all items)
  
  lcd.gotoxy (1, 5*8);                // Move cursor to x,y position
  lcd.string ("OUTPUT: OFF", false);  // Show Status
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 _                       
| |                      
| |     ___   ___  _ __  
| |    / _ \ / _ \| '_ \ 
| |___| (_) | (_) | |_) |
\_____/\___/ \___/| .__/ 
                  | |    
                  |_|    
*/
void loop() {

//TEST ENCODER SWITCH PRESS AND RESPOND
  boolean encSw = !digitalRead(ENCODER_SW); // get switch state, use ! becasue active low
  if (encSw == 1) {                         // If pressed
   if (encSwP == 0) {                       // and was preveiously not pressed (to prevent retriggering)
     valAdj = !valAdj;                      // Toggle between adjusting menu or values
     UpdateOptionCursor(valAdj); 
     if (valAdj == false) {                 // if just changed back to menu
       encoderIncIndex = 0;                 //set encoder back to normal multiplication
       lcd.gotoxy (1, 4*8);       
       lcd.string("      ", false);         // Clear multiplier text
       UpdateMenuCursor(false);
       ENCODER.write(0);    
     } else {                               // else if just moved to select values
       ENCODER.write(0);
     }
   }
   encSwP = 1;
  } else encSwP = 0;
  
// Check for start/stop button Press
  boolean startSw = !digitalRead(SW4); // get switch state, use ! becasue active low
  if (startSw == 1) {                  // If pressed
   if (startSwP == 0) {                // and was preveiously not pressed (to prevent retriggering)
     outputOn = !outputOn;
     if (outputOn) {
       UpdateOutputs();
       lcd.gotoxy (1, 5*8);                  // Move cursor to x,y position
       lcd.string ("OUTPUT: ON ", false);    // Show Status
     } else {
       NanoPWMac_off(); // Stop outputs
       lcd.gotoxy (1, 5*8);                  // Move cursor to x,y position
       lcd.string ("OUTPUT: OFF", false);    // Show Status
     }
   }
   startSwP = 1;
  } else startSwP = 0;

// Check for scale adjustmets: When pressed, increases the amount of adjustment the encoder does
  boolean multiSw = !digitalRead(SW3); // get switch state, use ! becasue active low
  if (multiSw == 1) {                  // If pressed
   if (multiSwP == 0) {                // and was preveiously not pressed (to prevent retriggering)
     if (valAdj == true) {
       encoderIncIndex ++;
       if (encoderIncIndex > maxMulti) encoderIncIndex = 0;
       if (encoderIncIndex != 0) {      // if not increasing by 1, show multiplication
         lcd.gotoxy (1, 4*8);           // Move cursor to x,y position
         lcd.string("      ", false); 
         lcd.gotoxy (1, 4*8);           // Move cursor to x,y position
         lcd.string ("x", false);  
         char buf[5];
         lcd.string(itoa(encoderInc[encoderIncIndex], buf, 10), false);  
       } else {
         lcd.gotoxy (1, 4*8);           // Move cursor to x,y position
         lcd.string("      ", false);
       }
     }
   }
   multiSwP = 1;
  } else multiSwP = 0;


//EITHER UPDATE MENU OR OPTION VALUE
  if (valAdj == false) {  // if not adjusting a parameter
    SetMenu();            // Checks encoder stores selected menu position and updates cursor position
  } else {                // else adjust selected option
    SetValue();   
  } 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 ___      _   __  __              
/ __| ___| |_|  \/  |___ _ _ _  _ 
\__ \/ -_)  _| |\/| / -_) ' \ || |
|___/\___|\__|_|  |_\___|_||_\_,_| 
Checks if encoeder has changed position and updates cursor position accordingly 
 */
void SetMenu() {
  selectedMenuItem += ENCODER.read()/2;              // Reads encoder value and adds to current position (/2 becasue encoder gives +2 per click)
  if (selectedMenuItem < 0) {
    selectedMenuItem = 0;                            // Prevent less than 0
    ENCODER.write(0);                                // Reset encoder value
  }
  if (selectedMenuItem > numMenuItems-1) {
    selectedMenuItem = numMenuItems-1;               // Limit to number of menu items (-1 is used because first menu item is at number 0)
    ENCODER.write(0);                                // Reset encoder value
  }
  if (selectedMenuItem != selectedMenuItemPrev) {    // Encoder has moved so do stuff
    UpdateMenuCursor(false);                         // just update one menu item
    ENCODER.write(0);                                // Reset encoder value
  }
  selectedMenuItemPrev = selectedMenuItem;           // Save value for comparison on next loop
}

/*
 ___      _ __   __    _          
/ __| ___| |\ \ / /_ _| |_  _ ___ 
\__ \/ -_)  _\ V / _` | | || / -_)
|___/\___|\__|\_/\__,_|_|\_,_\___|
Checks if encoeder has changed and updates option value accordingly 
*/
void SetValue() {
// Increment Value
  if (selectedMenuItem != 0 || selectedMenuItem != 3) { // only multiply increment when not in menu 0 and 3 (freq mode and time display)
    optionValue[selectedMenuItem] += (ENCODER.read()/2)*encoderInc[encoderIncIndex]; // Reads encoder and multiplies when faster scrolling is enabled
  } else {
    optionValue[selectedMenuItem] += ENCODER.read()/2; // Reads encoder
  }

// Clip to MIN allowed
  if (optionValue[selectedMenuItem] < optionValueMin[selectedMenuItem]) { 
    optionValue[selectedMenuItem] = optionValueMin[selectedMenuItem];
    ENCODER.write(0);
  }
  
// Clip to MAX allowed
  if (optionValue[0] == 1 && selectedMenuItem == 1) {  // if in kHz mode and frequency is selected
    if (optionValue[selectedMenuItem] > optionValueMax[selectedMenuItem]/1000) {
      optionValue[selectedMenuItem] = optionValueMax[selectedMenuItem]/1000;      // Limit to max allowed
      ENCODER.write(0);
    }
  } else {
    if (optionValue[selectedMenuItem] > optionValueMax[selectedMenuItem]) {
      optionValue[selectedMenuItem] = optionValueMax[selectedMenuItem];           // Limit to max allowed
      ENCODER.write(0);
    }
  }

  if (optionValue[selectedMenuItem] != optionValuePrev[selectedMenuItem]) {       // Encoder has changed so update display and frequency output
    UpdateOptionCursor(valAdj);                                                   // Update the cursor display
    ENCODER.write(0);
    UpdateOutputs();
  }
  optionValuePrev[selectedMenuItem] = optionValue[selectedMenuItem];
}


/*
 _   _          _      _        ___       _   _          ___                     
| | | |_ __  __| |__ _| |_ ___ / _ \ _ __| |_(_)___ _ _ / __|  _ _ _ ___ ___ _ _ 
| |_| | '_ \/ _` / _` |  _/ -_) (_) | '_ \  _| / _ \ ' \ (_| || | '_(_-</ _ \ '_|
 \___/| .__/\__,_\__,_|\__\___|\___/| .__/\__|_\___/_||_\___\_,_|_| /__/\___/_|  
      |_|                           |_|
Highlights or un-hilights the current menu option and updates the value shown
*/
void UpdateOptionCursor(boolean select) {          // if select is true, option has just been selected
  char buf[12];                                    // text output buffer
  lcd.gotoxy (1, selectedMenuItem*8);              // Move cursor to x,y position
  lcd.string (menuText[selectedMenuItem], false);  // Clears cursor on menu
  lcd.gotoxy (14*6, selectedMenuItem*8);           // Move cursor to right
  
  switch (selectedMenuItem) {
    case 0:      // case 0 is top menu which uses text array Hz kHz
      if (optionValue[selectedMenuItem] == 0 && optionValPrev == 1) {    // If has become Hz Mode
        optionValue[1] = optionValue[1] * 1000;             // Convert back to Hz
        // Update freqency dsiplayed
        sprintf(buf, "%ld", optionValue[1]);                // Convert long to text
        lcd.gotoxy (14*6, 1*8);                             // reset cursor position
        lcd.string("       ", false);                      // clear line
        lcd.gotoxy (14*6, 1*8);                             // reset cursor position
        lcd.string(buf, 0);                                 // set option text
      } 
      if (optionValue[selectedMenuItem] == 1 && optionValPrev == 0) {      // else has become kHz mode
        optionValue[1] = optionValue[1] / 1000;             // Convert back to kHz
        // Update freqency dsiplayed
        sprintf(buf, "%ld", optionValue[1]);                // Convert long to text
        lcd.gotoxy (14*6, 1*8);                             // reset cursor position
        lcd.string("       ", false);                      // clear line
        lcd.gotoxy (14*6, 1*8);                             // reset cursor position
        lcd.string(buf, 0);                                 // set option text
      }
      
      lcd.gotoxy (14*6, selectedMenuItem*8);           // Move cursor to right
      lcd.string("       ", false);                                 // clear text line
      lcd.gotoxy (14*6, selectedMenuItem*8);                         // reset position
      lcd.string(optFreqModes[optionValue[selectedMenuItem]], select);   // highlight(if select is true) and set option text
      optionValPrev = optionValue[selectedMenuItem];
      break;
  case 3:      // case 3 is 4th menu item Time Display
      lcd.string("       ", false);                                 // clear text line
      lcd.gotoxy (14*6, selectedMenuItem*8);                         // reset position
      lcd.string(optWidthModes[optionValue[selectedMenuItem]], select);   // highlight(if select is true) and set option text
      if (optionValue[3] == 0) {
        // Clear pulse times text
        lcd.gotoxy (1, 6*8);                         // Set cursor position
        lcd.string("                     ", 0);      // Clear Line
        lcd.gotoxy (1, 7*8);                         // Set cursor position
        lcd.string("                     ", 0);      // Clear Line
      }
      break;    
    default: 
      // if nothing else matches, do the default
      char buf[12];                                       // text output buffer
      sprintf(buf, "%ld", optionValue[selectedMenuItem]); // Convert long to text
      lcd.string("       ", false);                       // clear line
      lcd.gotoxy (14*6, selectedMenuItem*8);              // reset position
      lcd.string(buf, select);                            // highlight(if select is true) and set option text 
  }
}

/*
 _   _          _      _       __  __               ___                     
| | | |_ __  __| |__ _| |_ ___|  \/  |___ _ _ _  _ / __|  _ _ _ ___ ___ _ _ 
| |_| | '_ \/ _` / _` |  _/ -_) |\/| / -_) ' \ || | (_| || | '_(_-</ _ \ '_|
 \___/| .__/\__,_\__,_|\__\___|_|  |_\___|_||_\_,_|\___\_,_|_| /__/\___/_|  
      |_|                                              
Updates menu cursor position
*/
void UpdateMenuCursor(boolean all) {
  if (all) {
    for (int i = 0; i < numMenuItems; i++){
      lcd.gotoxy (1, i*8); // Move cursor to x,y position
      if (selectedMenuItem == i) {
        lcd.string (menuText[i], true); // inverted text    
      } else {
        lcd.string (menuText[i], false); //non-inverted text 
      }
    } 
  } else { // just update one
    lcd.gotoxy (1, selectedMenuItemPrev*8);             // Move cursor to x,y position
    lcd.string (menuText[selectedMenuItemPrev], false); // deselect previous item
    lcd.gotoxy (1, selectedMenuItem*8);                 // Move cursor to x,y position
    lcd.string (menuText[selectedMenuItem], true);      // select current item    
  }
}


/*
 _   _          _      _        ___       _             _      
| | | |_ __  __| |__ _| |_ ___ / _ \ _  _| |_ _ __ _  _| |_ ___
| |_| | '_ \/ _` / _` |  _/ -_) (_) | || |  _| '_ \ || |  _(_-<
 \___/| .__/\__,_\__,_|\__\___|\___/ \_,_|\__| .__/\_,_|\__/__/
      |_|                                    |_|    
Enables or updates the frequency outputs
*/
void UpdateOutputs() {
  unsigned long freqVal;
  if (optionValue[0]==0) {            // If in Hz mode
    freqVal = optionValue[1];         // Holds the value for output frequency
  } else {
    freqVal = optionValue[1] * 1000;  // Holds the value for output frequency
  }
   
  unsigned long duty = optionValue[2];              // Holds the duty value from options
  unsigned long long periodNs = 1000000000 / freqVal;    // The period of the signal in nanoseconds (long long is needed due to huge numbers at low frequencies)
    
  unsigned long onTime = periodNs * duty / 100;     // Calculate on time when using duty percentage
  unsigned long offTime = periodNs - onTime;        // Off time
  
  // Makes onTime and offTime divisible by 125 (because output has 125ns resolution)
  onTime = (onTime/125)*125;
  offTime = (offTime/125)*125;
  
  char buf[21];                  // text output buffer NB: if this buffer is too small for what you load into it, the device will reset
  char buff[21];                 // another text output buffer
  unsigned long rem = 0;         // used to store number after decimal point
  
  switch (optionValue[3]) {
    case 0:                                                // OFF
      // Do nothing
    break;
    case 1:                                                // NANOSECONDS
      sprintf(buf, "Ton:  %ldns", onTime);                 // Convert long to text
      sprintf(buff, "Toff: %ldns", offTime);               // Convert long to text
    break;
    case 2:                                                 // MICROSECONDS
      rem = onTime % 1000;                                  // Get remainder for after decimal point
      rem = rem/10;                                         // Reduce decimal precision NB: value will be truncated, not rounded.
      sprintf(buf, "Ton:  %ld.%ldus", onTime/1000, rem);    // Convert to text
      rem = offTime % 1000;                                 // Get remainder for after decimal point
      rem = rem/10;                                         // Reduce decimal precision
      sprintf(buff, "Toff: %ld.%ldus", offTime/1000, rem);  // Convert to text
    break;
    case 3:                                                  // MILLISECONDS
      rem = (onTime/1000) % 1000000;                         // Get remainder for after decimal point
      rem = rem/10;                                          // Reduce decimal precision NB: value will be truncated, not rounded.
      //int zeros = ;
      sprintf(buf, "Ton:  %ld.%ldms", onTime/1000000, rem);  // Convert to text
      rem = (offTime/1000) % 1000000;                        // Get remainder for after decimal point
      rem = rem/10;                                          // Reduce decimal precision
      sprintf(buff, "Toff: %ld.%ldms", offTime/1000000, rem);// Convert to text
    break;   
  }
  
  // Check for fully on or off states and overwrite any text in the buffer
  if (optionValue[3] != 0) {
    if (freqVal == 0 || duty == 0) {      // Check if frequency or duty are zero
      sprintf(buf, "Ton:  zero");
        sprintf(buff, "Toff: infinity");
    } else {
      if (duty == 100) {                  // Check if duty is 100%
        sprintf(buf, "Ton:  infinity");
        sprintf(buff, "Toff: zero");
      }
    }
  }
  
  if (optionValue[3] != 0) {
    // Display the pulse times
    lcd.gotoxy (1, 6*8);                         // Set cursor position
    lcd.string("                     ", 0);      // Clear Line
    lcd.gotoxy (1, 6*8);
    lcd.string(buf, 0);                          // Write buffer to screen
    lcd.gotoxy (1, 7*8);                         // Set cursor position
    lcd.string("                     ", 0);      // Clear Line
    lcd.gotoxy (1, 7*8);
    lcd.string(buff, 0);                         // Write buffer to screen
  }
  
  NanoPWMac(freqVal,onTime);               // Activate PWM output
}

