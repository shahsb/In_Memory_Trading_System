#include "../include/OrderBook.h"

namespace TradingSystem {

    OrderBook::OrderBook(const Symbol& symbol) : symbol_(symbol) {}
    
    bool OrderBook::addOrder(std::shared_ptr<Order> order) {
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
    
    bool OrderBook::cancelOrder(const OrderId& orderId) {
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
    
    bool OrderBook::modifyOrder(const OrderId& orderId, Quantity newQuantity, Price newPrice) {
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
    
    std::shared_ptr<Order> OrderBook::getOrder(const OrderId& orderId) const {
        std::shared_lock lock(mutex_);
        auto it = orderLookup_.find(orderId);
        return it != orderLookup_.end() ? it->second : nullptr;
    }
    
    std::vector<std::shared_ptr<Order>> OrderBook::getBuyOrders() const {
        std::shared_lock lock(mutex_);
        return std::vector<std::shared_ptr<Order>>(buyOrders_.begin(), buyOrders_.end());
    }
    
    std::vector<std::shared_ptr<Order>> OrderBook::getSellOrders() const {
        std::shared_lock lock(mutex_);
        return std::vector<std::shared_ptr<Order>>(sellOrders_.begin(), sellOrders_.end());
    }
    
    // CORE MATCHING ENGINE - PRICE-TIME PRIORITY MATCHING ALGORITHM
    std::vector<std::shared_ptr<Trade>> OrderBook::matchOrders() {
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
    
    Price OrderBook::getBestBid() const {
        std::shared_lock lock(mutex_);
        return buyOrders_.empty() ? 0.0 : (*buyOrders_.begin())->getPrice();
    }
    
    Price OrderBook::getBestAsk() const {
        std::shared_lock lock(mutex_);
        return sellOrders_.empty() ? 0.0 : (*sellOrders_.begin())->getPrice();
    }
    
    Price OrderBook::getSpread() const {
        return getBestAsk() - getBestBid();
    }
    
    bool OrderBook::isValid() const { return !symbol_.empty(); }
    const Symbol& OrderBook::getSymbol() const { return symbol_; }

} // namespace TradingSystem
