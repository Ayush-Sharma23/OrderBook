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

void OrderBook::AddOrder(Order& order){
    MatchOrder(order);

    if(order.getOrderQuantity() > 0){
        Price orderPrice = order.getOrderPrice();
        OrderId id = order.getOrderId();

    if(order.getOrderType() == Type::Buy){
            auto& level = buys_[orderPrice];
            level.levelQueue.push_back(order);
            level.totalQuantity += order.getOrderQuantity();

            auto it = std::prev(level.levelQueue.end());
            orderMap_[id] = {orderPrice, Type::Buy, it};
        }
    
    else{
            auto& level = asks_[orderPrice];
            level.levelQueue.push_back(order);
            level.totalQuantity += order.getOrderQuantity();

            auto it = std::prev(level.levelQueue.end());
            orderMap_[id] = {orderPrice, Type::Sell, it};
        }
    }
}

void OrderBook::CancelOrder(OrderId orderId){
    auto lookup = orderMap_.find(orderId);

    if(lookup == orderMap_.end()){
        //std::cout<<"Order Not Found!\n";
        return;
    }

    auto& details = lookup->second;

    if(details.type == Type::Buy){
        auto level = buys_.find(details.price);

        if(level != buys_.end()){
            level->second.totalQuantity -= details.it->getOrderQuantity();
            level->second.levelQueue.erase(details.it);
            
            if(level->second.levelQueue.empty()){
                buys_.erase(level);
            }
        }
    }
    else{
        auto level = asks_.find(details.price);

        if(level!= asks_.end()){
            level->second.totalQuantity -= details.it->getOrderQuantity();
            level->second.levelQueue.erase(details.it);

            if(level->second.levelQueue.empty()){
                asks_.erase(level);
            }
        }
    }
    orderMap_.erase(lookup);
}
