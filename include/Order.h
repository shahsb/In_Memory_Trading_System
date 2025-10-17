#pragma once

#include "TradingSystemCore.h"
#include "User.h"
#include <memory>

namespace TradingSystem {

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
              OrderTimeInForce timeInForce = OrderTimeInForce::GTC);
        
        virtual ~Order() = default;
        
        // GETTER METHODS
        const OrderId& getOrderId() const;
        const UserId& getUserId() const;
        OrderType getOrderType() const;
        const Symbol& getSymbol() const;
        Quantity getQuantity() const;
        Price getPrice() const;
        const Timestamp& getTimestamp() const;
        OrderStatus getStatus() const;
        OrderTimeInForce getTimeInForce() const;
        Quantity getFilledQuantity() const;
        Quantity getRemainingQuantity() const;
        
        // SETTER METHODS WITH VALIDATION
        virtual bool setQuantity(Quantity newQuantity);
        virtual bool setPrice(Price newPrice);
        virtual bool setStatus(OrderStatus newStatus);
        
        // ORDER OPERATIONS
        virtual bool canModify() const;
        virtual bool canCancel() const;
        virtual void fill(Quantity fillQuantity);
        
        // VALIDATION METHOD
        virtual bool isValid() const;
        
        // PROTOTYPE PATTERN - VIRTUAL CLONE METHOD
        virtual std::unique_ptr<Order> clone() const = 0;
    };

    // LIMIT ORDER - CONCRETE IMPLEMENTATION
    class LimitOrder : public Order {
    public:
        LimitOrder(const OrderId& orderId, const UserId& userId, OrderType orderType,
                   const Symbol& symbol, Quantity quantity, Price price);
        
        std::unique_ptr<Order> clone() const override;
    };

    // MARKET ORDER - CONCRETE IMPLEMENTATION WITH DIFFERENT VALIDATION
    class MarketOrder : public Order {
    public:
        MarketOrder(const OrderId& orderId, const UserId& userId, OrderType orderType,
                    const Symbol& symbol, Quantity quantity);
        
        std::unique_ptr<Order> clone() const override;
        
        // Market orders should still validate price is not negative
        bool isValid() const override;
        
        // Market orders cannot set price (they execute at market price)
        bool setPrice(Price newPrice) override;
    };

    // ORDER COMPARATORS - STRATEGY PATTERN FOR DIFFERENT SORTING STRATEGIES
    struct BuyOrderComparator {
        bool operator()(const std::shared_ptr<Order>& lhs, 
                       const std::shared_ptr<Order>& rhs) const;
    };

    struct SellOrderComparator {
        bool operator()(const std::shared_ptr<Order>& lhs, 
                       const std::shared_ptr<Order>& rhs) const;
    };

} // namespace TradingSystem
