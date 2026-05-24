#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "Order.h"
#include "PriceLevel.h"

class OrderBook {
    private:
        // Core Arenas (Pre-allocated at boot)
        std::vector<PriceLevel> buys_;
        std::vector<PriceLevel> asks_;
        std::vector<PoolOrder> orderPool_;
        
        // Memory Management State
        std::vector<PoolIdx> freeList_;
        size_t freeListTop_; // Fast array stack pointer

        // O(1) Direct ID Index Map
        std::vector<PoolIdx> orderIdToPoolIdx_;

        // Best Price Trackers
        int bestBidIdx_;
        int bestAskIdx_;

    private:
        PoolIdx allocateNode();
        void deallocateNode(PoolIdx idx);
        void intrusiveAppend(PriceLevel& level, PoolIdx newIdx);
        void intrusiveErase(PriceLevel& level, PoolIdx currIdx);

    public:
        OrderBook();
        bool validateOrder(const Order& order);
        void MatchOrder(Order& order);
        void AddOrder(Order& order);
        void CancelOrder(OrderId orderId);
};

#endif
