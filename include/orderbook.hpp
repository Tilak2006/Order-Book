#pragma once

#include "order.hpp"
#include "price_level.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double   price;
    uint32_t quantity;
};

class OrderBook {
public:
    std::vector<Trade> submit(Order order);
    bool cancel(uint64_t order_id);

    std::optional<double> best_bid() const;
    std::optional<double> best_ask() const;
    std::optional<double> spread()   const;

    uint32_t bid_quantity_at(double price) const;
    uint32_t ask_quantity_at(double price) const;

private:
    std::map<double, PriceLevel, std::greater<double>> bids_;
    std::map<double, PriceLevel>                       asks_;

    struct OrderLocation {
        double price;
        Side   side;
    };
    std::unordered_map<uint64_t, OrderLocation> order_map_;

    std::vector<Trade> match_limit(Order& order);
    std::vector<Trade> match_market(Order& order);

    template<typename MapType>
    std::vector<Trade> run_matching_loop(Order& order, MapType& passive_side) {
        std::vector<Trade> trades;

        while (order.quantity > 0 && !passive_side.empty()) {
            auto        best_it   = passive_side.begin();
            double      best_price = best_it->first;
            PriceLevel& level      = best_it->second;

            bool price_matches = (order.side == Side::BUY)
                ? (order.price >= best_price)
                : (order.price <= best_price);

            if (!price_matches) break;

            const Order& resting    = level.get_front();
            uint32_t     fill_qty   = std::min(order.quantity, resting.quantity);
            uint64_t     resting_id = resting.order_id;

            trades.push_back(Trade{
                .buy_order_id  = (order.side == Side::BUY)  ? order.order_id : resting_id,
                .sell_order_id = (order.side == Side::SELL) ? order.order_id : resting_id,
                .price         = best_price,
                .quantity      = fill_qty
            });

            order.quantity -= fill_qty;
            level.fill_front(fill_qty);

            if (level.is_empty()) {
                order_map_.erase(resting_id);
                passive_side.erase(best_it);
            }
        }

        return trades;
    }
};