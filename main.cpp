#include <iostream>
#include <iomanip>
#include "include/engine.hpp"

uint64_t next_id() {
    static uint64_t id = 1;
    return id++;
}

uint64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

Order make_order(Side side, OrderType type, double price, uint32_t qty) {
    return Order{
        .order_id  = next_id(),
        .timestamp = now_ns(),
        .price     = price,
        .quantity  = qty,
        .side      = side,
        .type      = type
    };
}

void print_trades(const std::vector<Trade>& trades) {
    if (trades.empty()) {
        std::cout << "  no trades\n";
        return;
    }
    for (const auto& t : trades) {
        std::cout << "  TRADE  buy_id=" << t.buy_order_id
                  << "  sell_id=" << t.sell_order_id
                  << "  price=" << std::fixed << std::setprecision(2) << t.price
                  << "  qty=" << t.quantity << "\n";
    }
}

void print_book(Engine& engine, const std::string& symbol) {
    std::cout << "\n  [" << symbol << " book]\n";

    auto bid = engine.best_bid(symbol);
    auto ask = engine.best_ask(symbol);
    auto spd = engine.spread(symbol);

    std::cout << "  best bid : " << (bid ? std::to_string(*bid) : "empty") << "\n";
    std::cout << "  best ask : " << (ask ? std::to_string(*ask) : "empty") << "\n";
    std::cout << "  spread   : " << (spd ? std::to_string(*spd) : "empty") << "\n";
}

int main() {
    Engine engine;

    std::cout << "========================================\n";
    std::cout << "  TEST 1 — limit orders, no match yet\n";
    std::cout << "========================================\n";
    auto t1 = engine.submit("AAPL", make_order(Side::BUY,  OrderType::LIMIT, 100.00, 200));
    auto t2 = engine.submit("AAPL", make_order(Side::BUY,  OrderType::LIMIT,  99.50, 300));
    auto t3 = engine.submit("AAPL", make_order(Side::SELL, OrderType::LIMIT, 101.00, 150));
    auto t4 = engine.submit("AAPL", make_order(Side::SELL, OrderType::LIMIT, 102.00, 400));
    print_trades(t1);
    print_trades(t2);
    print_trades(t3);
    print_trades(t4);
    print_book(engine, "AAPL");

    std::cout << "\n========================================\n";
    std::cout << "  TEST 2 — incoming buy crosses the ask\n";
    std::cout << "========================================\n";
    auto t5 = engine.submit("AAPL", make_order(Side::BUY, OrderType::LIMIT, 101.00, 150));
    print_trades(t5);
    print_book(engine, "AAPL");

    std::cout << "\n========================================\n";
    std::cout << "  TEST 3 — partial fill\n";
    std::cout << "========================================\n";
    auto t6 = engine.submit("AAPL", make_order(Side::BUY, OrderType::LIMIT, 102.00, 600));
    print_trades(t6);
    print_book(engine, "AAPL");

    std::cout << "\n========================================\n";
    std::cout << "  TEST 4 — market order\n";
    std::cout << "========================================\n";
    engine.submit("AAPL", make_order(Side::SELL, OrderType::LIMIT, 103.00, 500));
    engine.submit("AAPL", make_order(Side::SELL, OrderType::LIMIT, 104.00, 300));
    auto t7 = engine.submit("AAPL", make_order(Side::BUY, OrderType::MARKET, 0.0, 400));
    print_trades(t7);
    print_book(engine, "AAPL");

    std::cout << "\n========================================\n";
    std::cout << "  TEST 5 — cancel order\n";
    std::cout << "========================================\n";
    Order bid_to_cancel = make_order(Side::BUY, OrderType::LIMIT, 99.00, 1000);
    uint64_t cancel_id  = bid_to_cancel.order_id;
    engine.submit("AAPL", bid_to_cancel);
    std::cout << "  submitted bid id=" << cancel_id << " qty=1000 @ 99.00\n";
    print_book(engine, "AAPL");
    bool cancelled = engine.cancel("AAPL", cancel_id);
    std::cout << "  cancel result: " << (cancelled ? "SUCCESS" : "FAILED") << "\n";
    print_book(engine, "AAPL");

    std::cout << "\n========================================\n";
    std::cout << "  TEST 6 — second symbol, isolated book\n";
    std::cout << "========================================\n";
    engine.submit("RELIANCE", make_order(Side::BUY,  OrderType::LIMIT, 2500.00, 100));
    engine.submit("RELIANCE", make_order(Side::SELL, OrderType::LIMIT, 2500.00, 100));
    auto t8 = engine.submit("RELIANCE", make_order(Side::BUY, OrderType::LIMIT, 2500.00, 50));
    print_trades(t8);
    print_book(engine, "RELIANCE");
    print_book(engine, "AAPL");

    std::cout << "\n========================================\n";
    std::cout << "  ALL TESTS DONE\n";
    std::cout << "========================================\n";

    return 0;
}