#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include "Order.h"
#include <atomic>
#include <vector>
#include <stdexcept>

class SPSCQueue {
private:
    // Isolate the write-head state entirely onto its own cache line
    alignas(64) std::atomic<size_t> tail_{0};
    size_t local_head_{0}; 

    // Isolate the read-head state entirely onto its own cache line
    alignas(64) std::atomic<size_t> head_{0};
    size_t local_tail_{0}; 

    // Isolate control metadata structures onto a separate cache line
    alignas(64) std::vector<Order> ringBuffer_;
    size_t capacity_;
    size_t mask_; 

public:
    SPSCQueue(size_t capacity) {
        // Enforce power-of-two allocation for bitwise optimization
        if ((capacity & (capacity - 1)) != 0 || capacity == 0) {
            throw std::invalid_argument("SPSCQueue capacity MUST be a power of 2!");
        }
        capacity_ = capacity;
        mask_ = capacity_ - 1;
        ringBuffer_.resize(capacity_);
    }

    // Called exclusively by Core 1 (Ingestion Thread)
    bool push(const Order& order) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) & mask_;
        
        // Explicit parentheses override operator precedence bugs
        if (next_tail == local_head_) {
            local_head_ = head_.load(std::memory_order_acquire); 
            if (next_tail == local_head_) {
                return false; // Queue is genuinely full
            }
        }

        ringBuffer_[current_tail] = order;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // Called exclusively by Core 2 (Matching Engine Thread)
    bool pop(Order& order) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == local_tail_) {
            local_tail_ = tail_.load(std::memory_order_acquire); 
            if (current_head == local_tail_) {
                return false; // Queue is genuinely empty
            }
        }

        order = ringBuffer_[current_head];
        head_.store((current_head + 1) & mask_, std::memory_order_release);
        return true;
    }
};

#endif
