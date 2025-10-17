#pragma once

#include "TradingSystemCore.h"
#include "User.h"
#include "OrderBook.h"
#include "TradeObserver.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace TradingSystem {

    class TradingEngine {
    private:
        static TradingEngine* instance_;
        static std::mutex instanceMutex_;
        
        std::unordered_map<Symbol, std::unique_ptr<OrderBook>> orderBooks_;
        std::unordered_map<UserId, std::shared_ptr<User>> users_;
        std::vector<TradeObserver*> observers_;
        
        mutable std::shared_mutex mutex_;
        
        // Track all orders for status queries
        std::unordered_map<OrderId, std::shared_ptr<Order>> allOrders_;
        
        TradingEngine() = default;
        
    public:
        static TradingEngine& getInstance();
        
        TradingEngine(const TradingEngine&) = delete;
        TradingEngine& operator=(const TradingEngine&) = delete;
        
        bool registerUser(const std::shared_ptr<User>& user);
        std::shared_ptr<User> getUser(const UserId& userId) const;
        
        std::shared_ptr<Order> placeOrder(const UserId& userId, OrderType orderType,
                                         const Symbol& symbol, Quantity quantity, 
                                         Price price = 0.0);
        
        bool cancelOrder(const UserId& userId, const OrderId& orderId);
        bool modifyOrder(const UserId& userId, const OrderId& orderId,
                        Quantity newQuantity, Price newPrice);
        
        std::shared_ptr<Order> getOrderStatus(const UserId& userId, const OrderId& orderId) const;
        std::vector<std::shared_ptr<Order>> getUserOrders(const UserId& userId) const;
        
        void registerObserver(TradeObserver* observer);
        void unregisterObserver(TradeObserver* observer);
        
    private:
        OrderBook* getOrCreateOrderBook(const Symbol& symbol);
        void notifyTradeExecuted(const std::shared_ptr<Trade>& trade);
        void notifyOrderStatusChanged(const std::shared_ptr<Order>& order);
    };

} // namespace TradingSystem
