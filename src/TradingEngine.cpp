#include "../include/TradingEngine.h"

namespace TradingSystem {

    TradingEngine* TradingEngine::instance_ = nullptr;
    std::mutex TradingEngine::instanceMutex_;
    
    TradingEngine& TradingEngine::getInstance() {
        std::lock_guard lock(instanceMutex_);
        if (!instance_) {
            instance_ = new TradingEngine();
        }
        return *instance_;
    }
    
    bool TradingEngine::registerUser(const std::shared_ptr<User>& user) {
        if (!user || !user->isValid()) return false;
        
        std::unique_lock lock(mutex_);
        auto result = users_.emplace(user->getUserId(), user);
        return result.second;
    }
    
    std::shared_ptr<User> TradingEngine::getUser(const UserId& userId) const {
        std::shared_lock lock(mutex_);
        auto it = users_.find(userId);
        return it != users_.end() ? it->second : nullptr;
    }
    
    std::shared_ptr<Order> TradingEngine::placeOrder(const UserId& userId, OrderType orderType,
                                         const Symbol& symbol, Quantity quantity, 
                                         Price price) {
        if (!getUser(userId)) return nullptr;
        
        OrderId orderId = generateUUID();
        std::unique_ptr<Order> order;
        
        // Validate price before creating order
        if (price < 0) {
            return nullptr; // Reject negative prices immediately
        }
        
        if (price > 0) {
            order = std::make_unique<LimitOrder>(orderId, userId, orderType, symbol, quantity, price);
        } else {
            order = std::make_unique<MarketOrder>(orderId, userId, orderType, symbol, quantity);
        }
        
        if (!order->isValid()) return nullptr;
        
        auto orderBook = getOrCreateOrderBook(symbol);
        if (!orderBook) return nullptr;
        
        auto sharedOrder = std::shared_ptr<Order>(order.release());
        
        // Store order in allOrders before adding to order book
        {
            std::unique_lock lock(mutex_);
            allOrders_[sharedOrder->getOrderId()] = sharedOrder;
        }
        
        if (orderBook->addOrder(sharedOrder)) {
            notifyOrderStatusChanged(sharedOrder);
            
            auto trades = orderBook->matchOrders();
            for (const auto& trade : trades) {
                notifyTradeExecuted(trade);
            }
            
            return sharedOrder;
        }
        
        return nullptr;
    }
    
    bool TradingEngine::cancelOrder(const UserId& userId, const OrderId& orderId) {
        if (!getUser(userId)) return false;
        
        // Check if order exists and belongs to user
        std::shared_ptr<Order> order;
        {
            std::shared_lock lock(mutex_);
            auto orderIt = allOrders_.find(orderId);
            if (orderIt == allOrders_.end() || orderIt->second->getUserId() != userId) {
                return false;
            }
            order = orderIt->second;
        }
        
        // Find the appropriate order book and cancel
        std::shared_lock lock(mutex_);
        for (const auto& [symbol, orderBook] : orderBooks_) {
            if (orderBook->getOrder(orderId)) {
                if (orderBook->cancelOrder(orderId)) {
                    notifyOrderStatusChanged(order);
                    return true;
                }
            }
        }
        
        return false;
    }
    
    bool TradingEngine::modifyOrder(const UserId& userId, const OrderId& orderId,
                        Quantity newQuantity, Price newPrice) {
        if (!getUser(userId)) return false;
        
        // Validate price before attempting modification
        if (newPrice < 0) {
            return false;
        }
        
        // First find the order and order book without holding locks for too long
        std::shared_ptr<Order> order;
        OrderBook* orderBook = nullptr;
        Symbol symbol;
        
        {
            std::shared_lock lock(mutex_);
            
            // Find order in allOrders
            auto orderIt = allOrders_.find(orderId);
            if (orderIt == allOrders_.end() || orderIt->second->getUserId() != userId) {
                return false;
            }
            order = orderIt->second;
            symbol = order->getSymbol();
            
            // Find the order book
            auto bookIt = orderBooks_.find(symbol);
            if (bookIt == orderBooks_.end()) {
                return false;
            }
            orderBook = bookIt->second.get();
        }
        
        // Perform modification
        if (orderBook->modifyOrder(orderId, newQuantity, newPrice)) {
            // Get the updated order
            auto modifiedOrder = orderBook->getOrder(orderId);
            if (modifiedOrder) {
                // Update allOrders with the modified order
                {
                    std::unique_lock lock(mutex_);
                    allOrders_[orderId] = modifiedOrder;
                }
                notifyOrderStatusChanged(modifiedOrder);
                
                // Try to match orders after modification
                auto trades = orderBook->matchOrders();
                for (const auto& trade : trades) {
                    notifyTradeExecuted(trade);
                }
            }
            return true;
        }
        
        return false;
    }
    
    std::shared_ptr<Order> TradingEngine::getOrderStatus(const UserId& userId, const OrderId& orderId) const {
        if (!getUser(userId)) return nullptr;
        
        std::shared_lock lock(mutex_);
        
        auto it = allOrders_.find(orderId);
        if (it != allOrders_.end() && it->second->getUserId() == userId) {
            return it->second;
        }
        
        return nullptr;
    }
    
    std::vector<std::shared_ptr<Order>> TradingEngine::getUserOrders(const UserId& userId) const {
        std::vector<std::shared_ptr<Order>> userOrders;
        
        if (!getUser(userId)) return userOrders;
        
        std::shared_lock lock(mutex_);
        
        for (const auto& [orderId, order] : allOrders_) {
            if (order->getUserId() == userId) {
                userOrders.push_back(order);
            }
        }
        
        return userOrders;
    }
    
    void TradingEngine::registerObserver(TradeObserver* observer) {
        std::unique_lock lock(mutex_);
        observers_.push_back(observer);
    }
    
    void TradingEngine::unregisterObserver(TradeObserver* observer) {
        std::unique_lock lock(mutex_);
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end()
        );
    }
    
    OrderBook* TradingEngine::getOrCreateOrderBook(const Symbol& symbol) {
        std::unique_lock lock(mutex_);
        auto it = orderBooks_.find(symbol);
        if (it == orderBooks_.end()) {
            auto orderBook = std::make_unique<OrderBook>(symbol);
            it = orderBooks_.emplace(symbol, std::move(orderBook)).first;
        }
        return it->second.get();
    }
    
    void TradingEngine::notifyTradeExecuted(const std::shared_ptr<Trade>& trade) {
        // Make a copy of observers to avoid holding lock during notification
        std::vector<TradeObserver*> observersCopy;
        {
            std::shared_lock lock(mutex_);
            observersCopy = observers_;
        }
        
        for (auto observer : observersCopy) {
            if (observer) {
                observer->onTradeExecuted(trade);
            }
        }
    }
    
    void TradingEngine::notifyOrderStatusChanged(const std::shared_ptr<Order>& order) {
        // Make a copy of observers to avoid holding lock during notification
        std::vector<TradeObserver*> observersCopy;
        {
            std::shared_lock lock(mutex_);
            observersCopy = observers_;
        }
        
        for (auto observer : observersCopy) {
            if (observer) {
                observer->onOrderStatusChanged(order);
            }
        }
    }

} // namespace TradingSystem
