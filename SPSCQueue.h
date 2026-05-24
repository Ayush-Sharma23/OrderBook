#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include "Order.h"
#include <atomic>

class SPSCQueue{
    
    private:
        alignas(64) std::atomic<size_t> head_{0};
        alignas(64) std::atomic<size_t> tail_{0};

        std::vector<Order> ringBuffer_;
        size_t capacity_;

    public:
        SPSCQueue(size_t capacity) : ringBuffer_(capacity), capacity_(capacity){}

    // Called by the Ingestion Thread
    bool push(const Order& order) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t current_head = head_.load(std::memory_order_acquire);

        // Check if queue is full
        if ((current_tail + 1) % capacity_ == current_head) {
            return false; 
        }

        ringBuffer_[current_tail] = order;
        // Release memory barrier ensures data is written before tail index moves
        tail_.store((current_tail + 1) % capacity_, std::memory_order_release);
        return true;
    }

    // Called by the Matching Engine Thread
    bool pop(Order& order) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        size_t current_tail = tail_.load(std::memory_order_acquire);

        // Check if queue is empty
        if (current_head == current_tail) {
            return false; 
        }

        order = ringBuffer_[current_head];
        // Release barrier updates head state cleanly
        head_.store((current_head + 1) % capacity_, std::memory_order_release);
        return true;
    }
};


#endif 
