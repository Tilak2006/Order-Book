#pragma once

#include "order.hpp"
#include <cstdint>
#include <list>
#include <unordered_map>

class PriceLevel {
public:
    void add_order(const Order& order);
    void cancel_order(uint64_t order_id);
    void fill_front(uint32_t filled_quantity);
    
    const Order& get_front() const;
    
    bool is_empty() const;
    uint32_t total_quantity() const;

private:
    // the _ says that this is pvt, naming convention only.
    std::list<Order> orders_;

    // O(1) cancel index
    // maps order_id -> iterator pointing into orders_ list
    std::unordered_map<uint64_t, std::list<Order>::iterator> index_;
    uint32_t total_qty_ = 0; // O(1) access
};