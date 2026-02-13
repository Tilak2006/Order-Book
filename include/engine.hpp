#pragma once

#include "orderbook.hpp"
#include <string>
#include <unordered_map>

class Engine {
public:
    std::vector<Trade> submit(const std::string& symbol, Order order);
    bool cancel(const std::string& symbol, uint64_t order_id);

    std::optional<double> best_bid(const std::string& symbol) const;
    std::optional<double> best_ask(const std::string& symbol) const;
    std::optional<double> spread(const std::string& symbol)   const;

private:
    std::unordered_map<std::string, OrderBook> books_;

    OrderBook* get_book(const std::string& symbol);
    const OrderBook* get_book_const(const std::string& symbol) const;
};