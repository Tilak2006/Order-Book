# High-Performance Order Book Matching Engine

A price-time priority matching engine written in C++17, implementing the core architecture used in production exchange systems. Processes **8M+ orders/sec** with **sub-250ns p99 latency**.

---

## Architecture

```
Engine
└── unordered_map<symbol, OrderBook>
        └── OrderBook
            ├── bids_  →  map<price, PriceLevel, greater<>>   (highest first)
            ├── asks_  →  map<price, PriceLevel, less<>>      (lowest first)
            └── order_map_  →  unordered_map<id, {price, side}>  (O(1) cancel index)
                    └── PriceLevel
                        ├── list<Order>                        (FIFO queue)
                        └── unordered_map<id, list::iterator>  (O(1) cancel)
```

Each layer has exactly one responsibility. The engine routes by symbol. The orderbook manages matching. PriceLevel manages the queue at a single price point.

---

## Order Types

| Type   | Behaviour |
|--------|-----------|
| LIMIT  | Rests in book at specified price if unmatched, or matches aggressively if price crosses |
| MARKET | Matches immediately against best available price, remainder cancelled |
| CANCEL | Removes a resting order by ID in O(1) |

---

## Matching Algorithm

Price-time priority — the standard used by most exchanges globally.

```
INCOMING BUY LIMIT @ $101, qty=300

  best ask = $100 (500 shares)
  101 >= 100 → MATCH 300 shares @ $100
  ask reduced to 200 shares
  buy fully filled → done

INCOMING BUY LIMIT @ $102, qty=700

  best ask = $100 (200 shares)
  102 >= 100 → MATCH 200 shares @ $100   ← level consumed, removed
  buy has 500 remaining

  best ask = $101 (400 shares)
  102 >= 101 → MATCH 400 shares @ $101   ← level consumed, removed
  buy has 100 remaining

  best ask = $103
  102 >= 103? NO → stop

  100 shares REST in bid book @ $102
```

---

## Key Design Decisions

**Why `std::list` for PriceLevel queues?**

Cancel requires O(1) middle deletion. `std::list` iterators remain valid after other elements are erased — storing an iterator per order_id in an `unordered_map` gives O(1) cancel. `std::vector` and `std::deque` invalidate iterators on removal, making this pattern impossible.

**Why asymmetric map comparators?**

```cpp
std::map<double, PriceLevel, std::greater<double>> bids_;  // highest first
std::map<double, PriceLevel>                       asks_;  // lowest first
```

Both sides expose their best price at `begin()`. The matching loop always reads `begin()` — no searching, no scanning. Best bid and best ask are O(1) lookups.

**Why a template matching loop?**

`bids_` and `asks_` are different types (different comparator template parameter). A `template<typename MapType>` function defined in the header works on both without code duplication and with zero runtime overhead — the compiler generates separate versions at compile time.

**Why a dual cancel index?**

`order_map_` in `OrderBook` stores `{price, side}` per order_id — so cancel(id) can locate the right PriceLevel in O(1). `index_` in `PriceLevel` stores the list iterator per order_id — so removal from the FIFO queue is also O(1). Total cancel complexity: O(1) end to end.

---

## Benchmark Results

500,000 operations per test, compiled with `-O3`.

| Test | Throughput | Median | p99 | p99.9 |
|------|-----------|--------|-----|-------|
| Limit orders (no match) | — | — | — | — |
| Limit orders (matching) | — | — | — | — |
| Market orders | — | — | — | — |
| Cancel orders | **8,055,099/sec** | **62 ns** | **190 ns** | **483 ns** |
| Mixed workload (40% rest, 30% match, 20% market, 10% cancel) | **8,646,401/sec** | **68 ns** | **212 ns** | **521 ns** |

The p99.9 latency spike visible in `--max` values is caused by `std::map` rebalancing during price level insertion. The production fix is replacing the tree with a flat array price ladder (slot = price × tick_size), eliminating rebalancing entirely at the cost of fixed memory allocation.

---

## Build & Run

**Requirements:** g++ with C++17, Linux (uses `clock_gettime`)

```bash
# Clone
git clone https://github.com/Tilak2006/orderbook.git
cd orderbook

# Build and run correctness tests
make && ./main

# Build and run benchmarks (O3)
make bench
```

---

## File Structure

```
orderbook/
├── include/
│   ├── order.hpp          # Order struct, Side and OrderType enums
│   ├── price_level.hpp    # FIFO queue at one price point, O(1) cancel
│   ├── orderbook.hpp      # Bid/ask maps, matching engine, template loop
│   └── engine.hpp         # Symbol router, manages multiple books
├── src/
│   ├── price_level.cpp
│   ├── orderbook.cpp
│   └── engine.cpp
├── benchmarks/
│   └── bench.cpp          # Latency and throughput measurements
├── main.cpp               # 6-scenario correctness test suite
└── Makefile
```

---

## What's Measured in main.cpp

1. Limit orders resting with no match — verifies book state
2. Incoming buy crosses the ask — single clean match
3. Partial fill — one order consuming multiple price levels
4. Market order — greedy execution, remainder cancelled
5. Cancel order — O(1) removal, book state verified before and after
6. Symbol isolation — AAPL and RELIANCE books are fully independent
