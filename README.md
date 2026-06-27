# DistKV — In-Memory Key-Value Store

A Redis-like in-memory key-value store built from scratch in **C++17**.  
No external libraries — raw TCP sockets, POSIX threads, and STL only.

## Features
- `SET` `GET` `DEL` `EXPIRE` `PING` over raw TCP (port 6380)
- **LRU eviction** — O(1) using doubly linked list + hash map
- **Write-Ahead Logging (WAL)** — data survives server crashes and restarts
- **Fixed-size thread pool** — condition variables + producer-consumer queue,
  handles concurrent clients without unbounded thread creation

## Performance (localhost benchmark)

| Operation | avg  | p50  | p99   | max   |
|-----------|------|------|-------|-------|
| SET       | 39µs | 33µs | 113µs | 278µs |
| GET       | 26µs | 24µs | 70µs  | 198µs |

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
echo 'PING'              | nc 127.0.0.1 6380   # +PONG
echo 'SET name Moonlight'| nc 127.0.0.1 6380   # +OK
echo 'GET name'          | nc 127.0.0.1 6380   # $9 Moonlight
echo 'EXPIRE name 5'     | nc 127.0.0.1 6380   # :1
echo 'DEL name'          | nc 127.0.0.1 6380   # :1
```

## Benchmark
```bash
make bench && ./bench
```

## Architecture

### How a request flows through the system

```text
Client (nc / any TCP client)
        │
        │ raw TCP on port 6380
        ▼
┌─────────────────────────────┐
│         server.cpp          │
│     TCP accept() loop       │
│                             │
│ accept() → submit to pool   │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│      thread_pool.cpp        │
│   Fixed N worker threads    │
│ condition_variable queue    │
│   N = CPU core count        │
└──────────┬──────────────────┘
           │ worker picks up clientFd
           ▼
┌─────────────────────────────┐
│        parser.cpp           │
│   "SET name Rahul"          │
│ → ["SET","name","Rahul"]    │
└──────────┬──────────────────┘
           │
     ┌─────┴─────┐
     ▼           ▼
┌─────────┐  ┌────────────┐
│ wal.cpp │  │ store.cpp  │
│ log to  │  │ LRU hash   │
│ disk    │  │ map + mutex│
│ first   │  │            │
└─────────┘  └────────────┘
```

### Component responsibilities

| File | Responsibility |
|------|---------------|
| `server.cpp` | TCP socket setup, accept loop, WAL replay on startup, command dispatch |
| `thread_pool.cpp` | Fixed worker threads, condition variable sleep/wake, job queue |
| `store.cpp` | LRU eviction (`std::list` + `std::unordered_map`), mutex, TTL expiry |
| `wal.cpp` | Append-only command log, `flush()` on every write for durability |
| `parser.cpp` | Splits raw string into token vector |
| `bench.cpp` | TCP benchmark client that measures latency |

### Data structures inside `store.cpp`

```text
std::list<Node> lru_list_

[key5, val5] ← front (most recently used)
[key2, val2]
[key8, val8] ← back (least recently used)

std::unordered_map<string, list::iterator> map_

"key5" → points to front node
"key2" → points to middle node
"key8" → points to back node

std::unordered_map<string, time_point> expiry_

"key2" → expires at T + 30s
```

On every **GET** or **SET**, the accessed node is moved to the front in **O(1)** using `splice()`.

When the cache reaches capacity, the node at the back of the LRU list is evicted in **O(1)**.
