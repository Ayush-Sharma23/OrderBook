#include "Order.h"

int main(){
    
    Order ob(1,50,100,Type::Buy);

    std::cout<<"OrderId\t:\t"<<ob.getOrderId()
        <<"\nPrice\t:\t"<<ob.getOrderPrice()
        <<"\nQuantity\t:\t"<<ob.getOrderQuantity()
        <<"\nType\t:\t"<<ob.getOrderType();

    return 0;
}
