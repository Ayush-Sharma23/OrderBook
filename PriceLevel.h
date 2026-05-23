#ifndef PRICELEVEL_H
#define PRICELEVEL_H 
#include "Order.h"

struct PriceLevel{
    Price levelPrice = 0;
    Quantity totalQuantity = 0;

    std::list<Order> levelQueue;
};

#endif

