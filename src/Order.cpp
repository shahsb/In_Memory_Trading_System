#include "../include/Order.h"

namespace TradingSystem {

    // Order base class implementation
    Order::Order(const OrderId& orderId, const UserId& userId, OrderType orderType,
              const Symbol& symbol, Quantity quantity, Price price,
              OrderTimeInForce timeInForce)
        : orderId_(orderId), userId_(userId), orderType_(orderType),
          symbol_(symbol), quantity_(quantity), price_(price),
          timestamp_(getCurrentTimestamp()), status_(OrderStatus::PENDING),
          timeInForce_(timeInForce), filledQuantity_(0) {}
    
    // GETTER METHODS
    const OrderId& Order::getOrderId() const { return orderId_; }
    const UserId& Order::getUserId() const { return userId_; }
    OrderType Order::getOrderType() const { return orderType_; }
    const Symbol& Order::getSymbol() const { return symbol_; }
    Quantity Order::getQuantity() const { return quantity_; }
    Price Order::getPrice() const { return price_; }
    const Timestamp& Order::getTimestamp() const { return timestamp_; }
    OrderStatus Order::getStatus() const { return status_; }
    OrderTimeInForce Order::getTimeInForce() const { return timeInForce_; }
    Quantity Order::getFilledQuantity() const { return filledQuantity_; }
    Quantity Order::getRemainingQuantity() const { return quantity_ - filledQuantity_; }
    
    // SETTER METHODS WITH VALIDATION
    bool Order::setQuantity(Quantity newQuantity) {
        if (newQuantity <= 0 || newQuantity > MAX_ORDER_QUANTITY) return false;
        if (!canModify()) return false;
        quantity_ = newQuantity;
        return true;
    }
    
    bool Order::setPrice(Price newPrice) {
        if (newPrice < MIN_ORDER_PRICE || newPrice > MAX_ORDER_PRICE) return false;
        if (!canModify()) return false;
        price_ = newPrice;
        return true;
    }
    
    bool Order::setStatus(OrderStatus newStatus) {
        status_ = newStatus;
        return true;
    }
    
    // ORDER OPERATIONS
    bool Order::canModify() const {
        return status_ == OrderStatus::PENDING || status_ == OrderStatus::ACCEPTED;
    }
    
    bool Order::canCancel() const {
        return status_ == OrderStatus::PENDING || status_ == OrderStatus::ACCEPTED || 
               status_ == OrderStatus::PARTIALLY_FILLED;
    }
    
    void Order::fill(Quantity fillQuantity) {
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
    bool Order::isValid() const {
        return !orderId_.empty() && !userId_.empty() && !symbol_.empty() &&
               quantity_ > 0 && quantity_ <= MAX_ORDER_QUANTITY &&
               price_ >= MIN_ORDER_PRICE && price_ <= MAX_ORDER_PRICE;
    }

    // LimitOrder implementation
    LimitOrder::LimitOrder(const OrderId& orderId, const UserId& userId, OrderType orderType,
               const Symbol& symbol, Quantity quantity, Price price)
        : Order(orderId, userId, orderType, symbol, quantity, price) {}
    
    std::unique_ptr<Order> LimitOrder::clone() const {
        return std::make_unique<LimitOrder>(*this);
    }

    // MarketOrder implementation
    MarketOrder::MarketOrder(const OrderId& orderId, const UserId& userId, OrderType orderType,
                const Symbol& symbol, Quantity quantity)
        : Order(orderId, userId, orderType, symbol, quantity, 0.0) {}
    
    std::unique_ptr<Order> MarketOrder::clone() const {
        return std::make_unique<MarketOrder>(*this);
    }
    
    bool MarketOrder::isValid() const {
        return !orderId_.empty() && !userId_.empty() && !symbol_.empty() &&
               quantity_ > 0 && quantity_ <= MAX_ORDER_QUANTITY &&
               price_ >= 0; // Market orders can have 0 price but not negative
    }
    
    bool MarketOrder::setPrice(Price newPrice) {
        return false; // Market orders cannot change price
    }

    // Order comparators implementation
    bool BuyOrderComparator::operator()(const std::shared_ptr<Order>& lhs, 
                       const std::shared_ptr<Order>& rhs) const {
        if (std::abs(lhs->getPrice() - rhs->getPrice()) > 1e-9) {
            return lhs->getPrice() > rhs->getPrice();
        }
        return lhs->getTimestamp() < rhs->getTimestamp();
    }

    bool SellOrderComparator::operator()(const std::shared_ptr<Order>& lhs, 
                       const std::shared_ptr<Order>& rhs) const {
        if (std::abs(lhs->getPrice() - rhs->getPrice()) > 1e-9) {
            return lhs->getPrice() < rhs->getPrice();
        }
        return lhs->getTimestamp() < rhs->getTimestamp();
    }

} // namespace TradingSystem
