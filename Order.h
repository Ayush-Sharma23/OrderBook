#ifndef ORDER_H
#define ORDER_H

//#include <bits/stdc++.h>
#include "declaratives.h"

class Order{
private:
    OrderId orderId;
    Price orderPrice;
    Quantity orderQuantity;
    Type orderType;
public:
    Order(OrderId,Price,Quantity,Type);
    OrderId getOrderId() const;
    Price getOrderPrice()const;
    Quantity getOrderQuantity()const;
    Type getOrderType()const;
    void reduceQuantity(Quantity);
};

#endif
