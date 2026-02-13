#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>
#include <ctime>
#include "../include/engine.hpp"

static uint64_t next_id() {
    static uint64_t id = 1;
    return id++;
}

static uint64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static Order make_order(Side side, OrderType type, double price, uint32_t qty) {
    return Order{
        .order_id  = next_id(),
        .timestamp = now_ns(),
        .price     = price,
        .quantity  = qty,
        .side      = side,
        .type      = type
    };
}

void print_stats(const std::string& label, std::vector<uint64_t>& latencies, uint64_t total_ns, size_t count) {
    std::sort(latencies.begin(), latencies.end());

    uint64_t min_lat    = latencies.front();
    uint64_t max_lat    = latencies.back();
    uint64_t median     = latencies[latencies.size() * 50 / 100];
    uint64_t p99        = latencies[latencies.size() * 99 / 100];
    uint64_t p999       = latencies[latencies.size() * 999 / 1000];
    double   avg        = (double)std::accumulate(latencies.begin(), latencies.end(), 0ULL) / count;
    double   throughput = (double)count / ((double)total_ns / 1e9);

    std::cout << "\n--- " << label << " ---\n";
    std::cout << "  orders          : " << count << "\n";
    std::cout << "  total time      : " << std::fixed << std::setprecision(3) << (double)total_ns / 1e6 << " ms\n";
    std::cout << "  throughput      : " << std::fixed << std::setprecision(0) << throughput << " orders/sec\n";
    std::cout << "  latency min     : " << min_lat << " ns\n";
    std::cout << "  latency avg     : " << std::fixed << std::setprecision(1) << avg << " ns\n";
    std::cout << "  latency median  : " << median << " ns\n";
    std::cout << "  latency p99     : " << p99 << " ns\n";
    std::cout << "  latency p99.9   : " << p999 << " ns\n";
    std::cout << "  latency max     : " << max_lat << " ns\n";
}

void bench_limit_no_match(size_t n) {
    Engine engine;
    std::vector<uint64_t> latencies;
    latencies.reserve(n);

    uint64_t start = now_ns();
    for (size_t i = 0; i < n; i++) {
        double price = 100.0 + (i % 50);
        Side side    = (i % 2 == 0) ? Side::BUY : Side::SELL;
        double buy_price  = price;
        double sell_price = price + 60.0;
        double final_price = (side == Side::BUY) ? buy_price : sell_price;

        uint64_t t0 = now_ns();
        engine.submit("AAPL", make_order(side, OrderType::LIMIT, final_price, 100));
        uint64_t t1 = now_ns();

        latencies.push_back(t1 - t0);
    }
    uint64_t total = now_ns() - start;

    print_stats("LIMIT ORDERS (no match, building book)", latencies, total, n);
}

void bench_limit_with_match(size_t n) {
    Engine engine;
    std::vector<uint64_t> latencies;
    latencies.reserve(n);

    for (size_t i = 0; i < 1000; i++) {
        double price = 100.0 + (i % 20);
        engine.submit("AAPL", make_order(Side::SELL, OrderType::LIMIT, price, 10000));
    }

    uint64_t start = now_ns();
    for (size_t i = 0; i < n; i++) {
        double price = 100.0 + (i % 20);

        uint64_t t0 = now_ns();
        engine.submit("AAPL", make_order(Side::BUY, OrderType::LIMIT, price, 10));
        uint64_t t1 = now_ns();

        latencies.push_back(t1 - t0);
    }
    uint64_t total = now_ns() - start;

    print_stats("LIMIT ORDERS (matching against resting asks)", latencies, total, n);
}

void bench_market_orders(size_t n) {
    Engine engine;
    std::vector<uint64_t> latencies;
    latencies.reserve(n);

    for (size_t i = 0; i < 500; i++) {
        double price = 100.0 + (i % 30);
        engine.submit("AAPL", make_order(Side::SELL, OrderType::LIMIT, price, 100000));
    }

    uint64_t start = now_ns();
    for (size_t i = 0; i < n; i++) {
        uint64_t t0 = now_ns();
        engine.submit("AAPL", make_order(Side::BUY, OrderType::MARKET, 0.0, 10));
        uint64_t t1 = now_ns();

        latencies.push_back(t1 - t0);
    }
    uint64_t total = now_ns() - start;

    print_stats("MARKET ORDERS (immediate execution)", latencies, total, n);
}

void bench_cancel_orders(size_t n) {
    Engine engine;
    std::vector<uint64_t> latencies;
    std::vector<uint64_t> ids;
    latencies.reserve(n);
    ids.reserve(n);

    for (size_t i = 0; i < n; i++) {
        double price  = 50.0 + (i % 100);
        Order o       = make_order(Side::BUY, OrderType::LIMIT, price, 100);
        ids.push_back(o.order_id);
        engine.submit("AAPL", o);
    }

    uint64_t start = now_ns();
    for (size_t i = 0; i < n; i++) {
        uint64_t t0 = now_ns();
        engine.cancel("AAPL", ids[i]);
        uint64_t t1 = now_ns();

        latencies.push_back(t1 - t0);
    }
    uint64_t total = now_ns() - start;

    print_stats("CANCEL ORDERS (O(1) cancel from live book)", latencies, total, n);
}

void bench_mixed_workload(size_t n) {
    Engine engine;
    std::vector<uint64_t> latencies;
    std::vector<uint64_t> live_ids;
    latencies.reserve(n);

    for (size_t i = 0; i < 200; i++) {
        double price = 99.0 + (i % 10);
        engine.submit("AAPL", make_order(Side::SELL, OrderType::LIMIT, price, 50000));
    }

    uint64_t start = now_ns();
    for (size_t i = 0; i < n; i++) {
        uint64_t t0 = now_ns();

        int roll = i % 10;

        if (roll < 4) {
            Order o = make_order(Side::BUY, OrderType::LIMIT, 98.0 + (i % 5), 100);
            live_ids.push_back(o.order_id);
            engine.submit("AAPL", o);
        } else if (roll < 7) {
            engine.submit("AAPL", make_order(Side::BUY, OrderType::LIMIT, 100.0 + (i % 5), 50));
        } else if (roll < 9) {
            engine.submit("AAPL", make_order(Side::BUY, OrderType::MARKET, 0.0, 20));
        } else {
            if (!live_ids.empty()) {
                engine.cancel("AAPL", live_ids.back());
                live_ids.pop_back();
            }
        }

        uint64_t t1 = now_ns();
        latencies.push_back(t1 - t0);
    }
    uint64_t total = now_ns() - start;

    print_stats("MIXED WORKLOAD (40% rest, 30% match, 20% market, 10% cancel)", latencies, total, n);
}

int main() {
    const size_t N = 500000;

    std::cout << "========================================\n";
    std::cout << "  ORDERBOOK BENCHMARK\n";
    std::cout << "  " << N << " operations per test\n";
    std::cout << "========================================";

    bench_limit_no_match(N);
    bench_limit_with_match(N);
    bench_market_orders(N);
    bench_cancel_orders(N);
    bench_mixed_workload(N);

    std::cout << "\n========================================\n";
    std::cout << "  BENCHMARK COMPLETE\n";
    std::cout << "========================================\n";

    return 0;
}