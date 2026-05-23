#include "Order.h"
#include "PriceLevel.h"

class OrderBook{
    private:
        std::map<Price, PriceLevel, std::greater<Price>> buys_;
        std::map<Price,PriceLevel> asks_;
        std::unordered_map<OrderId,std::pair<Price,Type>> orderMap_;
    public:
        bool validateOrder(Order order);
        void MatchOrder(Order& order);
        void CancelOrder(OrderId orderId);
        void AddOrder(Order& order);
};
