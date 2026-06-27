# DistKV — In-Memory Key-Value Store

A Redis-like in-memory key-value store built from scratch in C++17.

## Features
- GET, SET, DEL, EXPIRE, PING over raw TCP sockets
- LRU eviction using hash map + doubly linked list
- Write-Ahead Logging (WAL)
- Fixed-size thread pool

## Build

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Run

```bash
./distkv
```

## Project Structure

- include/
- src/
- tests/
