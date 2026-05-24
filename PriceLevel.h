#ifndef PRICELEVEL_H
#define PRICELEVEL_H

#include "declaratives.h"

// The Raw Slot Node that occupies our pre-allocated Arena
struct alignas(32) PoolOrder {
    OrderId id = 0;
    Price price = 0;
    Quantity qty = 0;
    Type type = Type::Buy;

    // The Intrusive Doubly-Linked List Links
    PoolIdx next_idx = INVALID_IDX;
    PoolIdx prev_idx = INVALID_IDX;

    // (4 ints/uint32_ts * 4 bytes = 16 bytes. 2 links*4bytes = 8bytes.)
    // Total = 24 bytes 
    // 64 - 24 = 40 bytes of padding 
    //uint8_t padding[40];
};

// Simple boundary window tracking a linked sequence inside the pool
struct alignas(32) PriceLevel {
    Price levelPrice = 0;
    Quantity totalQuantity = 0;
    PoolIdx head_idx = INVALID_IDX; // Start of FIFO queue
    PoolIdx tail_idx = INVALID_IDX; // End of FIFO queue
    
    // 4 fields * 4 bytes = 16 bytes. 64-16 = 48 bytes of padding.
    //uint8_t padding[48];
};

#endif
