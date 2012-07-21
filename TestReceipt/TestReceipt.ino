

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


/* Character width of receipt paper used to psudo-right justify */
const char PAPER_CHAR_WIDTH = 32;

const int PRINTER_RX_PIN = A0;  /* this is the green wire */
const int PRINTER_TX_PIN = A1;  /* this is the yellow wire */

const unsigned char LCD_COLS = 20;

const int DataPin = A2;
const int IRQpin =  3;

Adafruit_Thermal printer(PRINTER_RX_PIN, PRINTER_TX_PIN);
RTC_DS1307 RTC;
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
PS2Keyboard keyboard;

String cashierName;

void setup(){
    /* Set up all hardware */
    Serial.begin(9600);
    Wire.begin();
    RTC.begin();
    printer.begin();
    printer.setDefault();
    lcd.begin(LCD_COLS, 4);
    keyboard.begin(DataPin, IRQpin);

        
    /* Hardware check: RTC */
    if (! RTC.isrunning()) {
        Serial.println("RTC is NOT running!");
    }
    
    /* Print date and time and prompt cashier to enter name */
    lcd.setCursor(0, 0);
    String date = dateStr();
    String time = timeStr();
    lcd.print(date);
    for(char i = 0; i < (LCD_COLS - date.length() - time.length()); i++){
        lcd.print(" ");
    }
    lcd.print(time);
    lcd.setCursor(0, 1);
    lcd.print("Enter cashier name: ");
    cashierName = LCDinputPrompt(3);
    Serial.println(cashierName);
}
void loop(){
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Enter customer name:");
    String customerName = LCDinputPrompt(3);
    
    printReceipt(customerName);
}
void clearPS2Buffer(){
    while(keyboard.available()){
        keyboard.read();
    }
}
/* Blinks a line on the LCD until user enters value and presses enter. Max 
  length 20 chars. */
String LCDinputPrompt(char row){
    String input = "";
    clearPS2Buffer();
    
    lcd.cursor();
    lcd.blink();
    lcd.setCursor(0, row);
    
    char c = 0;
    while(c != PS2_ENTER){
        if(keyboard.available()){
            c = keyboard.read();
            
            if(c == PS2_DELETE){
                lcd.setCursor(0, row);
                lcd.print("                    ");
                lcd.setCursor(0, row);
                input = "";
            } else if (c == PS2_TAB) {
            } else if (c == PS2_ESC) {
            } else if (c == PS2_PAGEDOWN) {
            } else if (c == PS2_PAGEUP) {
            } else if (c == PS2_LEFTARROW) {
            } else if (c == PS2_RIGHTARROW) {
            } else if (c == PS2_UPARROW) {
            } else if (c == PS2_DOWNARROW) {
            } else {
                lcd.print(c);
                input += c;
            }
        }
    }
    
    lcd.noCursor();
    lcd.noBlink();
    return input;
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
void printReceipt(String customerName){
    
//    Serial.println(date);
//    Serial.println(time);
    
    /* Print logo */
    printer.printBitmap(380, 128, logo);
    
    /* Print contact info */
    printer.justify('C'); 
    printer.underlineOn(); 
    printer.println("ieee@ee.ucr.edu");
    printer.underlineOff(); 
    
    /* Print date and time */
    printer.feed(1);
    printer.justify('L'); 
    printer.boldOn();
    printLeftRight(dateStr(), timeStr(), PAPER_CHAR_WIDTH);
    printer.boldOff();
    
    /* Print cashier and customer name */
    printer.print("Cashier: ");
    printer.println(cashierName);
    printer.print("Customer: ");
    printer.println(customerName);
    
    /* Print items */
    printer.feed(1);
    printLeftRight(String("EE1A Parts Kit"), String("30.00"), PAPER_CHAR_WIDTH);
    printLeftRight(String("Large breadboard"), String("30.00"), PAPER_CHAR_WIDTH);   
    printLeftRight(String("Total: "), String("60.00"), PAPER_CHAR_WIDTH);
    
    printer.feed(1);
    printer.print("Lists: ");
    printer.underlineOn(); 
    printer.println("ieee.ucr.edu/parts.html");
    printer.underlineOff(); 
    
    printer.feed(1);
    printer.println("Thanks for supporting UCR IEEE!");

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
