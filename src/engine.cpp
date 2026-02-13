#include "../include/engine.hpp"

OrderBook* Engine::get_book(const std::string& symbol) {
    auto it = books_.find(symbol);
    if (it == books_.end()) return nullptr;
    return &it->second;
}

const OrderBook* Engine::get_book_const(const std::string& symbol) const {
    auto it = books_.find(symbol);
    if (it == books_.end()) return nullptr;
    return &it->second;
}

std::vector<Trade> Engine::submit(const std::string& symbol, Order order) {
    return books_[symbol].submit(order);
}

bool Engine::cancel(const std::string& symbol, uint64_t order_id) {
    OrderBook* book = get_book(symbol);
    if (!book) return false;
    return book->cancel(order_id);
}

std::optional<double> Engine::best_bid(const std::string& symbol) const {
    const OrderBook* book = get_book_const(symbol);
    if (!book) return std::nullopt;
    return book->best_bid();
}

std::optional<double> Engine::best_ask(const std::string& symbol) const {
    const OrderBook* book = get_book_const(symbol);
    if (!book) return std::nullopt;
    return book->best_ask();
}

std::optional<double> Engine::spread(const std::string& symbol) const {
    const OrderBook* book = get_book_const(symbol);
    if (!book) return std::nullopt;
    return book->spread();
}