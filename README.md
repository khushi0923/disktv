# DistKV — In-Memory Key-Value Store

A Redis-like in-memory key-value store built from scratch in **C++17**.  
No libraries used — raw TCP sockets, POSIX threads, and STL only.

## Features
- `SET` `GET` `DEL` `EXPIRE` `PING` over raw TCP (port 6380)
- **LRU eviction** — O(1) using doubly linked list + hash map
- **Write-Ahead Logging (WAL)** — data survives server crashes
- **Fixed-size thread pool** — condition variables, producer-consumer queue,
  handles 10,000+ connections without unbounded thread creation

## Performance (localhost benchmark)
| Operation | avg | p50 | p99 | max |
|-----------|-----|-----|-----|-----|
| SET       | 39µs | 33µs | 113µs | 278µs |
| GET       | 26µs | 24µs |  70µs | 198µs |

## Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Run
```bash
./distkv
# [ThreadPool] Started 4 worker threads.
# [Server] DistKV listening on port 6380 with 4 worker threads.
```

## Test
```bash
echo 'PING'           | nc 127.0.0.1 6380   # +PONG
echo 'SET name Sohel' | nc 127.0.0.1 6380   # +OK
echo 'GET name'       | nc 127.0.0.1 6380   # $5 Rahul
echo 'EXPIRE name 5'  | nc 127.0.0.1 6380   # :1
echo 'DEL name'       | nc 127.0.0.1 6380   # :1
```

## Benchmark
```bash
make bench && ./bench
```

## Architecture
