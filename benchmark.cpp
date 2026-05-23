#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include "OrderBook.h"

// Generates an explicit list of random orders before timing starts
std::vector<Order> generateMockOrders(int count, OrderId startId) {
    std::vector<Order> orders;
    orders.reserve(count);

    // Using a fixed seed ensures your benchmark runs are deterministic and reproducible
    std::mt19937 rng(42); 
    std::uniform_int_distribution<Price> priceDist(95, 105);      // Stock price swinging between 95 and 105
    std::uniform_int_distribution<Quantity> qtyDist(10, 500);     // Quantities between 10 and 500
    std::uniform_int_distribution<int> typeDist(0, 1);            // 0 = Buy, 1 = Sell

    for (int i = 0; i < count; ++i) {
        Type type = (typeDist(rng) == 0) ? Type::Buy : Type::Sell;
        orders.emplace_back(startId + i, priceDist(rng), qtyDist(rng), type);
    }

    return orders;
}

int main() {
    const int ORDER_COUNT = 100000; // 100,000 orders
    std::cout << "Pre-generating " << ORDER_COUNT << " mock orders...\n";
    auto orders = generateMockOrders(ORDER_COUNT, 1);

    OrderBook book;

    std::cout << "Starting OrderBook Add/Match Benchmark...\n";
    
    // --- 1. Benchmark Order Addition & Matching ---
    auto startMatch = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ORDER_COUNT; ++i) {
        // We use AddOrder since it processes matching first, then saves remainder
        book.AddOrder(orders[i]); 
    }

    auto endMatch = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> matchDuration = endMatch - startMatch;

    // --- 2. Benchmark Cancellation Performance ---
    // Try to cancel the first 20k orders submitted (some might already be fully filled)
    const int CANCEL_COUNT = 20000;
    std::cout << "Starting OrderBook Cancellation Benchmark (" << CANCEL_COUNT << " operations)...\n";
    
    auto startCancel = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < CANCEL_COUNT; ++i) {
        book.CancelOrder(orders[i].getOrderId());
    }

    auto endCancel = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> cancelDuration = endCancel - startCancel;

    // --- Performance Metrics Printout ---
    double matchSeconds = matchDuration.count() / 1000.0;
    double cancelSeconds = cancelDuration.count() / 1000.0;

    std::cout << "\n================= BENCHMARK PERFORMANCE RESULTS =================\n";
    std::cout << "Add & Match Operations:\n";
    std::cout << "  Total Execution Time : " << matchDuration.count() << " ms\n";
    std::cout << "  Throughput           : " << (ORDER_COUNT / matchSeconds) << " orders/sec\n";
    std::cout << "  Avg Latency          : " << (matchDuration.count() * 1000.0 / ORDER_COUNT) << " microseconds/order\n\n";

    std::cout << "Cancel Operations:\n";
    std::cout << "  Total Execution Time : " << cancelDuration.count() << " ms\n";
    std::cout << "  Throughput           : " << (CANCEL_COUNT / cancelSeconds) << " cancels/sec\n";
    std::cout << "  Avg Latency          : " << (cancelDuration.count() * 1000.0 / CANCEL_COUNT) << " microseconds/cancel\n";
    std::cout << "=================================================================\n";

    return 0;
}
