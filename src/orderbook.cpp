#include "../include/orderbook.hpp"

std::vector<Trade> OrderBook::submit(Order order) {
    if (order.type == OrderType::CANCEL) {
        cancel(order.order_id);
        return {};
    }
    if (order.type == OrderType::MARKET)
        return match_market(order);

    return match_limit(order);
}

std::vector<Trade> OrderBook::match_limit(Order& order) {
    std::vector<Trade> trades;

    if (order.side == Side::BUY)
        trades = run_matching_loop(order, asks_);
    else
        trades = run_matching_loop(order, bids_);

    if (order.quantity > 0) {
        if (order.side == Side::BUY) {
            bids_[order.price].add_order(order);
        } else {
            asks_[order.price].add_order(order);
        }
        order_map_[order.order_id] = {order.price, order.side};
    }

    return trades;
}

std::vector<Trade> OrderBook::match_market(Order& order) {
    if (order.side == Side::BUY)
        return run_matching_loop(order, asks_);
    else
        return run_matching_loop(order, bids_);
}

bool OrderBook::cancel(uint64_t order_id) {
    auto loc = order_map_.find(order_id);
    if (loc == order_map_.end()) return false;

    double price = loc->second.price;
    Side   side  = loc->second.side;

    if (side == Side::BUY) {
        auto level_it = bids_.find(price);
        if (level_it != bids_.end()) {
            level_it->second.cancel_order(order_id);
            if (level_it->second.is_empty())
                bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()) {
            level_it->second.cancel_order(order_id);
            if (level_it->second.is_empty())
                asks_.erase(level_it);
        }
    }

    order_map_.erase(loc);
    return true;
}

std::optional<double> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<double> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<double> OrderBook::spread() const {
    auto bid = best_bid();
    auto ask = best_ask();
    if (!bid || !ask) return std::nullopt;
    return *ask - *bid;
}

uint32_t OrderBook::bid_quantity_at(double price) const {
    auto it = bids_.find(price);
    if (it == bids_.end()) return 0;
    return it->second.total_quantity();
}

uint32_t OrderBook::ask_quantity_at(double price) const {
    auto it = asks_.find(price);
    if (it == asks_.end()) return 0;
    return it->second.total_quantity();
}