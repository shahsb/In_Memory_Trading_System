#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <future>

// ============================================================================
// TRADING SYSTEM CORE IMPLEMENTATION
// ============================================================================

namespace TradingSystem {

    // DESIGN PRINCIPLE: Use strong typing with enums instead of primitive types
    enum class OrderType { BUY, SELL };
    enum class OrderStatus { PENDING, ACCEPTED, PARTIALLY_FILLED, FILLED, CANCELLED, REJECTED };
    enum class OrderTimeInForce { GTC, IOC, FOK }; // Good Till Cancel, Immediate or Cancel, Fill or Kill

    // DESIGN DECISION: Define system-wide constraints to prevent invalid states
    constexpr int MAX_ORDER_QUANTITY = 1000000;
    constexpr double MIN_ORDER_PRICE = 0.01;
    constexpr double MAX_ORDER_PRICE = 1000000.0;

    // DESIGN PRINCIPLE: Domain-Driven Design - use meaningful type names
    using UserId = std::string;
    using OrderId = std::string;
    using TradeId = std::string;
    using Symbol = std::string;
    using Quantity = int;
    using Price = double;
    using Timestamp = std::chrono::system_clock::time_point;

    // DESIGN PATTERN: Utility functions follow the Static Method pattern
    std::string generateUUID() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);
        
        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 8; i++) ss << dis(gen);
        ss << "-";
        for (int i = 0; i < 4; i++) ss << dis(gen);
        ss << "-4";
        for (int i = 0; i < 3; i++) ss << dis(gen);
        ss << "-";
        ss << dis2(gen);
        for (int i = 0; i < 3; i++) ss << dis(gen);
        ss << "-";
        for (int i = 0; i < 12; i++) ss << dis(gen);
        return ss.str();
    }

    Timestamp getCurrentTimestamp() {
        return std::chrono::system_clock::now();
    }

    // FORWARD DECLARATIONS
    class Order;
    class Trade;

    // USER CLASS - DOMAIN ENTITY
    class User {
    private:
        UserId userId_;
        std::string userName_;
        std::string phoneNumber_;
        std::string emailId_;
        
    public:
        User(const UserId& userId, const std::string& userName, 
             const std::string& phoneNumber, const std::string& emailId)
            : userId_(userId), userName_(userName), 
              phoneNumber_(phoneNumber), emailId_(emailId) {}
        
        const UserId& getUserId() const { return userId_; }
        const std::string& getUserName() const { return userName_; }
        const std::string& getPhoneNumber() const { return phoneNumber_; }
        const std::string& getEmailId() const { return emailId_; }
        
        bool isValid() const {
            return !userId_.empty() && !userName_.empty() && 
                   !phoneNumber_.empty() && !emailId_.empty();
        }
    };

    // ORDER BASE CLASS - ABSTRACT BASE CLASS USING TEMPLATE METHOD PATTERN
    class Order {
    protected:
        OrderId orderId_;
        UserId userId_;
        OrderType orderType_;
        Symbol symbol_;
        Quantity quantity_;
        Price price_;
        Timestamp timestamp_;
        OrderStatus status_;
        OrderTimeInForce timeInForce_;
        Quantity filledQuantity_;
        
    public:
        Order(const OrderId& orderId, const UserId& userId, OrderType orderType,
              const Symbol& symbol, Quantity quantity, Price price,
              OrderTimeInForce timeInForce = OrderTimeInForce::GTC)
            : orderId_(orderId), userId_(userId), orderType_(orderType),
              symbol_(symbol), quantity_(quantity), price_(price),
              timestamp_(getCurrentTimestamp()), status_(OrderStatus::PENDING),
              timeInForce_(timeInForce), filledQuantity_(0) {}
        
        virtual ~Order() = default;
        
        // GETTER METHODS
        const OrderId& getOrderId() const { return orderId_; }
        const UserId& getUserId() const { return userId_; }
        OrderType getOrderType() const { return orderType_; }
        const Symbol& getSymbol() const { return symbol_; }
        Quantity getQuantity() const { return quantity_; }
        Price getPrice() const { return price_; }
        const Timestamp& getTimestamp() const { return timestamp_; }
        OrderStatus getStatus() const { return status_; }
        OrderTimeInForce getTimeInForce() const { return timeInForce_; }
        Quantity getFilledQuantity() const { return filledQuantity_; }
        Quantity getRemainingQuantity() const { return quantity_ - filledQuantity_; }
        
        // SETTER METHODS WITH VALIDATION
        virtual bool setQuantity(Quantity newQuantity) {
            if (newQuantity <= 0 || newQuantity > MAX_ORDER_QUANTITY) return false;
            if (!canModify()) return false;
            quantity_ = newQuantity;
            return true;
        }
        
        virtual bool setPrice(Price newPrice) {
            if (newPrice < MIN_ORDER_PRICE || newPrice > MAX_ORDER_PRICE) return false;
            if (!canModify()) return false;
            price_ = newPrice;
            return true;
        }
        
        virtual bool setStatus(OrderStatus newStatus) {
            status_ = newStatus;
            return true;
        }
        
        // ORDER OPERATIONS
        virtual bool canModify() const {
            return status_ == OrderStatus::PENDING || status_ == OrderStatus::ACCEPTED;
        }
        
        virtual bool canCancel() const {
            return status_ == OrderStatus::PENDING || status_ == OrderStatus::ACCEPTED || 
                   status_ == OrderStatus::PARTIALLY_FILLED;
        }
        
        virtual void fill(Quantity fillQuantity) {
            if (fillQuantity <= getRemainingQuantity()) {
                filledQuantity_ += fillQuantity;
                if (filledQuantity_ == quantity_) {
                    status_ = OrderStatus::FILLED;
                } else if (filledQuantity_ > 0) {
                    status_ = OrderStatus::PARTIALLY_FILLED;
                }
            }
        }
        
        // VALIDATION METHOD
        virtual bool isValid() const {
            return !orderId_.empty() && !userId_.empty() && !symbol_.empty() &&
                   quantity_ > 0 && quantity_ <= MAX_ORDER_QUANTITY &&
                   price_ >= MIN_ORDER_PRICE && price_ <= MAX_ORDER_PRICE;
        }
        
        // PROTOTYPE PATTERN - VIRTUAL CLONE METHOD
        virtual std::unique_ptr<Order> clone() const = 0;
    };

    // LIMIT ORDER - CONCRETE IMPLEMENTATION
    class LimitOrder : public Order {
    public:
        LimitOrder(const OrderId& orderId, const UserId& userId, OrderType orderType,
                   const Symbol& symbol, Quantity quantity, Price price)
            : Order(orderId, userId, orderType, symbol, quantity, price) {}
        
        std::unique_ptr<Order> clone() const override {
            return std::make_unique<LimitOrder>(*this);
        }
    };

    // MARKET ORDER - CONCRETE IMPLEMENTATION WITH DIFFERENT VALIDATION
    class MarketOrder : public Order {
    public:
        MarketOrder(const OrderId& orderId, const UserId& userId, OrderType orderType,
                    const Symbol& symbol, Quantity quantity)
            : Order(orderId, userId, orderType, symbol, quantity, 0.0) {}
        
        std::unique_ptr<Order> clone() const override {
            return std::make_unique<MarketOrder>(*this);
        }
        
        // FIXED: Market orders should still validate price is not negative
        bool isValid() const override {
            return !orderId_.empty() && !userId_.empty() && !symbol_.empty() &&
                   quantity_ > 0 && quantity_ <= MAX_ORDER_QUANTITY &&
                   price_ >= 0; // Market orders can have 0 price but not negative
        }
        
        // FIXED: Market orders cannot set price (they execute at market price)
        bool setPrice(Price newPrice) override {
            return false; // Market orders cannot change price
        }
    };

    // TRADE CLASS - IMMUTABLE DOMAIN ENTITY
    class Trade {
    private:
        TradeId tradeId_;
        OrderType tradeType_;
        OrderId buyerOrderId_;
        OrderId sellerOrderId_;
        Symbol symbol_;
        Quantity quantity_;
        Price price_;
        Timestamp timestamp_;
        
    public:
        Trade(const TradeId& tradeId, OrderType tradeType,
              const OrderId& buyerOrderId, const OrderId& sellerOrderId,
              const Symbol& symbol, Quantity quantity, Price price)
            : tradeId_(tradeId), tradeType_(tradeType),
              buyerOrderId_(buyerOrderId), sellerOrderId_(sellerOrderId),
              symbol_(symbol), quantity_(quantity), price_(price),
              timestamp_(getCurrentTimestamp()) {}
        
        const TradeId& getTradeId() const { return tradeId_; }
        OrderType getTradeType() const { return tradeType_; }
        const OrderId& getBuyerOrderId() const { return buyerOrderId_; }
        const OrderId& getSellerOrderId() const { return sellerOrderId_; }
        const Symbol& getSymbol() const { return symbol_; }
        Quantity getQuantity() const { return quantity_; }
        Price getPrice() const { return price_; }
        const Timestamp& getTimestamp() const { return timestamp_; }
    };

    // ORDER COMPARATORS - STRATEGY PATTERN FOR DIFFERENT SORTING STRATEGIES
    struct BuyOrderComparator {
        bool operator()(const std::shared_ptr<Order>& lhs, 
                       const std::shared_ptr<Order>& rhs) const {
            if (std::abs(lhs->getPrice() - rhs->getPrice()) > 1e-9) {
                return lhs->getPrice() > rhs->getPrice();
            }
            return lhs->getTimestamp() < rhs->getTimestamp();
        }
    };

    struct SellOrderComparator {
        bool operator()(const std::shared_ptr<Order>& lhs, 
                       const std::shared_ptr<Order>& rhs) const {
            if (std::abs(lhs->getPrice() - rhs->getPrice()) > 1e-9) {
                return lhs->getPrice() < rhs->getPrice();
            }
            return lhs->getTimestamp() < rhs->getTimestamp();
        }
    };

    // ORDER BOOK CLASS - CORE MATCHING ENGINE WITH THREAD SAFETY
    class OrderBook {
    private:
        Symbol symbol_;
        
        std::multiset<std::shared_ptr<Order>, BuyOrderComparator> buyOrders_;
        std::multiset<std::shared_ptr<Order>, SellOrderComparator> sellOrders_;
        
        mutable std::shared_mutex mutex_;
        std::map<OrderId, std::shared_ptr<Order>> orderLookup_;
        
    public:
        explicit OrderBook(const Symbol& symbol) : symbol_(symbol) {}
        
        bool addOrder(std::shared_ptr<Order> order) {
            if (!order || order->getSymbol() != symbol_ || !order->isValid()) {
                return false;
            }
            
            std::unique_lock lock(mutex_);
            
            if (orderLookup_.find(order->getOrderId()) != orderLookup_.end()) {
                return false;
            }
            
            order->setStatus(OrderStatus::ACCEPTED);
            
            if (order->getOrderType() == OrderType::BUY) {
                buyOrders_.insert(order);
            } else {
                sellOrders_.insert(order);
            }
            
            orderLookup_[order->getOrderId()] = order;
            return true;
        }
        
        bool cancelOrder(const OrderId& orderId) {
            std::unique_lock lock(mutex_);
            
            auto it = orderLookup_.find(orderId);
            if (it == orderLookup_.end()) {
                return false;
            }
            
            auto order = it->second;
            if (!order->canCancel()) {
                return false;
            }
            
            bool removed = false;
            if (order->getOrderType() == OrderType::BUY) {
                for (auto iter = buyOrders_.begin(); iter != buyOrders_.end(); ++iter) {
                    if ((*iter)->getOrderId() == orderId) {
                        buyOrders_.erase(iter);
                        removed = true;
                        break;
                    }
                }
            } else {
                for (auto iter = sellOrders_.begin(); iter != sellOrders_.end(); ++iter) {
                    if ((*iter)->getOrderId() == orderId) {
                        sellOrders_.erase(iter);
                        removed = true;
                        break;
                    }
                }
            }
            
            if (removed) {
                order->setStatus(OrderStatus::CANCELLED);
                return true;
            }
            
            return false;
        }
        
        // FIXED: Order modification without deadlock
        bool modifyOrder(const OrderId& orderId, Quantity newQuantity, Price newPrice) {
            // First, find the order and validate without holding the lock for too long
            std::shared_ptr<Order> existingOrder;
            {
                std::shared_lock lock(mutex_);
                auto it = orderLookup_.find(orderId);
                if (it == orderLookup_.end()) {
                    return false;
                }
                existingOrder = it->second;
            }
            
            // Validate outside the lock
            if (!existingOrder->canModify()) {
                return false;
            }
            
            // Create the modified order
            auto modifiedOrder = existingOrder->clone();
            if (!modifiedOrder->setQuantity(newQuantity) || !modifiedOrder->setPrice(newPrice)) {
                return false;
            }
            
            // Now acquire exclusive lock and perform the swap
            std::unique_lock lock(mutex_);
            
            // Re-verify existence under lock
            auto it = orderLookup_.find(orderId);
            if (it == orderLookup_.end() || !it->second->canModify()) {
                return false;
            }
            
            // Remove old order
            auto order = it->second;
            bool removed = false;
            if (order->getOrderType() == OrderType::BUY) {
                for (auto iter = buyOrders_.begin(); iter != buyOrders_.end(); ++iter) {
                    if ((*iter)->getOrderId() == orderId) {
                        buyOrders_.erase(iter);
                        removed = true;
                        break;
                    }
                }
            } else {
                for (auto iter = sellOrders_.begin(); iter != sellOrders_.end(); ++iter) {
                    if ((*iter)->getOrderId() == orderId) {
                        sellOrders_.erase(iter);
                        removed = true;
                        break;
                    }
                }
            }
            
            if (!removed) {
                return false;
            }
            
            // Add modified order
            auto sharedModifiedOrder = std::shared_ptr<Order>(modifiedOrder.release());
            sharedModifiedOrder->setStatus(OrderStatus::ACCEPTED);
            if (sharedModifiedOrder->getOrderType() == OrderType::BUY) {
                buyOrders_.insert(sharedModifiedOrder);
            } else {
                sellOrders_.insert(sharedModifiedOrder);
            }
            
            // Update lookup with new order
            orderLookup_[orderId] = sharedModifiedOrder;
            
            return true;
        }
        
        std::shared_ptr<Order> getOrder(const OrderId& orderId) const {
            std::shared_lock lock(mutex_);
            auto it = orderLookup_.find(orderId);
            return it != orderLookup_.end() ? it->second : nullptr;
        }
        
        std::vector<std::shared_ptr<Order>> getBuyOrders() const {
            std::shared_lock lock(mutex_);
            return std::vector<std::shared_ptr<Order>>(buyOrders_.begin(), buyOrders_.end());
        }
        
        std::vector<std::shared_ptr<Order>> getSellOrders() const {
            std::shared_lock lock(mutex_);
            return std::vector<std::shared_ptr<Order>>(sellOrders_.begin(), sellOrders_.end());
        }
        
        // CORE MATCHING ENGINE - PRICE-TIME PRIORITY MATCHING ALGORITHM
        std::vector<std::shared_ptr<Trade>> matchOrders() {
            std::vector<std::shared_ptr<Trade>> trades;
            
            std::unique_lock lock(mutex_);
            
            while (!buyOrders_.empty() && !sellOrders_.empty()) {
                auto bestBuy = *buyOrders_.begin();
                auto bestSell = *sellOrders_.begin();
                
                if (bestBuy->getPrice() < bestSell->getPrice()) {
                    break;
                }
                
                Quantity tradeQuantity = std::min(bestBuy->getRemainingQuantity(), 
                                                bestSell->getRemainingQuantity());
                Price tradePrice = bestSell->getPrice();
                
                auto trade = std::make_shared<Trade>(
                    generateUUID(), OrderType::BUY,
                    bestBuy->getOrderId(), bestSell->getOrderId(),
                    symbol_, tradeQuantity, tradePrice
                );
                trades.push_back(trade);
                
                bestBuy->fill(tradeQuantity);
                bestSell->fill(tradeQuantity);
                
                if (bestBuy->getRemainingQuantity() == 0) {
                    buyOrders_.erase(buyOrders_.begin());
                }
                
                if (bestSell->getRemainingQuantity() == 0) {
                    sellOrders_.erase(sellOrders_.begin());
                }
            }
            
            return trades;
        }
        
        Price getBestBid() const {
            std::shared_lock lock(mutex_);
            return buyOrders_.empty() ? 0.0 : (*buyOrders_.begin())->getPrice();
        }
        
        Price getBestAsk() const {
            std::shared_lock lock(mutex_);
            return sellOrders_.empty() ? 0.0 : (*sellOrders_.begin())->getPrice();
        }
        
        Price getSpread() const {
            return getBestAsk() - getBestBid();
        }
        
        bool isValid() const { return !symbol_.empty(); }
        const Symbol& getSymbol() const { return symbol_; }
    };

    // OBSERVER INTERFACE - OBSERVER PATTERN FOR EVENT NOTIFICATIONS
    class TradeObserver {
    public:
        virtual ~TradeObserver() = default;
        virtual void onTradeExecuted(const std::shared_ptr<Trade>& trade) = 0;
        virtual void onOrderStatusChanged(const std::shared_ptr<Order>& order) = 0;
    };

    // TRADING ENGINE - SINGLETON PATTERN FOR GLOBAL ACCESS
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
        static TradingEngine& getInstance() {
            std::lock_guard lock(instanceMutex_);
            if (!instance_) {
                instance_ = new TradingEngine();
            }
            return *instance_;
        }
        
        TradingEngine(const TradingEngine&) = delete;
        TradingEngine& operator=(const TradingEngine&) = delete;
        
        bool registerUser(const std::shared_ptr<User>& user) {
            if (!user || !user->isValid()) return false;
            
            std::unique_lock lock(mutex_);
            auto result = users_.emplace(user->getUserId(), user);
            return result.second;
        }
        
        std::shared_ptr<User> getUser(const UserId& userId) const {
            std::shared_lock lock(mutex_);
            auto it = users_.find(userId);
            return it != users_.end() ? it->second : nullptr;
        }
        
        std::shared_ptr<Order> placeOrder(const UserId& userId, OrderType orderType,
                                         const Symbol& symbol, Quantity quantity, 
                                         Price price = 0.0) {
            if (!getUser(userId)) return nullptr;
            
            OrderId orderId = generateUUID();
            std::unique_ptr<Order> order;
            
            // FIXED: Validate price before creating order
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
        
        bool cancelOrder(const UserId& userId, const OrderId& orderId) {
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
        
        // FIXED: Order modification without deadlock
        bool modifyOrder(const UserId& userId, const OrderId& orderId,
                        Quantity newQuantity, Price newPrice) {
            if (!getUser(userId)) return false;
            
            // FIXED: Validate price before attempting modification
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
        
        std::shared_ptr<Order> getOrderStatus(const UserId& userId, const OrderId& orderId) const {
            if (!getUser(userId)) return nullptr;
            
            std::shared_lock lock(mutex_);
            
            auto it = allOrders_.find(orderId);
            if (it != allOrders_.end() && it->second->getUserId() == userId) {
                return it->second;
            }
            
            return nullptr;
        }
        
        std::vector<std::shared_ptr<Order>> getUserOrders(const UserId& userId) const {
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
        
        void registerObserver(TradeObserver* observer) {
            std::unique_lock lock(mutex_);
            observers_.push_back(observer);
        }
        
        void unregisterObserver(TradeObserver* observer) {
            std::unique_lock lock(mutex_);
            observers_.erase(
                std::remove(observers_.begin(), observers_.end(), observer),
                observers_.end()
            );
        }
        
    private:
        OrderBook* getOrCreateOrderBook(const Symbol& symbol) {
            std::unique_lock lock(mutex_);
            auto it = orderBooks_.find(symbol);
            if (it == orderBooks_.end()) {
                auto orderBook = std::make_unique<OrderBook>(symbol);
                it = orderBooks_.emplace(symbol, std::move(orderBook)).first;
            }
            return it->second.get();
        }
        
        void notifyTradeExecuted(const std::shared_ptr<Trade>& trade) {
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
        
        void notifyOrderStatusChanged(const std::shared_ptr<Order>& order) {
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
    };

    TradingEngine* TradingEngine::instance_ = nullptr;
    std::mutex TradingEngine::instanceMutex_;

} // namespace TradingSystem

// ============================================================================
// COMPREHENSIVE TEST SUITE
// ============================================================================

using namespace TradingSystem;

class TestObserver : public TradeObserver {
public:
    std::vector<std::shared_ptr<Trade>> executedTrades;
    std::vector<std::shared_ptr<Order>> statusChangedOrders;
    std::atomic<int> tradeCount{0};
    std::atomic<int> orderCount{0};
    
    void onTradeExecuted(const std::shared_ptr<Trade>& trade) override {
        executedTrades.push_back(trade);
        tradeCount++;
        std::cout << "[TEST] Trade Executed: " << trade->getSymbol() 
                  << " Qty: " << trade->getQuantity() 
                  << " Price: " << trade->getPrice() << std::endl;
    }
    
    void onOrderStatusChanged(const std::shared_ptr<Order>& order) override {
        statusChangedOrders.push_back(order);
        orderCount++;
        std::cout << "[TEST] Order Updated: " << order->getOrderId() 
                  << " Status: " << static_cast<int>(order->getStatus())
                  << " Remaining: " << order->getRemainingQuantity() << std::endl;
    }
    
    void reset() {
        executedTrades.clear();
        statusChangedOrders.clear();
        tradeCount = 0;
        orderCount = 0;
    }
};

bool testBasicOrderPlacement() {
    std::cout << "\n=== Test 1: Basic Order Placement ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U1", "Test User", "1234567890", "test@example.com");
    engine.registerUser(user);
    
    auto order = engine.placeOrder("U1", OrderType::BUY, "RELIANCE", 100, 2500.0);
    assert(order != nullptr);
    assert(order->getStatus() == OrderStatus::ACCEPTED);
    assert(order->getSymbol() == "RELIANCE");
    assert(order->getQuantity() == 100);
    assert(order->getPrice() == 2500.0);
    
    auto invalidOrder = engine.placeOrder("INVALID", OrderType::BUY, "RELIANCE", 100, 2500.0);
    assert(invalidOrder == nullptr);
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Basic Order Placement Test" << std::endl;
    return true;
}

bool testOrderMatching() {
    std::cout << "\n=== Test 2: Order Matching ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user1 = std::make_shared<User>("U2", "Buyer", "1111111111", "buyer@test.com");
    auto user2 = std::make_shared<User>("U3", "Seller", "2222222222", "seller@test.com");
    engine.registerUser(user1);
    engine.registerUser(user2);
    
    observer.reset();
    
    auto buyOrder = engine.placeOrder("U2", OrderType::BUY, "WIPRO", 100, 500.0);
    assert(buyOrder != nullptr);
    
    auto sellOrder = engine.placeOrder("U3", OrderType::SELL, "WIPRO", 100, 500.0);
    assert(sellOrder != nullptr);
    
    // Give some time for matching to occur
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    assert(observer.tradeCount > 0);
    if (observer.tradeCount > 0) {
        auto trade = observer.executedTrades[0];
        assert(trade->getQuantity() == 100);
        assert(trade->getPrice() == 500.0);
        assert(trade->getBuyerOrderId() == buyOrder->getOrderId());
        assert(trade->getSellerOrderId() == sellOrder->getOrderId());
    }
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Order Matching Test" << std::endl;
    return true;
}

bool testPriceTimePriority() {
    std::cout << "\n=== Test 3: Price-Time Priority ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U4", "Trader", "3333333333", "trader@test.com");
    engine.registerUser(user);
    
    observer.reset();
    
    auto order1 = engine.placeOrder("U4", OrderType::BUY, "INFY", 100, 1800.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto order2 = engine.placeOrder("U4", OrderType::BUY, "INFY", 100, 1800.0);
    
    auto sellOrder = engine.placeOrder("U4", OrderType::SELL, "INFY", 100, 1800.0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    assert(observer.tradeCount > 0);
    if (observer.tradeCount > 0) {
        auto trade = observer.executedTrades[0];
        assert(trade->getBuyerOrderId() == order1->getOrderId());
    }
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Price-Time Priority Test" << std::endl;
    return true;
}

bool testOrderCancellation() {
    std::cout << "\n=== Test 4: Order Cancellation ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U5", "Canceller", "4444444444", "cancel@test.com");
    engine.registerUser(user);
    
    observer.reset();
    
    auto order = engine.placeOrder("U5", OrderType::BUY, "TCS", 50, 3200.0);
    assert(order != nullptr);
    
    OrderId orderId = order->getOrderId();
    
    bool cancelResult = engine.cancelOrder("U5", orderId);
    assert(cancelResult);
    
    auto cancelledOrder = engine.getOrderStatus("U5", orderId);
    assert(cancelledOrder != nullptr);
    assert(cancelledOrder->getStatus() == OrderStatus::CANCELLED);
    
    bool cancelAgain = engine.cancelOrder("U5", orderId);
    assert(!cancelAgain);
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Order Cancellation Test" << std::endl;
    return true;
}

bool testOrderModification() {
    std::cout << "\n=== Test 5: Order Modification ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U6", "Modifier", "5555555555", "modify@test.com");
    engine.registerUser(user);
    
    observer.reset();
    
    auto order = engine.placeOrder("U6", OrderType::BUY, "HDFC", 100, 1500.0);
    assert(order != nullptr);
    
    OrderId orderId = order->getOrderId();
    
    // Wait a bit to ensure order is processed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    bool modifyResult = engine.modifyOrder("U6", orderId, 150, 1600.0);
    assert(modifyResult);
    
    auto modifiedOrder = engine.getOrderStatus("U6", orderId);
    assert(modifiedOrder != nullptr);
    assert(modifiedOrder->getQuantity() == 150);
    assert(modifiedOrder->getPrice() == 1600.0);
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Order Modification Test" << std::endl;
    return true;
}

bool testPartialOrderMatching() {
    std::cout << "\n=== Test 6: Partial Order Matching ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user1 = std::make_shared<User>("U7", "Big Buyer", "6666666666", "big@test.com");
    auto user2 = std::make_shared<User>("U8", "Small Seller", "7777777777", "small@test.com");
    engine.registerUser(user1);
    engine.registerUser(user2);
    
    observer.reset();
    
    auto buyOrder = engine.placeOrder("U7", OrderType::BUY, "SBIN", 1000, 600.0);
    assert(buyOrder != nullptr);
    
    OrderId buyOrderId = buyOrder->getOrderId();
    
    auto sellOrder1 = engine.placeOrder("U8", OrderType::SELL, "SBIN", 300, 600.0);
    auto sellOrder2 = engine.placeOrder("U8", OrderType::SELL, "SBIN", 400, 600.0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto updatedOrder = engine.getOrderStatus("U7", buyOrderId);
    assert(updatedOrder != nullptr);
    assert(updatedOrder->getStatus() == OrderStatus::PARTIALLY_FILLED);
    assert(updatedOrder->getFilledQuantity() == 700);
    assert(updatedOrder->getRemainingQuantity() == 300);
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Partial Order Matching Test" << std::endl;
    return true;
}

bool testInvalidOrders() {
    std::cout << "\n=== Test 7: Invalid Orders ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U9", "Edge Tester", "8888888888", "edge@test.com");
    engine.registerUser(user);
    
    observer.reset();
    
    auto zeroQtyOrder = engine.placeOrder("U9", OrderType::BUY, "RELIANCE", 0, 2500.0);
    assert(zeroQtyOrder == nullptr);
    
    // FIXED: Now this should correctly return nullptr for negative price
    auto negPriceOrder = engine.placeOrder("U9", OrderType::BUY, "RELIANCE", 100, -100.0);
    assert(negPriceOrder == nullptr);
    
    auto largeQtyOrder = engine.placeOrder("U9", OrderType::BUY, "RELIANCE", 10000000, 2500.0);
    assert(largeQtyOrder == nullptr);
    
    auto emptySymbolOrder = engine.placeOrder("U9", OrderType::BUY, "", 100, 2500.0);
    assert(emptySymbolOrder == nullptr);
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Invalid Orders Test" << std::endl;
    return true;
}

bool testConcurrency() {
    std::cout << "\n=== Test 8: Concurrency Stress Test ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U10", "Stress Tester", "9999999999", "stress@test.com");
    engine.registerUser(user);
    
    observer.reset();
    
    const int numThreads = 2;
    const int ordersPerThread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successfulOrders{0};
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < ordersPerThread; ++j) {
                OrderType type = (i % 2 == 0) ? OrderType::BUY : OrderType::SELL;
                auto order = engine.placeOrder("U10", type, "AXIS", 10, 1000.0 + j % 100);
                if (order && order->getStatus() == OrderStatus::ACCEPTED) {
                    successfulOrders++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Successful Orders: " << successfulOrders << std::endl;
    std::cout << "Executed Trades: " << observer.tradeCount << std::endl;
    
    assert(successfulOrders > 0);
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Concurrency Stress Test" << std::endl;
    return true;
}

bool testMarketDataQueries() {
    std::cout << "\n=== Test 9: Market Data Queries ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U11", "Data Query", "1010101010", "data@test.com");
    engine.registerUser(user);
    
    observer.reset();
    
    engine.placeOrder("U11", OrderType::BUY, "ICICI", 100, 950.0);
    engine.placeOrder("U11", OrderType::BUY, "ICICI", 200, 940.0);
    engine.placeOrder("U11", OrderType::SELL, "ICICI", 150, 960.0);
    engine.placeOrder("U11", OrderType::SELL, "ICICI", 100, 970.0);
    
    auto userOrders = engine.getUserOrders("U11");
    assert(userOrders.size() >= 4);
    
    if (!userOrders.empty()) {
        auto orderStatus = engine.getOrderStatus("U11", userOrders[0]->getOrderId());
        assert(orderStatus != nullptr);
        assert(orderStatus->getOrderId() == userOrders[0]->getOrderId());
    }
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Market Data Queries Test" << std::endl;
    return true;
}

bool testMultipleSymbols() {
    std::cout << "\n=== Test 10: Multiple Symbols ===" << std::endl;
    
    auto& engine = TradingEngine::getInstance();
    TestObserver observer;
    engine.registerObserver(&observer);
    
    auto user = std::make_shared<User>("U12", "Multi Symbol", "1212121212", "multi@test.com");
    engine.registerUser(user);
    
    observer.reset();
    
    auto order1 = engine.placeOrder("U12", OrderType::BUY, "TATASTEEL", 100, 120.0);
    auto order2 = engine.placeOrder("U12", OrderType::SELL, "TATAMOTORS", 50, 650.0);
    auto order3 = engine.placeOrder("U12", OrderType::BUY, "HINDALCO", 200, 450.0);
    
    assert(order1 != nullptr);
    assert(order2 != nullptr);
    assert(order3 != nullptr);
    
    auto userOrders = engine.getUserOrders("U12");
    assert(userOrders.size() >= 3);
    
    bool foundSteel = false, foundMotors = false, foundAlco = false;
    for (const auto& order : userOrders) {
        if (order->getSymbol() == "TATASTEEL") foundSteel = true;
        if (order->getSymbol() == "TATAMOTORS") foundMotors = true;
        if (order->getSymbol() == "HINDALCO") foundAlco = true;
    }
    
    assert(foundSteel && foundMotors && foundAlco);
    
    engine.unregisterObserver(&observer);
    std::cout << "PASS: Multiple Symbols Test" << std::endl;
    return true;
}

int main() {
    std::cout << "STARTING COMPREHENSIVE TRADING SYSTEM TESTS" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    bool allTestsPassed = true;
    
    try {
        // Run core functionality tests
        allTestsPassed &= testBasicOrderPlacement();
        allTestsPassed &= testOrderMatching();
        allTestsPassed &= testPriceTimePriority();
        allTestsPassed &= testOrderCancellation();
        allTestsPassed &= testOrderModification();
        allTestsPassed &= testPartialOrderMatching();
        allTestsPassed &= testInvalidOrders();  // This was failing due to negative price validation
        allTestsPassed &= testMarketDataQueries();
        allTestsPassed &= testMultipleSymbols();
        
        // Run concurrency test last
        allTestsPassed &= testConcurrency();
        
        std::cout << "\n===========================================" << std::endl;
        if (allTestsPassed) {
            std::cout << "ALL TESTS PASSED! Trading System is working correctly." << std::endl;
        } else {
            std::cout << "SOME TESTS FAILED! Please check the implementation." << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
