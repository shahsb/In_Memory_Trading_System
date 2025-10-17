#include "../include/Trade.h"

namespace TradingSystem {

    Trade::Trade(const TradeId& tradeId, OrderType tradeType,
              const OrderId& buyerOrderId, const OrderId& sellerOrderId,
              const Symbol& symbol, Quantity quantity, Price price)
        : tradeId_(tradeId), tradeType_(tradeType),
          buyerOrderId_(buyerOrderId), sellerOrderId_(sellerOrderId),
          symbol_(symbol), quantity_(quantity), price_(price),
          timestamp_(getCurrentTimestamp()) {}
    
    const TradeId& Trade::getTradeId() const { return tradeId_; }
    OrderType Trade::getTradeType() const { return tradeType_; }
    const OrderId& Trade::getBuyerOrderId() const { return buyerOrderId_; }
    const OrderId& Trade::getSellerOrderId() const { return sellerOrderId_; }
    const Symbol& Trade::getSymbol() const { return symbol_; }
    Quantity Trade::getQuantity() const { return quantity_; }
    Price Trade::getPrice() const { return price_; }
    const Timestamp& Trade::getTimestamp() const { return timestamp_; }

} // namespace TradingSystem
