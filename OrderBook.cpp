#include "OrderBook.h"

OrderBook::OrderBook() :
    buys_(ARRAY_SIZE),
    asks_(ARRAY_SIZE),
    orderPool_(MAX_ORDERS),
    freeList_(MAX_ORDERS),
    freeListTop_(MAX_ORDERS),
    orderIdToPoolIdx_(MAX_ORDER_IDS, INVALID_IDX),
    bestBidIdx_(-1),
    bestAskIdx_(ARRAY_SIZE)
{
    for(size_t i = 0; i < ARRAY_SIZE; ++i) {
        buys_[i].levelPrice = MIN_PRICE + i;
        asks_[i].levelPrice = MIN_PRICE + i;
    }

    for(size_t i = 0; i < MAX_ORDERS; ++i) {
        freeList_[i] = (MAX_ORDERS - 1) - i;
    }
}

PoolIdx OrderBook::allocateNode() {
    if (freeListTop_ == 0) return INVALID_IDX; // Pool Exceeded
    return freeList_[--freeListTop_];
}

void OrderBook::deallocateNode(PoolIdx idx) {
    freeList_[freeListTop_++] = idx;
}

bool OrderBook::validateOrder(const Order& order) {
    OrderId id = order.getOrderId();
    if (id < 0 || id >= (int)MAX_ORDER_IDS || orderIdToPoolIdx_[id] != INVALID_IDX) return false;
    if (order.getOrderQuantity() <= 0) return false;
    
    Price p = order.getOrderPrice();
    return (p >= MIN_PRICE && p <= MAX_PRICE);
}

void OrderBook::intrusiveAppend(PriceLevel& level, PoolIdx newIdx) {
    if (level.tail_idx == INVALID_IDX) {
        level.head_idx = newIdx;
        level.tail_idx = newIdx;
        orderPool_[newIdx].prev_idx = INVALID_IDX;
        orderPool_[newIdx].next_idx = INVALID_IDX;
    } else {
        PoolIdx oldTail = level.tail_idx;
        orderPool_[oldTail].next_idx = newIdx;
        orderPool_[newIdx].prev_idx = oldTail;
        orderPool_[newIdx].next_idx = INVALID_IDX;
        level.tail_idx = newIdx;
    }
}

void OrderBook::intrusiveErase(PriceLevel& level, PoolIdx currIdx) {
    PoolIdx pIdx = orderPool_[currIdx].prev_idx;
    PoolIdx nIdx = orderPool_[currIdx].next_idx;

    if (currIdx == level.head_idx) {
        level.head_idx = nIdx;
    } else {
        orderPool_[pIdx].next_idx = nIdx;
    }

    if (currIdx == level.tail_idx) {
        level.tail_idx = pIdx;
    } else {
        orderPool_[nIdx].prev_idx = pIdx;
    }
}

void OrderBook::MatchOrder(Order& order) {
    if (order.getOrderType() == Type::Buy) {
        while (bestAskIdx_ < (int)ARRAY_SIZE && (MIN_PRICE + bestAskIdx_) <= order.getOrderPrice() && order.getOrderQuantity() > 0) {
            auto& level = asks_[bestAskIdx_];
            
            while (level.head_idx != INVALID_IDX && order.getOrderQuantity() > 0) {
                PoolIdx restingIdx = level.head_idx;
                PoolOrder& resting = orderPool_[restingIdx];

                Quantity traded = std::min(order.getOrderQuantity(), resting.qty);
                order.reduceQuantity(traded);
                resting.qty -= traded;
                level.totalQuantity -= traded;

                if (resting.qty == 0) {
                    orderIdToPoolIdx_[resting.id] = INVALID_IDX;
                    
                    level.head_idx = resting.next_idx;
                    if (level.head_idx == INVALID_IDX) {
                        level.tail_idx = INVALID_IDX;
                    } else {
                        orderPool_[level.head_idx].prev_idx = INVALID_IDX;
                    }
                    
                    deallocateNode(restingIdx);
                }
            }

            if (level.head_idx == INVALID_IDX) {
                bestAskIdx_++;
                while (bestAskIdx_ < (int)ARRAY_SIZE && asks_[bestAskIdx_].head_idx == INVALID_IDX) {
                    bestAskIdx_++;
                }
            }
        }
    } 
    else {
        while (bestBidIdx_ >= 0 && (MIN_PRICE + bestBidIdx_) >= order.getOrderPrice() && order.getOrderQuantity() > 0) {
            auto& level = buys_[bestBidIdx_];

            while (level.head_idx != INVALID_IDX && order.getOrderQuantity() > 0) {
                PoolIdx restingIdx = level.head_idx;
                PoolOrder& resting = orderPool_[restingIdx];

                Quantity traded = std::min(order.getOrderQuantity(), resting.qty);
                order.reduceQuantity(traded);
                resting.qty -= traded;
                level.totalQuantity -= traded;

                if (resting.qty == 0) {
                    orderIdToPoolIdx_[resting.id] = INVALID_IDX;

                    level.head_idx = resting.next_idx;
                    if (level.head_idx == INVALID_IDX) {
                        level.tail_idx = INVALID_IDX;
                    } else {
                        orderPool_[level.head_idx].prev_idx = INVALID_IDX;
                    }

                    deallocateNode(restingIdx);
                }
            }

            if (level.head_idx == INVALID_IDX) {
                bestBidIdx_--;
                while (bestBidIdx_ >= 0 && buys_[bestBidIdx_].head_idx == INVALID_IDX) {
                    bestBidIdx_--;
                }
            }
        }
    }
}

void OrderBook::AddOrder(Order& order) {
    MatchOrder(order);

    if (order.getOrderQuantity() > 0) {
        Price orderPrice = order.getOrderPrice();
        size_t idx = orderPrice - MIN_PRICE;
        OrderId id = order.getOrderId();

        PoolIdx newIdx = allocateNode();
        if (newIdx == INVALID_IDX) return; 

        orderPool_[newIdx].id = id;
        orderPool_[newIdx].price = orderPrice;
        orderPool_[newIdx].qty = order.getOrderQuantity();
        orderPool_[newIdx].type = order.getOrderType();

        orderIdToPoolIdx_[id] = newIdx;

        if (order.getOrderType() == Type::Buy) {
            auto& level = buys_[idx];
            intrusiveAppend(level, newIdx);
            level.totalQuantity += order.getOrderQuantity();

            if ((int)idx > bestBidIdx_) {
                bestBidIdx_ = idx;
            }
        } 
        else { 
            auto& level = asks_[idx];
            intrusiveAppend(level, newIdx);
            level.totalQuantity += order.getOrderQuantity();

            if ((int)idx < bestAskIdx_) {
                bestAskIdx_ = idx;
            }
        }
    }
}

void OrderBook::CancelOrder(OrderId orderId) {
    if (orderId < 0 || orderId >= (int)MAX_ORDER_IDS) return;
    
    PoolIdx currIdx = orderIdToPoolIdx_[orderId];
    if (currIdx == INVALID_IDX) return; 

    PoolOrder& target = orderPool_[currIdx];
    size_t idx = target.price - MIN_PRICE;

    if (target.type == Type::Buy) {
        auto& level = buys_[idx];
        level.totalQuantity -= target.qty;
        intrusiveErase(level, currIdx);

        if (level.head_idx == INVALID_IDX && (int)idx == bestBidIdx_) {
            bestBidIdx_--;
            while (bestBidIdx_ >= 0 && buys_[bestBidIdx_].head_idx == INVALID_IDX) {
                bestBidIdx_--;
            }
        }
    } 
    else { 
        auto& level = asks_[idx];
        level.totalQuantity -= target.qty;
        intrusiveErase(level, currIdx);

        if (level.head_idx == INVALID_IDX && (int)idx == bestAskIdx_) {
            bestAskIdx_++;
            while (bestAskIdx_ < (int)ARRAY_SIZE && asks_[bestAskIdx_].head_idx == INVALID_IDX) {
                bestAskIdx_++;
            }
        }
    }

    orderIdToPoolIdx_[orderId] = INVALID_IDX;
    deallocateNode(currIdx);
}
