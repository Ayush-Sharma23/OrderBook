#include "Order.h"
#include "PriceLevel.h"

class OrderBook{
    private:
        std::map<Price, PriceLevel, std::greater<Price>> buys_;
        std::map<Price,PriceLevel> asks_;
    
        using ListIterator = std::list<Order>::iterator;

        struct orderDetail{
            Price price;
            Type type;
            ListIterator it;
        };

        std::unordered_map<OrderId,orderDetail> orderMap_;
    
    public:
        bool validateOrder(Order order);
        void MatchOrder(Order& order);
        void CancelOrder(OrderId orderId);
        void AddOrder(Order& order);
};
