#pragma once
#include <cstdint>

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    LIMIT,
    MARKET,
    CANCEL
};

struct Order {
    uint64_t order_id;
    uint64_t timestamp;
    double price;
    uint64_t quantity;
    Side side;
    OrderType type;
};