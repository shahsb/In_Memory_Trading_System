#pragma once

#include "TradingSystemCore.h"
#include "Order.h"
#include "Trade.h"
#include <set>
#include <map>
#include <shared_mutex>

namespace TradingSystem {

    class OrderBook {
    private:
        Symbol symbol_;
        
        std::multiset<std::shared_ptr<Order>, BuyOrderComparator> buyOrders_;
        std::multiset<std::shared_ptr<Order>, SellOrderComparator> sellOrders_;
        
        mutable std::shared_mutex mutex_;
        std::map<OrderId, std::shared_ptr<Order>> orderLookup_;
        
    public:
        explicit OrderBook(const Symbol& symbol);
        
        bool addOrder(std::shared_ptr<Order> order);
        bool cancelOrder(const OrderId& orderId);
        bool modifyOrder(const OrderId& orderId, Quantity newQuantity, Price newPrice);
        std::shared_ptr<Order> getOrder(const OrderId& orderId) const;
        std::vector<std::shared_ptr<Order>> getBuyOrders() const;
        std::vector<std::shared_ptr<Order>> getSellOrders() const;
        
        // CORE MATCHING ENGINE - PRICE-TIME PRIORITY MATCHING ALGORITHM
        std::vector<std::shared_ptr<Trade>> matchOrders();
        
        Price getBestBid() const;
        Price getBestAsk() const;
        Price getSpread() const;
        
        bool isValid() const;
        const Symbol& getSymbol() const;
    };

} // namespace TradingSystem
