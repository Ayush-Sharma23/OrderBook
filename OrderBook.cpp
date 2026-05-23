#include "OrderBook.h"

bool OrderBook::validateOrder(Order order){
    if(orderMap_.find(order.getOrderId()) != orderMap_.end()){
        return false;
    }
    if(order.getOrderQuantity() <= 0){
        return false;
    }
    return true;
}

void OrderBook::MatchOrder(Order& order){
    if(!validateOrder(order)){
        std::cerr<<"Invalid Order\n";
        return;
    }

    if(order.getOrderType() == Type::Buy){
       while(!asks_.empty() && order.getOrderQuantity()>0){
        
        auto bestAsk_ = asks_.begin();
        Price bestAskPrice_ = bestAsk_->first;

        if(order.getOrderPrice() < bestAskPrice_){
            break;
        }
        
        auto& lqueue = bestAsk_->second.levelQueue;
        
        while(!lqueue.empty() && order.getOrderQuantity()>0){
            Order& resting = lqueue.front();

            Quantity traded = std::min(order.getOrderQuantity(), resting.getOrderQuantity());
            order.reduceQuantity(traded);
            resting.reduceQuantity(traded);
            bestAsk_->second.totalQuantity-=traded;

            if(resting.getOrderQuantity()==0){
                orderMap_.erase(resting.getOrderId());
                lqueue.pop_front();
            }
        }
        if(lqueue.empty()){
            asks_.erase(bestAsk_);
        }
       } 
    }
    else{
        while(!buys_.empty() && order.getOrderQuantity() >0){
            
            auto bestBuy_ = buys_.begin();
            Price bestBuyPrice_ = bestBuy_->first;

            if(order.getOrderPrice() > bestBuyPrice_){
                return;
            }

            auto& lqueue = bestBuy_->second.levelQueue;

            while(!lqueue.empty() && order.getOrderQuantity()>0){
                Order& resting  = lqueue.front();

                Quantity traded = std::min(order.getOrderQuantity(), resting.getOrderQuantity());

                order.reduceQuantity(traded);
                resting.reduceQuantity(traded);
                bestBuy_->second.totalQuantity -= traded;

                if(resting.getOrderQuantity() == 0){
                    orderMap_.erase(resting.getOrderId());
                    lqueue.pop_front();
                }
            }
            if(lqueue.empty()){
                buys_.erase(bestBuy_);
            }

        }
    }
}
