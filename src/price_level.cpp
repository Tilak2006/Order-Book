#include "../include/price_level.hpp"  // relative path up one folder
#include <cstdint>
#include <iterator>
#include <stdexcept>

void PriceLevel::add_order(const Order& order){
    orders_.push_back(order);
    index_[order.order_id] = std::prev(orders_.end());
    total_qty_ += order.quantity;
}

void PriceLevel::cancel_order(uint64_t order_id) {
    auto it = index_.find(order_id);
    if(it == index_.end()) return;

    total_qty_ -= it->second->quantity;
    orders_.erase(it->second); // O(1)
    index_.erase(it); // clean indx too
}

void PriceLevel::fill_front(uint32_t filled_quantity){
    Order& front = orders_.front();
    front.quantity -= filled_quantity;
    total_qty_ -= filled_quantity;

    if(front.quantity == 0) {
        index_.erase(front.order_id);
        orders_.pop_front();
    }
}
const Order& PriceLevel::get_front() const {
    if (orders_.empty()) {
        throw std::runtime_error("get_front() called on empty PriceLevel");
    }
    return orders_.front();
}

bool PriceLevel::is_empty() const {
    return orders_.empty();
}

uint32_t PriceLevel::total_quantity() const {
    return total_qty_;
}