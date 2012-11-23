/* This is a cash register program for selling IEEE
  parts kits. */

/* Debug 0 just shows basic information
    Debug level 2 shows states and memory  */
#define DEBUG 1

/* Printer includes */
#include "SoftwareSerial.h"
#include "Adafruit_Thermal.h"
#include "logo.cpp"
/* RTC includes */
#include <Wire.h>
#include "RTClib.h"
/* LCD include */
#include <LiquidCrystal.h>
/* Keyboard */
#include <PS2Keyboard.h>
#include <MemoryFree.h>
/* Others */
#include <Streaming.h>
#include "Item.h"
/* Requires all of the above plus float-to-string patch: 
http://www.timewasters-place.com/arduino-string-and-float/ */

/* Character width of receipt paper used to psudo-right justify */
const char PAPER_CHAR_WIDTH = 32;

const int PRINTER_RX_PIN = A0;  /* this is the green wire */
const int PRINTER_TX_PIN = A1;  /* this is the yellow wire */

const unsigned char LCD_COLS = 20;

const int DataPin = A2;
const int IRQpin =  3;

const char ERASED_SPECIAL_CHARACTER = 'a';

const int NUM_ITEMS = 3;
/* The catalog */
Item items[NUM_ITEMS];

Adafruit_Thermal printer(PRINTER_RX_PIN, PRINTER_TX_PIN);
RTC_DS1307 RTC;
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
PS2Keyboard keyboard;


/** Start global variables */
String cashierName;
String customerName;
/* "Buffers" */
String textBuffer;
String specialCharBuffer;
boolean textBufferChanged = false;

/* For the current sale */
const byte MAX_CART_ITEMS = 10;
Item cart[MAX_CART_ITEMS];
byte currentCartCount = 0;
boolean applyMemberDiscount = false;
float total = 0;

byte rightArrow[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b00010,
  0b11111,
  0b00010,
  0b00100,
  0b00000
};
byte leftArrow[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b01000,
  0b11111,
  0b01000,
  0b00100,
  0b00000
};
byte enter[8] = {
  0b00000,
  0b00000,
  0b00001,
  0b00101,
  0b01001,
  0b11111,
  0b01000,
  0b00100
};
byte upArrow[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100
};
byte downArrow[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};

void setup(){
    /* Set up all hardware */
    Serial.begin(57600);
    Wire.begin();
    RTC.begin();
    printer.begin();
    printer.setDefault();
    lcd.begin(LCD_COLS, 4);
    keyboard.begin(DataPin, IRQpin);
    
    Serial << F("IEEE Cash Register") << endl;
    #if DEBUG >= 2
    Serial << F("Free memory: ") << freeMemory() << endl;
    #endif
        
    /* Hardware check: RTC */
    if (! RTC.isrunning()) {
        Serial << F("RTC is NOT running!") << endl;
    }
    
    /* Put custom characters into LCD */
    lcd.createChar(1, leftArrow);
    lcd.createChar(2, rightArrow);
    lcd.createChar(3, upArrow);
    lcd.createChar(4, downArrow);
    lcd.createChar(5, enter);
    
    
    /* Print date and time and prompt cashier to enter name */
    lcd.setCursor(0, 0);
    String date = dateStr();
    String time = timeStr();
    lcd.print(date);
    for(char i = 0; i < (LCD_COLS - date.length() - time.length()); i++){
        lcd.print(" ");
    }
    lcd.print(time);
    
    populateItems();
                
}

void loop(){
    registerSM();
    processKeys();
}
void clearBuffers(){
    while(keyboard.available()){
        keyboard.read();
    }
    textBuffer = "";
    specialCharBuffer = "";
    textBufferChanged = false;
}
/* Reads from the PS2 ring buffer and stores into this program's 
   buffers. There is one for text and one for special keys like 
   arrows, tab, Esc. */
void processKeys(){
    while(keyboard.available()){
        char c = keyboard.read();
        
        if(c == PS2_DELETE
        || c == PS2_ENTER
        || c == PS2_TAB
        || c == PS2_ESC
        || c == PS2_PAGEDOWN
        || c == PS2_PAGEUP
        || c == PS2_LEFTARROW
        || c == PS2_RIGHTARROW
        || c == PS2_UPARROW
        || c == PS2_DOWNARROW){
            specialCharBuffer += c;
            
            /* This is for the item adding screen so that it can update
               the item display */
            if(c == PS2_DELETE){
                textBuffer = textBuffer.substring(0, textBuffer.length() - 1);
                textBufferChanged = true;
            }
        }
        else if (textBuffer.length() < LCD_COLS - 1) {
            textBuffer += c;
            textBufferChanged = true;
        }
    }
    /* Remove erased special keys (ERASED_SPECIAL_CHARACTER) */
    String newbuffer = "";
    for(unsigned char i = 0; i < specialCharBuffer.length(); i++){
        if(specialCharBuffer[i] != ERASED_SPECIAL_CHARACTER){
            newbuffer += specialCharBuffer[i];
        }
        
    }
    specialCharBuffer = newbuffer;
}
/* Blinks a line on the LCD until user enters value and presses enter. Max 
  length 20 chars. Intended use: call LCDinputPrompt with row >= 0 then 
  poll it with row == -1.
  
  @param char row Specifies which row on the LCD to start blinking. If -1, 
  indicates polling.
  @return 0 for not done, 1 for done. When done, read from textBuffer. 
*/
unsigned char LCDinputPrompt(char rowToPrint){
    static char row = 0;
  
    /* If prompting for new value, clear buffers and blink LCD. */
    if(rowToPrint >= 0){
        row = rowToPrint;
        clearBuffers();
        lcd.cursor();
        lcd.blink();
        lcd.setCursor(0, row);
        return 0;
    }
    else {
        /* Process keys in special keys queue */
        for(unsigned char i = 0; i < specialCharBuffer.length(); i++){
            char c = specialCharBuffer[i];
            
            /* Delete last character from LCD */
            if(c == PS2_DELETE){
                if(textBuffer.length() > 0){
                    lcd.setCursor(textBuffer.length(), row);
                    lcd.print(' ');
                    lcd.setCursor(textBuffer.length(), row);
                }
                
                /* Set special character to ERASED_SPECIAL_CHARACTER
                to erase it. */
                specialCharBuffer[i] = ERASED_SPECIAL_CHARACTER;
            } 
            /* Input is done so return 1 to notify calling function to 
               take from text buffer */
            else if (c == PS2_ENTER) {
                /* Set special character to ERASED_SPECIAL_CHARACTER
                to erase it. */
                specialCharBuffer[i] = ERASED_SPECIAL_CHARACTER;
              
                return 1;
            }
            
        }
        /* and update LCD with new text */
        if (textBufferChanged) {
            lcd.setCursor(0, row);
            lcd.print(F("                    "));
            lcd.setCursor(0, row);
            lcd.print(textBuffer);
            textBufferChanged = false;
        }
        return 0;
    }
}
/* Like the previous function but returns 1 for yes, 0 for no, and
   -1 for anything else. 
   Uses LCDinputPrompt and reads first character */
char LCDpromptYN(){
    char firstCharacter = textBuffer[0];
    
    if(firstCharacter == 'y' || firstCharacter == 'Y'){
        return 1;
    } 
    else if (firstCharacter == 'n' || firstCharacter == 'N'){
        return 0;
    } 
    else {
        return -1;
    }
}
String dateStr(){
    DateTime now = RTC.now();
    return String(now.month()) + "/" 
         + String(now.day()) + "/"
         + String(now.year());
}
String timeStr(){
    DateTime now = RTC.now();
    return String(now.hour()) + ":" 
         + String(now.minute()) + ":"
         + String(now.second());
}
void printReceipt(String customerName, String cashierName){
    
//    Serial.println(date);
//    Serial.println(time);
    
    /* Print logo */
    printer.printBitmap(380, 128, logo);
    
    /* Print contact info */
    printer.justify('C'); 
    printer.underlineOn(); 
    printer.println(F("ieee@ee.ucr.edu"));
    printer.underlineOff(); 
    
    /* Print date and time */
    printer.feed(1);
    printer.justify('L'); 
    printer.boldOn();
    printLeftRight(dateStr(), timeStr(), PAPER_CHAR_WIDTH);
    printer.boldOff();
    
    /* Print cashier and customer name */
    printer.print(F("Cashier: "));
    printer.println(cashierName);
    printer.print(F("Customer: "));
    printer.println(customerName);
    
    /* Print items */
    printer.feed(1);
//    printLeftRight(String("EE1A Parts Kit"), String("30.00"), PAPER_CHAR_WIDTH);
//    printLeftRight(String("Large breadboard"), String("30.00"), PAPER_CHAR_WIDTH);   
//    printLeftRight(String("Total: "), String("60.00"), PAPER_CHAR_WIDTH);
    for(byte b = 0; b < currentCartCount; b++){
        /* If quantity is over 1, format string like "2x EE1A Parts kit" */
        String itemString = "";
        if(cart[b].quantity > 1){
            itemString += String(cart[b].quantity);
            itemString += "x ";
        }
        itemString += cart[b].name;
        
        /* Print line */
        printLeftRight(itemString, String(cart[b].price * cart[b].quantity, 2), PAPER_CHAR_WIDTH);
        if(applyMemberDiscount){
            printLeftRight(String("IEEE discount"), String(-cart[b].discount * cart[b].quantity, 2), PAPER_CHAR_WIDTH);
        }
    }
    printLeftRight(String("Total: "), String(total, 2), PAPER_CHAR_WIDTH);
    
    printer.feed(1);
    printer.print(F("Lists: "));
    printer.underlineOn(); 
    printer.println(F("ieee.ucr.edu/parts.html"));
    printer.underlineOff(); 
    
    printer.feed(1);
    printer.println(F("Thanks for supporting UCR IEEE!"));

    /* End of receipt, feed some paper for tearing. */
    printer.feed(3);
}
/* Print the two strings with enough spaces to right justify the right string 
  The strings must fit inside width.
    */
void printLeftRight(String left, String right, char width){
    printer.print(left);
    for(char i = 0; i < (width - left.length() - right.length()); i++){
        printer.print(" ");
    }
    printer.println(right);
}
