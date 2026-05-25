#include "OrderBook.h"
#include "SPSCQueue.h"
#include "Protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <pthread.h>
#include <sched.h>

// Configuration Constants
const size_t TOTAL_BENCHMARK_ORDERS = 2'000'000; // 2 Million operations test suite
const size_t RING_BUFFER_CAPACITY = 262144;      // Pre-allocated lock-free ring slots (Must be Power of 2)
const int PORT_GATEWAY = 9999;

// Heap pointers to guarantee clean unmounting before application exit boundaries
std::unique_ptr<SPSCQueue> orderQueue;
std::unique_ptr<OrderBook> ob;

// Multi-threaded synchronization primitives
std::atomic<bool> networkRunning{true};
std::atomic<bool> clientFinished{false};

// Enforces Hardware Core Affinity
void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Configures socket descriptors to operate without synchronous blocking stalls
void make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Inline hardware-level micro-stall to mitigate Infinity Fabric thrashing
inline void cpu_relax() {
#if defined(__x86_64__) || defined(_M_X64)
    __builtin_ia32_pause();
#else
    std::this_thread::yield();
#endif
}

// =================================================================
// 1. HIGH-SPEED NATIVE BENCHMARKING CLIENT (Core 0 -> Logical 0)
// =================================================================
void native_benchmark_client_thread() {
    pin_thread_to_core(0); // Lock client to Physical Core 0
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Allow server to bind

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_GATEWAY);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "[Client Error] Connection to local matching engine failed.\n";
        return;
    }

    // Pre-allocate a contiguous memory arena to avoid hot-loop allocations
    std::vector<uint8_t> transmitBuffer;
    size_t packetSize = sizeof(PacketHeader) + sizeof(WireNewOrder);
    transmitBuffer.resize(TOTAL_BENCHMARK_ORDERS * packetSize);

    size_t offset = 0;
    for (size_t i = 0; i < TOTAL_BENCHMARK_ORDERS; ++i) {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(&transmitBuffer[offset]);
        header->type = MsgType::NewOrder;
        header->length = sizeof(WireNewOrder);

        WireNewOrder* wireOrd = reinterpret_cast<WireNewOrder*>(&transmitBuffer[offset + sizeof(PacketHeader)]);
        wireOrd->orderId = static_cast<int32_t>(i);
        wireOrd->price = static_cast<int32_t>(500 + (i % 10));
        wireOrd->quantity = 100;
        wireOrd->side = (i % 2 == 0) ? 0 : 1; // Alternating Buy/Sell to trigger instant matches

        offset += packetSize;
    }
    
    size_t totalBytesToWrite = transmitBuffer.size();
    size_t bytesWrittenSoFar = 0;
    
    while (bytesWrittenSoFar < totalBytesToWrite) {
        ssize_t chunk = write(client_fd, transmitBuffer.data() + bytesWrittenSoFar, totalBytesToWrite - bytesWrittenSoFar);
        if (chunk > 0) {
            bytesWrittenSoFar += chunk;
        }
    }

    clientFinished.store(true);
    close(client_fd);
}

// =================================================================
// 2. STATIC-ARENA NETWORK INGESTION ENGINE (Core 1 -> Logical 2)
// =================================================================
void live_network_ingestion_thread() {
    pin_thread_to_core(2); // Lock Ingestion to Physical Core 1 (Logical 2)
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_GATEWAY);
    
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);
    make_socket_non_blocking(server_fd);
    
    int client_fd = -1;
    
    // Low-latency alternative to std::vector. Fixed heap buffer to stop reallocations
    const size_t ARENA_SIZE = 256 * 1024; 
    uint8_t* inboundArena = new uint8_t[ARENA_SIZE];
    size_t writeOffset = 0;
    size_t readOffset = 0;

    while (networkRunning.load()) {
        if (client_fd == -1) {
            socklen_t addrlen = sizeof(address);
            client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
            if (client_fd != -1) {
                make_socket_non_blocking(client_fd);
            } else {
                if (clientFinished.load()) break;
                std::this_thread::yield();
               // cpu_relax();
                continue;
            }
        }

        ssize_t bytesRead = read(client_fd, inboundArena + writeOffset, ARENA_SIZE - writeOffset);
        
        if (bytesRead > 0) {
            writeOffset += bytesRead;
        } else if (bytesRead == 0 || (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            close(client_fd);
            client_fd = -1;
            writeOffset = 0;
            readOffset = 0;
            break;
        }

        // Parse zero-copy representations straight out of the byte stream arena
        while ((writeOffset - readOffset) >= sizeof(PacketHeader)) {
            PacketHeader* header = reinterpret_cast<PacketHeader*>(inboundArena + readOffset);
            size_t totalMessageSize = sizeof(PacketHeader) + header->length;

            if ((writeOffset - readOffset) < totalMessageSize) {
                break;
            }

            size_t payloadOffset = readOffset + sizeof(PacketHeader);
            if (header->type == MsgType::NewOrder) {
                WireNewOrder* wireOrd = reinterpret_cast<WireNewOrder*>(inboundArena + payloadOffset);
                Order ord(wireOrd->orderId, wireOrd->price, wireOrd->quantity, (wireOrd->side == 0) ? Type::Buy : Type::Sell);
                
                // Optimized spin-push utilizing hardware instruction stalls
                while (!orderQueue->push(ord)) [[unlikely]] { 
                    cpu_relax(); 
                }
            }
            
            readOffset += totalMessageSize;
        }

        // Shift remaining fragmentary bytes to the front of the block arena
        size_t unparsedBytes = writeOffset - readOffset;
        if (unparsedBytes > 0 && readOffset > 0) {
            std::memmove(inboundArena, inboundArena + readOffset, unparsedBytes);
            writeOffset = unparsedBytes;
            readOffset = 0;
        } else if (unparsedBytes == 0) {
            writeOffset = 0;
            readOffset = 0;
        }
    }

    if (client_fd != -1) close(client_fd);
    close(server_fd);
    
    delete[] inboundArena;
}

// =================================================================
// 3. TELEMETRY MATCHING ENGINE CORE (Core 2 -> Logical 4)
// =================================================================
void dedicated_matching_engine_thread() {
    pin_thread_to_core(4); // Lock Matching Engine to Physical Core 2 (Logical 4)
    
    Order incomingOrder;
    size_t handledOrders = 0;
    
    std::chrono::high_resolution_clock::time_point start_time;
    bool timer_started = false;

    while (true) {
        if (orderQueue->pop(incomingOrder)) {
            if (!timer_started) {
                start_time = std::chrono::high_resolution_clock::now();
                timer_started = true;
            }

            if (incomingOrder.getOrderQuantity() > 0) [[likely]] {
                ob->AddOrder(incomingOrder);
            } else {
                ob->CancelOrder(incomingOrder.getOrderId());
            }
            
            handledOrders++;
            
            if (handledOrders == TOTAL_BENCHMARK_ORDERS) {
                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = end_time - start_time;
                
                std::cout << "\n================= PRODUCTION SERVER BENCHMARK =================\n";
                std::cout << " Add & Match Operations (Over Live Network Loopback Link):\n";
                std::cout << "  Total Processed Count : " << handledOrders << " orders\n";
                std::cout << "  Total Execution Time  : " << elapsed.count() << " ms\n";
                std::cout << "  Engine Throughput     : " << (handledOrders / (elapsed.count() / 1000.0)) << " orders/sec\n";
                std::cout << "  Avg Network+Match Latency: " << (elapsed.count() * 1000.0 / handledOrders) << " microseconds/order\n";
                std::cout << "===============================================================\n";
                
                networkRunning.store(false);
                break;
            }
        } else {
            // Drop core instruction pressure when queue transitions to an empty state
            cpu_relax();
        }
    }
}

// =================================================================
// 4. COORDINATION LIFECYCLE CONTROLLER
// =================================================================
int main() {
    // Heap-allocate pointers to gain direct control over structural lifetimes
    orderQueue = std::make_unique<SPSCQueue>(RING_BUFFER_CAPACITY);
    ob = std::make_unique<OrderBook>();
    
    std::thread serverNetThread(live_network_ingestion_thread);
    std::thread serverMatchThread(dedicated_matching_engine_thread);
    std::thread clientDriverThread(native_benchmark_client_thread);
    
    // Synchronous execution checkpoint unmounting sequence
    clientDriverThread.join();
    serverNetThread.join();   
    serverMatchThread.join(); 
    
    orderQueue.reset();
    ob.reset();
    
    return 0;
}
