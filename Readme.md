# Low-Latency Order Book 

A high-frequency order matching engine built in C++17.
The system uses a multi-threaded architecture to achieve sub-microsecond end-to-end processing latencies over Linux TCP stack.

Throughput upto 24m+ messages/sec , average latency 40-45ns.

## Project Description

This project is an implementation of a classic Order Book designed around low-latency execution patterns along with a completely lock-free, asynchronous data pipeline. 

The architecture isolates runtime operations across distinct, hardware-pinned CPU cores:

* Core 0: Native Client Simulation (Ingestion Traffic Generator)
* Core 1: Live Network Ingestion Loop (TCP Binary Protocol Reassembler)
* Core 2: Core Matching Engine execution thread (Drives FIFO priority matching)

Communication between the ingestion pipeline and execution pipeline is handled via a custom single-producer single-consumer(SPSC) lock-free ring buffer. This isolates the core matching engine from network jitter and OS disruptions, maintaining predictable execution characteristics.
```text
[ HARDWARE ISOLATION BOUNDARY ]
                                  
 ┌───────────────────────────┐      Local TCP Loopback      ┌───────────────────────────────────┐
 │          CORE 0           │        Binary Stream         │              CORE 1               │
 │                           │                              │                                   │
 │  NATIVE SIMULATION CLIENT │─────────────────────────────►│   LIVE NETWORK INGESTION ENGINE   │
 │                           │                              │                                   │
 │ ── Allocates static arena │                              │  ── Allocates raw heap arena buffer│
 │ ── Packs binary structs   │                              │     (Eliminates vector resizing)  │
 │ ── Blasts 2M messages     │                              │  ── Reassembles TCP segments      │
 └───────────────────────────┘                              │  ── Zero-copy type casting via    │
                                                            │     reinterpret_cast pointers     │
                                                            └─────────────────┬─────────────────┘
                                                                              │
                                                                              │ Inbound Packed Binary
                                                                              │ (Order Type Entities)
                                                                              ▼
                                                            ┌───────────────────────────────────┐
                                                            │      LOCK-FREE SPSC QUEUE         │
                                                            │                                   │
                                                            │  ── Circular ring-buffer data     │
                                                            |     structure                     |                                  
                                                            │  ── head_ and tail_ counters      │
                                                            │     isolated via explicit         | 
                                                            │     alignas(64)                   |
                                                            │    (Eliminates cross-core L3 cache│
                                                            │      false sharing invalidation)  │
                                                            └─────────────────┬─────────────────┘
                                                                              │
                                                                              │ Lock-Free Spin-Pop
                                                                              ▼
                                                            ┌───────────────────────────────────┐
                                                            │              CORE 2               │
                                                            │                                   │
                                                            │      CORE MATCHING ENGINE         │
                                                            │                                   │
                                                            │  ── Monitored by Telemetry Timer  │
                                                            │  ── Mutates static OrderBook      │
                                                            │  ── Executes price-time priority  │
                                                            │  ── Drives zero-allocation loops  │
                                                            └───────────────────────────────────┘


```
## Core Matching Engine (`OrderBook`)

The core execution layer implements a deterministic, ultra-low-latency Limit Order Book (LOB) designed around price-time priority ($O(1)$ matching complexity). Traditional order books suffer from performance degradation due to pointer-chasing in linked lists or balance adjustments in tree structures (e.g., `std::map`). This architecture eliminates those bottlenecks by utilizing flat, cache-aligned contiguous memory arrays and indexed memory pools.



### Key Architectural Features

* **Constant-Time Level Lookup:** Price levels are mapped directly to array offsets or flat buckets. This converts typical $O(\log N)$ search paths into instant $O(1)$ direct index access, ensuring execution speeds remain stable regardless of order book depth.
* **Intra-Core Cache Locality:** Bids and asks are separated into independent, continuous memory structures. By tightly packing active order properties (price, quantity, order ID) and eliminating heap allocations during mutations, data fits perfectly inside the CPU's local L1/L2 data caches.
* **Zero-Allocation Internal Memory Pool:** To prevent memory fragmentation and operating system pauses during peak execution intervals, individual order nodes are stored inside a pre-allocated structural arena (`MemoryPool`). When an order is added, modified, or canceled, the engine recycles index blocks inline without invoking system memory management.

### Data Structures and Layout Boundaries

The internal memory parameters are defined inside `declaratives.h` to allocate adequate static memory for high-volume execution datasets:

```cpp
const size_t MAX_ORDERS = 2005000;      // Maximum active tracking capacity across the execution arena
const size_t MAX_ORDER_IDS = 2005000;   // Inbound index array boundary constraint
```

## Setup and Prerequisites

### Operating System 
Linux environment (Required for native `pthread` affinity extensions and non-blocking socket manipulation API headers).

### Toolchain 
GCC compiler supporting C++17 standards or above.

```bash
sudo apt-get update
sudo apt-get install build-essential gcc g++
```

### Compilation 
* Ensure all source files are placed in the same working directory.
 ```bash
 g++ -O3 -std=c++17 Order.cpp OrderBook.cpp benchmark.cpp -o orderbook_bench -lpthread
 ```
### Execution
 ```bash
./orderbook_bench
```
#### Note : 
execute the following script to run in elevated mode (Gives stable results)
```bash
sudo chrt -f 99 ./orderbook_bench
```

### Benchmark
The benchmark should appear like this -
``` text
================= PRODUCTION SERVER BENCHMARK =================
 Add & Match Operations (Over Live Network Loopback Link):
  Total Processed Count : 2000000 orders
  Total Execution Time  : 89.7268 ms
  Engine Throughput     : 2.22899e+07 orders/sec
  Avg Network+Match Latency: 0.0448634 microseconds/order
===============================================================
```

Performance Benchmark - 
```bash 
 perf stat -e cycles,instructions,cache-misses ./orderbook_bench
```
Results should look like this- 
```text 
================= PRODUCTION SERVER BENCHMARK =================
 Add & Match Operations (Over Live Network Loopback Link):
  Total Processed Count : 2000000 orders
  Total Execution Time  : 96.2921 ms
  Engine Throughput     : 2.07701e+07 orders/sec
  Avg Network+Match Latency: 0.0481461 microseconds/order
===============================================================

 Performance counter stats for './orderbook_bench':

     1,763,236,613      cycles:u                                                              
     3,550,407,726      instructions:u                                                        
         1,275,553      cache-misses:u                                                        

       0.678657050 seconds time elapsed

       0.772009000 seconds user
       0.538656000 seconds sys

```
