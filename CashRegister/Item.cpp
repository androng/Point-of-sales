#include "Item.h"

Item::Item(): 
           id(0), 
           price(0), 
           discount(0), 
           quantity(0)
{
    name = String("");
}
Item::Item(String name2,
           int id2,
           float price2,
           float discount2): 
           id(id2), 
           price(price2), 
           discount(discount2), 
           quantity(1)
{
    name = String(name2);
}

