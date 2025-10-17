#pragma once

#include "TradingSystemCore.h"
#include "Order.h"
#include "Trade.h"
#include <memory>

namespace TradingSystem {

    class TradeObserver {
    public:
        virtual ~TradeObserver() = default;
        virtual void onTradeExecuted(const std::shared_ptr<Trade>& trade) = 0;
        virtual void onOrderStatusChanged(const std::shared_ptr<Order>& order) = 0;
    };

} // namespace TradingSystem
