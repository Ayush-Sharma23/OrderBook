#include "Order.h"

struct PriceLevel{
    Price levelPrice;
    Quantity totalQuantity;

    std::list<Order> levelQueue;
};
