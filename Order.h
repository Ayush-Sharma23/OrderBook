#ifndef ORDER_H
#define ORDER_H

#include "declaratives.h"

class Order{
private:
    OrderId orderId;
    Price orderPrice;
    Quantity orderQuantity;
    Type orderType;
public:
    Order(): orderId(0), orderPrice(0), orderQuantity(0), orderType(Type::Buy){}
    Order(OrderId,Price,Quantity,Type);
    OrderId getOrderId() const;
    Price getOrderPrice()const;
    Quantity getOrderQuantity()const;
    Type getOrderType()const;
    void reduceQuantity(Quantity);
};

#endif
