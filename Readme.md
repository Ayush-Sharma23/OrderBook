# Low-Latency Order Book 

A high-frequency trading mathing engine built in C++17.
The system uses a multi-threaded architecture to achieve sub-microsecond ene-to-end processing latencies over Linux TCP stack.

Throughput 22m+ messages/sec , average latency 40-45ns.

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


