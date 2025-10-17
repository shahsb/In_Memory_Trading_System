#pragma once

#include "TradingSystemCore.h"
#include "Order.h"

namespace TradingSystem {

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
              const Symbol& symbol, Quantity quantity, Price price);
        
        const TradeId& getTradeId() const;
        OrderType getTradeType() const;
        const OrderId& getBuyerOrderId() const;
        const OrderId& getSellerOrderId() const;
        const Symbol& getSymbol() const;
        Quantity getQuantity() const;
        Price getPrice() const;
        const Timestamp& getTimestamp() const;
    };

} // namespace TradingSystem
