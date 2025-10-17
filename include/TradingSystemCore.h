#pragma once

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

    // Utility function declarations
    std::string generateUUID();
    Timestamp getCurrentTimestamp();

    // Forward declarations
    class User;
    class Order;
    class Trade;
    class OrderBook;
    class TradeObserver;
    class TradingEngine;

} // namespace TradingSystem
