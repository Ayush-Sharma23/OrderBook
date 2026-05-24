#include "Order.h"

Order::Order(OrderId orderId_, Price orderPrice_, Quantity orderQuantity_, Type orderType_):
    orderId(orderId_),
    orderPrice(orderPrice_),
    orderQuantity(orderQuantity_),
    orderType(orderType_)
{}

OrderId Order::getOrderId()const{return orderId;}
Price Order:: getOrderPrice()const{return this->orderPrice;}
Quantity Order::getOrderQuantity()const{return this->orderQuantity;}
Type Order::getOrderType()const{return this->orderType;}

void Order::reduceQuantity(Quantity traded){
    this->orderQuantity -= traded; 
}
