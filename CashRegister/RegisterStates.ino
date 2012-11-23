/* State machine for cash register.

    TODO: add a "start over" button (that resets the cart and everything else)
*/

enum registerStates {
    stateInputCashierName, 
    stateInputCustomerName,
    stateInputtingItemIDs,
    stateIEEEMember, 
    stateConfirmOrder,
    statePrintingReceipt,
    statePromptAnotherSale,
    statePrintTestReceipt
};

int registerState = stateInputCashierName;
boolean registerTransistion = true;

void registerSM() {
    static unsigned char itemQuantity = 1;
    
    /* Transitions:
       (do once upon entering state */
    if(registerTransistion){    
        #if DEBUG >= 2
        Serial << "S: " << registerState << endl;
        Serial << F("Free memory: ") << freeMemory() << endl;
        #endif
    
        switch (registerState) {
        case stateInputCashierName:
            lcd.setCursor(0, 1);
            lcd.print(F("Enter cashier name: "));
            LCDinputPrompt(3);
            break;
        case stateInputCustomerName:
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print(F("Enter customer name:"));
            LCDinputPrompt(3);
            break;
        case stateInputtingItemIDs:
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Enter item ID "));
            lcd.write(1); /* Left arrow */
            lcd.write(2); /* Right arrow */
            lcd.print(F(":qty "));
            lcd.setCursor(0, 1);
            lcd.print(F("+:add item "));
            lcd.write(5); /* Enter */
            lcd.print(F(":done"));
            printQuantity(itemQuantity);
            LCDinputPrompt(3);
            lcd.setCursor(textBuffer.length(), 3);
            break; 
        case stateIEEEMember: {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Apply IEEE member"));
            lcd.setCursor(0, 1);
            lcd.print(F("discount? (y/n):"));
            LCDinputPrompt(3);
        }
            break;            
        case stateConfirmOrder:
            /* TODO: confirm order showing all products ordered */
            
            #if DEBUG >= 1
            Serial << currentCartCount <<" items in cart: " << endl;
            for(byte b = 0; b < currentCartCount; b++){
                Serial << cart[b].quantity << "x "
                << cart[b].name << endl;
            }
            #endif
//            lcd.clear();
//            lcd.setCursor(0, 0);
//            lcd.print(F("First 3 items"));
//            lcd.setCursor(0, 1);
//            lcd.print(F("discount? (y/n):"));
//            LCDinputPrompt(3);
            break;
        case statePrintingReceipt:
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print(F("Printing receipt.."));
            total = sumUpParts();
//            lcd.setCursor(0, 2);
//            lcd.print(F("Total: "));
//            lcd.print(String(total, 2));
            printReceipt(customerName, cashierName);
            
            break;
        case statePromptAnotherSale: {
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print(F("New sale? Press y:"));
            LCDinputPrompt(3);
        }
            break;
            
        default:
            Serial.println(F("Invalid case"));
            delay(1000);
            break;
        }
        registerTransistion = false;
    }
    /* Actions: */
    switch (registerState) {
    case stateInputCashierName:
        
        if(LCDinputPrompt(-1) == 1){
            cashierName = textBuffer;
            Serial.println(cashierName);
            changeRegisterState(stateInputCustomerName);
        }
        break;
    case stateInputCustomerName:
        /* Look for special keys */
        for(unsigned char i = 0; i < specialCharBuffer.length(); i++){
            char c = specialCharBuffer[i];
            
            if(c == PS2_ESC){
                changeRegisterState(stateInputCashierName);
                specialCharBuffer[i] = ERASED_SPECIAL_CHARACTER;
            }
                
             
        }
        if(LCDinputPrompt(-1) == 1){
            customerName = textBuffer;
            changeRegisterState(stateInputtingItemIDs);
        }
        break;
    case stateInputtingItemIDs:{
        boolean returnPressed = false;
        /* Look for special keys */
        for(unsigned char i = 0; i < specialCharBuffer.length(); i++){
            char c = specialCharBuffer[i];
            
            /* Increment or decrement quantity */
            if(c == PS2_LEFTARROW || c == PS2_RIGHTARROW){
                if(c == PS2_RIGHTARROW){
                    itemQuantity++;
                }
                else if(c == PS2_LEFTARROW){
                    itemQuantity--;
                } 
                
                /* Print new quantity */
                printQuantity(itemQuantity);
                lcd.setCursor(textBuffer.length(), 3);
                
                specialCharBuffer[i] = ERASED_SPECIAL_CHARACTER;
            }
            else if(c == PS2_ESC){
                changeRegisterState(stateInputCustomerName);
                specialCharBuffer[i] = ERASED_SPECIAL_CHARACTER;
            }
//            else if(c == PS2_ENTER){
//                returnPressed = true;
//                specialCharBuffer[i] = ERASED_SPECIAL_CHARACTER;
//            }
        }
        boolean plusEntered = false;
        if (textBufferChanged) {
            /* Erase line and print quantity */
            lcd.setCursor(0, 2);
            lcd.print(F("                    "));
            printQuantity(itemQuantity);
                
            if(textBuffer.length() > 0){
                /* Reject all input besides numbers */
                char c = textBuffer[textBuffer.length() - 1];
                if(!(c >= '0' && c <= '9')){
                    if(c == '+'){
                        plusEntered = true;
                    }
                    /* Remove last character */
                    textBuffer = textBuffer.substring(0, textBuffer.length() - 1);
                }
            }    
            if(textBuffer.length() > 0){
                /* Look up entered item ID in items array */
                char idStr[textBuffer.length() + 1]; /* Don't understand why I need this "+ 1" */
                textBuffer.toCharArray(idStr, textBuffer.length() + 1);
                int id = atoi(idStr); 
//                Serial << "textBuffer: " << textBuffer << " idStr: " << idStr << " Converted: " << id << endl;
                for(byte i = 0; i < NUM_ITEMS; i++){
                    /* If match found then print the name beside the quantity */
                    if(items[i].id == id){
                        
                        
                        /* If the return button was entered then add the item to cart   */
//                        if(returnPressed){  
                        if(plusEntered){
                            if(currentCartCount < MAX_CART_ITEMS){
                                cart[currentCartCount] = items[i];
                                cart[currentCartCount].quantity = itemQuantity;
                                #if DEBUG >= 1
                                Serial << "Item added: " << itemQuantity << "x "
                                << cart[currentCartCount].name << endl;
                                #endif
                                currentCartCount++;
                                
                                /* Clear text buffer and reset quantity */
                                textBuffer = "";
                                itemQuantity = 1;
                                changeRegisterState(stateInputtingItemIDs);
                            }
                        } 
                        /* Else print the name of the item to screen */
                        else {
                            #if DEBUG >= 1
                            Serial.print(F("match found: "));
                            Serial.println(items[i].name);
                            #endif
                            String toPrint = items[i].name.substring(0, 16);
                            #if DEBUG >= 2
    //                        Serial << toPrint << endl;
                            #endif
                            lcd.setCursor(4, 2);
                            lcd.print(toPrint);
                        }
                    }
                }
            }
        }
//        LCDinputPrompt(-1); /* "Enter key" was handled earlier in the state */
//        if(plusEntered){
        if(LCDinputPrompt(-1) == 1){
            /* Continue if cart is not empty */
            if(currentCartCount > 0){
                changeRegisterState(stateIEEEMember);
            } 
            else {
                lcd.setCursor(0, 0);
                lcd.print(F("Cart is empty"));
                lcd.setCursor(textBuffer.length(), 3);
            }
        }
        break;    
    }        
    case stateIEEEMember: 
        /* Look for special keys */
        for(unsigned char i = 0; i < specialCharBuffer.length(); i++){
            char c = specialCharBuffer[i];
            
            if(c == PS2_ESC){
                changeRegisterState(stateInputtingItemIDs);
                specialCharBuffer[i] = ERASED_SPECIAL_CHARACTER;
            }
        }
        /* y/n prompt for IEEE member discount  */
        if(LCDinputPrompt(-1) == 1){
            if(LCDpromptYN() == 1){
                applyMemberDiscount = true;
                changeRegisterState(stateConfirmOrder);
            } 
            else if(LCDpromptYN() == 0){
                changeRegisterState(stateConfirmOrder);
            }
            else {
                /* Prompt again. */
                changeRegisterState(stateIEEEMember);
            }
        }
        break; 
    case stateConfirmOrder:
        changeRegisterState(statePrintingReceipt);
        break;
    case statePrintingReceipt:
        changeRegisterState(statePromptAnotherSale);
        break;
    case statePromptAnotherSale: 
        if(LCDinputPrompt(-1) == 1){
            if(LCDpromptYN() == 1){
                /* Reset everything */  
                itemQuantity = 1;
                customerName = "";
                currentCartCount = 0;
                applyMemberDiscount = false;
                total = 0;
                changeRegisterState(stateInputCustomerName);
            }
            else {
                /* Prompt again. */
                changeRegisterState(statePromptAnotherSale);
            }
        }
        break;  
    default:
        Serial.println(F("Invalid case"));
        delay(1000);
        break;
    }
}
/* Changes state with registerTransistion true assignment. */
void changeRegisterState(int state){
    registerState = state;
    registerTransistion = true;        
}
/* Prints a quantity and an x, for example 3x */
void printQuantity(byte quantity){  
    lcd.setCursor(0, 2);
    lcd.print("    ");
    lcd.setCursor(0, 2);
    lcd.print(quantity);
    lcd.print('x');
}
/* Checkout by adding together cost of all parts */
float sumUpParts(){
    float currentTotal = 0;
    for(byte b = 0; b < currentCartCount; b++){
        currentTotal += cart[b].price * cart[b].quantity;
        
        /* IEEE discount */
        if(applyMemberDiscount){  
            currentTotal -= cart[b].discount * cart[b].quantity;
        }
    }
    return currentTotal;
}
