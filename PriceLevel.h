#ifndef PRICELEVEL_H
#define PRICELEVEL_H

#include "declaratives.h"

// The Raw Slot Node that occupies our pre-allocated Arena
struct PoolOrder {
    OrderId id = 0;
    Price price = 0;
    Quantity qty = 0;
    Type type = Type::Buy;

    // The Intrusive Doubly-Linked List Links
    PoolIdx next_idx = INVALID_IDX;
    PoolIdx prev_idx = INVALID_IDX;
};

// Simple boundary window tracking a linked sequence inside the pool
struct PriceLevel {
    Price levelPrice = 0;
    Quantity totalQuantity = 0;
    PoolIdx head_idx = INVALID_IDX; // Start of FIFO queue
    PoolIdx tail_idx = INVALID_IDX; // End of FIFO queue
};

#endif
