#ifndef ITEM_H
#define ITEM_H

#if defined(ARDUINO) && (ARDUINO >= 100)
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

class Item {
public:
    Item();
    Item(String name2, int id2, float price2, float discount2);
    
//private:
    /* Max length: PAPER_CHAR_WIDTH - 6 */
    String name;
    /* This is for if you want the ID to be a special number not 
       dependent on the program, like ID 100 when there are only 
       4 items. */
    int id;
    float price;
    /* IEEE discount */
    float discount;
    byte quantity;
};



#endif ITEM_H
