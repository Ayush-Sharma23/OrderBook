#include "OrderBook.h"
#include "SPSCQueue.h"
#include <thread>
#include <chrono>
#include <vector>
#include <pthread.h>
#include <sched.h>

const size_t BENCHMARK_COUNT = 1'000'000;
SPSCQueue orderQueue(65536); // Power of 2 ring buffer capacity
OrderBook ob;
std::atomic<bool> producerFinished{false};

void pin_thread(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Thread 1: Simulates Network Ingestion
void producer_thread() {
    pin_thread(1); // Pin Producer to Core 1
    
    for (size_t i = 0; i < BENCHMARK_COUNT; ++i) {
        Order order(i, 500 + (i % 10), 100, (i % 2 == 0) ? Type::Buy : Type::Sell);
        // Spin-wait if the lock-free queue temporarily fills up
        while (!orderQueue.push(order)) {
            std::this_thread::yield();
        }
    }
    producerFinished.store(true);
}

// Thread 2: Dedicated Core Matching Engine
void consumer_thread() {
    pin_thread(2); // Pin Matching Engine to Core 2
    
    Order incomingOrder(0, 0, 0, Type::Buy);
    while (true) {
        if (orderQueue.pop(incomingOrder)) {
            ob.AddOrder(incomingOrder);
        } else if (producerFinished.load()) {
            // Check one last time to drain queue
            if (!orderQueue.pop(incomingOrder)) break;
            ob.AddOrder(incomingOrder);
        }
    }
}

int main() {
//    std::cout << "Starting Lock-Free Asynchronous Matching Engine Benchmark...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread t1(producer_thread);
    std::thread t2(consumer_thread);
    
    t1.join();
    t2.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "==================================================\n";
    std::cout << "Multi-Threaded Execution Time: " << elapsed.count() << " ms\n";
    std::cout << "Throughput: " << (BENCHMARK_COUNT / (elapsed.count() / 1000.0)) << " orders/sec\n";
    std::cout << "==================================================\n";
    
    return 0;
}
