#include "OrderBook.h"

OrderBook::OrderBook():
    buys_(ARRAY_SIZE),
    asks_(ARRAY_SIZE),
    bestBidIdx_(-1),
    bestAskIdx_(ARRAY_SIZE)
{   
    for(size_t i=0; i<ARRAY_SIZE; ++i){
        buys_[i].levelPrice = MIN_PRICE + i;
        asks_[i].levelPrice = MIN_PRICE + i;
    }

}

bool OrderBook::validateOrder(Order order){
    if(orderMap_.find(order.getOrderId()) != orderMap_.end()){
        return false;
    }
    if(order.getOrderQuantity() <= 0){
        return false;
    }
    Price p = order.getOrderPrice();
    if(p < MIN_PRICE || p > MAX_PRICE){
        return false;
    }
    return true;
}

void OrderBook::MatchOrder(Order& order){
    if(!validateOrder(order)){
        //std::cerr<<"Invalid Order\n";
        return;
    }

    if(order.getOrderType() == Type::Buy){
      while (bestAskIdx_ < ARRAY_SIZE && (MIN_PRICE + bestAskIdx_) <= order.getOrderPrice() && order.getOrderQuantity() > 0){
        
        auto& level = asks_[bestAskIdx_];
        auto& lqueue = level.levelQueue;

 
            while(!lqueue.empty() && order.getOrderQuantity()>0){
                Order& resting  = lqueue.front();

                Quantity traded = std::min(order.getOrderQuantity(), resting.getOrderQuantity());

                order.reduceQuantity(traded);
                resting.reduceQuantity(traded);
                level.totalQuantity -= traded;

                if(resting.getOrderQuantity() == 0){
                    orderMap_.erase(resting.getOrderId());
                    lqueue.pop_front();
                }
            }

            if(lqueue.empty()){
                ++bestAskIdx_;
                while(bestAskIdx_ < ARRAY_SIZE && asks_[bestAskIdx_].levelQueue.empty()){
                    ++bestAskIdx_;
                }
            }        

      } 
    }
    else{
      while (bestBidIdx_ >=0 && (MIN_PRICE + bestBidIdx_) >= order.getOrderPrice() && order.getOrderQuantity() > 0){
        
        auto& level = buys_[bestBidIdx_];
        auto& lqueue = level.levelQueue;

 
            while(!lqueue.empty() && order.getOrderQuantity()>0){
                Order& resting  = lqueue.front();

                Quantity traded = std::min(order.getOrderQuantity(), resting.getOrderQuantity());

                order.reduceQuantity(traded);
                resting.reduceQuantity(traded);
                level.totalQuantity -= traded;

                if(resting.getOrderQuantity() == 0){
                    orderMap_.erase(resting.getOrderId());
                    lqueue.pop_front();
                }
            }

            if(lqueue.empty()){
                --bestBidIdx_;
                while(bestBidIdx_ >= 0 && buys_[bestBidIdx_].levelQueue.empty()){
                    --bestBidIdx_;
                }
        }        

      } 
    }
}

void OrderBook::AddOrder(Order& order){
    MatchOrder(order);

    if(order.getOrderQuantity() > 0){
        Price orderPrice = order.getOrderPrice();
        size_t idx = orderPrice - MIN_PRICE;
        OrderId id = order.getOrderId();

    if(order.getOrderType() == Type::Buy){
            auto& level = buys_[idx];
            level.levelQueue.push_back(order);
            level.totalQuantity += order.getOrderQuantity();

            auto it = std::prev(level.levelQueue.end());
            orderMap_[id] = {orderPrice, Type::Buy, it};

            if((int)idx > bestBidIdx_){
                bestBidIdx_ = idx;
            }
        }
    
    else{
            auto& level = asks_[idx];
            level.levelQueue.push_back(order);
            level.totalQuantity += order.getOrderQuantity();

            auto it = std::prev(level.levelQueue.end());
            orderMap_[id] = {orderPrice, Type::Sell, it};

            if ((int)idx < bestAskIdx_) {
                bestAskIdx_ = idx;
            }
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
    size_t idx = details.price - MIN_PRICE;

    if(details.type == Type::Buy){
        auto& level = buys_[idx];
        level.totalQuantity -= details.it->getOrderQuantity();
        level.levelQueue.erase(details.it);

        if(level.levelQueue.empty() && (int)idx == bestBidIdx_){
            --bestBidIdx_;
            while(bestBidIdx_ >= 0 && buys_[bestBidIdx_].levelQueue.empty()){
                --bestBidIdx_;
            }
        }
    }
    else{
        auto& level = asks_[idx];
        level.totalQuantity -= details.it->getOrderQuantity();
        level.levelQueue.erase(details.it);

        if(level.levelQueue.empty() && (int)idx == bestAskIdx_){
            ++bestAskIdx_;
            while(bestAskIdx_ < ARRAY_SIZE && asks_[bestAskIdx_].levelQueue.empty()){
                ++bestAskIdx_;
            }
        }
    }
    orderMap_.erase(lookup);
}
