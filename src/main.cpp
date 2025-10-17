#include "../include/TradingSystemCore.h"
#include "../include/User.h"
#include "../include/Order.h"
#include "../include/Trade.h"
#include "../include/OrderBook.h"
#include "../include/TradeObserver.h"
#include "../include/TradingEngine.h"

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
        allTestsPassed &= testInvalidOrders();
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
