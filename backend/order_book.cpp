#include <iostream>
#include <string>
#include <ctime>
#include <map>
#include <functional>
#include <utility>
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include <queue>
#include <chrono>
#include <thread> // For sleep_for

// Forward declarations
class Order;
class Trade;

// Enum for order types
enum class OrderType {
    LIMIT,
    MARKET
};

// Trade record for transaction logging
class Trade {
public:
    int buy_order_id;
    int sell_order_id;
    double price;
    unsigned int quantity;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    
    Trade(int buy_id, int sell_id, double p, unsigned int qty) 
        : buy_order_id(buy_id), sell_order_id(sell_id), price(p), quantity(qty),
          timestamp(std::chrono::steady_clock::now()) {}
    
    void repr() const {
        std::cout << "Trade: Buy Order " << buy_order_id 
                  << " | Sell Order " << sell_order_id
                  << " | Price: " << price 
                  << " | Quantity: " << quantity << std::endl;
    }
};

class Order {
    int order_id;
    double price;
    unsigned int quantity;
    bool side;
    std::time_t timestamp;
    OrderType order_type;
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiry_time;
    std::chrono::time_point<std::chrono::steady_clock> creation_time;
    /*
    Buy is true; sell is false
    */

public:
    // Enhanced constructor with order type and expiry
    Order(int id, double p, unsigned int qty, bool s, std::time_t t, 
          OrderType type = OrderType::LIMIT, 
          std::optional<std::chrono::seconds> ttl = std::nullopt) 
        : order_id(id), price(p), quantity(qty), side(s), timestamp(t), 
          order_type(type), creation_time(std::chrono::steady_clock::now()) {
        
        if (ttl.has_value()) {
            expiry_time = creation_time + ttl.value();
        }
    }

    void repr() const {
        std::cout   << "Order Id : " << order_id << ",\n"
                    << "Price : " << price << ",\n"
                    << "Quantity : " << quantity << ",\n"
                    << "Side : " << (side ? "Buy" : "Sell") << ",\n"
                    << "Type : " << (order_type == OrderType::MARKET ? "Market" : "Limit") << ",\n"
                    << "Timestamp : " << timestamp << "\n";
    }

    // compare two orders
    bool operator==(const Order& other) const {
        return (order_id == other.order_id) &&
                (price == other.price) &&
                (quantity == other.quantity) &&
                (side == other.side) &&
                (timestamp == other.timestamp);
    }
    
    // getters
    int get_order_id() const { return order_id; }
    double get_price() const { return price; }
    unsigned int get_qty() const { return quantity; }
    bool get_side() const { return side; }
    time_t get_timestamp() const { return timestamp; }
    OrderType get_order_type() const { return order_type; }
    std::chrono::time_point<std::chrono::steady_clock> get_creation_time() const { return creation_time; }
    
    // Check if order has expired
    bool is_expired() const {
        if (!expiry_time.has_value()) return false;
        return std::chrono::steady_clock::now() > expiry_time.value();
    }

    // public setters
    void set_price(double new_price) {
        change_price(new_price);
    }

    void set_qty(unsigned int new_qty) {
        change_qty(new_qty);
    }

    void set_side(bool new_side) {
        change_side(new_side);
    }

    void toggle_side() {
        side = 1 - side;
    }

    // deconstructor
    ~Order() {}

private:
    void change_price(double new_price) {
        price = new_price;
    }

    void change_qty(unsigned int new_quantity) {
        quantity = new_quantity;
    }

    void change_side(bool new_side) {
        side = new_side;
    }
};

// Custom comparator for buy orders (higher price first, then time priority)
struct BuyOrderComparator {
    bool operator()(const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& a,
                   const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& b) const {
        // Market orders have priority over limit orders
        if (a.first.get_order_type() != b.first.get_order_type()) {
            return a.first.get_order_type() == OrderType::LIMIT && b.first.get_order_type() == OrderType::MARKET;
        }
        
        // Higher price has priority for buy orders
        if (a.first.get_price() != b.first.get_price()) {
            return a.first.get_price() < b.first.get_price();
        }
        
        // Time priority (earlier orders first)
        return a.second > b.second;
    }
};

// Custom comparator for sell orders (lower price first, then time priority)
struct SellOrderComparator {
    bool operator()(const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& a,
                   const std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>& b) const {
        // Market orders have priority over limit orders
        if (a.first.get_order_type() != b.first.get_order_type()) {
            return a.first.get_order_type() == OrderType::LIMIT && b.first.get_order_type() == OrderType::MARKET;
        }
        
        // Lower price has priority for sell orders
        if (a.first.get_price() != b.first.get_price()) {
            return a.first.get_price() > b.first.get_price();
        }
        
        // Time priority (earlier orders first)
        return a.second > b.second;
    }
};

class OrderBook {
private:
    // Using priority queues for better performance
    std::priority_queue<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>, 
                       std::vector<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>>, 
                       BuyOrderComparator> buy_orders;
    
    std::priority_queue<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>, 
                       std::vector<std::pair<Order, std::chrono::time_point<std::chrono::steady_clock>>>, 
                       SellOrderComparator> sell_orders;
    
    std::unordered_map<int, std::shared_ptr<Order>> order_lookup;
    std::vector<Trade> trade_log;

    // Helper method to match orders
    std::vector<Trade> match_order(const Order& incoming_order) {
        std::vector<Trade> trades;
        Order working_order = incoming_order;
        
        if (working_order.get_side()) { // Buy order
            // Match against sell orders
            while (working_order.get_qty() > 0 && !sell_orders.empty()) {
                auto best_sell_pair = sell_orders.top();
                Order best_sell = best_sell_pair.first;
                
                // Check if expired
                if (best_sell.is_expired()) {
                    sell_orders.pop();
                    order_lookup.erase(best_sell.get_order_id());
                    continue;
                }
                
                // Check price condition for limit orders
                if (working_order.get_order_type() == OrderType::LIMIT && 
                    working_order.get_price() < best_sell.get_price()) {
                    break; // No match possible
                }
                
                // Execute trade
                unsigned int trade_qty = std::min(working_order.get_qty(), best_sell.get_qty());
                double trade_price = best_sell.get_price(); // Use the resting order's price
                
                Trade trade(working_order.get_order_id(), best_sell.get_order_id(), 
                           trade_price, trade_qty);
                trades.push_back(trade);
                
                // Update quantities
                working_order.set_qty(working_order.get_qty() - trade_qty);
                
                sell_orders.pop(); // Remove from queue
                
                if (best_sell.get_qty() > trade_qty) {
                    // Partial fill - update and re-add
                    best_sell.set_qty(best_sell.get_qty() - trade_qty);
                    sell_orders.push(std::make_pair(best_sell, best_sell_pair.second));
                    // Update in lookup
                    if (order_lookup.find(best_sell.get_order_id()) != order_lookup.end()) {
                        order_lookup[best_sell.get_order_id()]->set_qty(best_sell.get_qty());
                    }
                } else {
                    // Complete fill - remove from lookup
                    order_lookup.erase(best_sell.get_order_id());
                }
            }
        } else { // Sell order
            // Match against buy orders
            while (working_order.get_qty() > 0 && !buy_orders.empty()) {
                auto best_buy_pair = buy_orders.top();
                Order best_buy = best_buy_pair.first;
                
                // Check if expired
                if (best_buy.is_expired()) {
                    buy_orders.pop();
                    order_lookup.erase(best_buy.get_order_id());
                    continue;
                }
                
                // Check price condition for limit orders
                if (working_order.get_order_type() == OrderType::LIMIT && 
                    working_order.get_price() > best_buy.get_price()) {
                    break; // No match possible
                }
                
                // Execute trade
                unsigned int trade_qty = std::min(working_order.get_qty(), best_buy.get_qty());
                double trade_price = best_buy.get_price(); // Use the resting order's price
                
                Trade trade(best_buy.get_order_id(), working_order.get_order_id(), 
                           trade_price, trade_qty);
                trades.push_back(trade);
                
                // Update quantities
                working_order.set_qty(working_order.get_qty() - trade_qty);
                
                buy_orders.pop(); // Remove from queue
                
                if (best_buy.get_qty() > trade_qty) {
                    // Partial fill - update and re-add
                    best_buy.set_qty(best_buy.get_qty() - trade_qty);
                    buy_orders.push(std::make_pair(best_buy, best_buy_pair.second));
                    // Update in lookup
                    if (order_lookup.find(best_buy.get_order_id()) != order_lookup.end()) {
                        order_lookup[best_buy.get_order_id()]->set_qty(best_buy.get_qty());
                    }
                } else {
                    // Complete fill - remove from lookup
                    order_lookup.erase(best_buy.get_order_id());
                }
            }
        }
        
        // If there's remaining quantity and it's a limit order, add to book
        if (working_order.get_qty() > 0 && working_order.get_order_type() == OrderType::LIMIT) {
            add_order_to_book(working_order);
        }
        
        return trades;
    }
    
    void add_order_to_book(const Order& order) {
        std::shared_ptr<Order> order_ptr = std::make_shared<Order>(order);
        order_lookup[order.get_order_id()] = order_ptr;
        
        auto timestamp = std::chrono::steady_clock::now();
        
        if (order.get_side()) {
            buy_orders.push(std::make_pair(*order_ptr, timestamp));
        } else {
            sell_orders.push(std::make_pair(*order_ptr, timestamp));
        }
    }

public:
    // constructor
    OrderBook() = default;

    // Enhanced add_order method that handles matching
    std::vector<Trade> add_order(const Order& order) {
        // Check if order is expired before processing
        if (order.is_expired()) {
            std::cout << "Order " << order.get_order_id() << " is expired and will not be processed." << std::endl;
            return {};
        }
        
        std::vector<Trade> trades = match_order(order);
        
        // Log all trades
        for (const auto& trade : trades) {
            trade_log.push_back(trade);
            trade.repr();
        }
        
        return trades;
    }

    // Legacy method for backward compatibility
    void add_order_legacy(const Order& order) {
        add_order_to_book(order);
    }

    void remove_order(const int order_id) {
        order_lookup.erase(order_id);
        // Note: Orders in priority queues will be filtered out when accessed
        // This is more efficient than rebuilding the entire queue
    }

    // Clean expired orders
    void clean_expired_orders() {
        // This method would be called periodically
        // For now, expired orders are cleaned during matching
        std::cout << "Expired orders are cleaned automatically during matching." << std::endl;
    }

    void repr() {
        // Temporary containers to display current state
        std::vector<Order> current_buys, current_sells;
        
        // Extract non-expired orders
        auto temp_buy_queue = buy_orders;
        while (!temp_buy_queue.empty()) {
            auto order_pair = temp_buy_queue.top();
            temp_buy_queue.pop();
            if (!order_pair.first.is_expired() && 
                order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                current_buys.push_back(order_pair.first);
            }
        }
        
        auto temp_sell_queue = sell_orders;
        while (!temp_sell_queue.empty()) {
            auto order_pair = temp_sell_queue.top();
            temp_sell_queue.pop();
            if (!order_pair.first.is_expired() && 
                order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                current_sells.push_back(order_pair.first);
            }
        }
        
        std::cout << "\nBuy Orders:\n";
        for (const auto& order : current_buys) {
            order.repr();
            std::cout << "\n";
        }

        std::cout << "\nSell Orders:\n";
        for (const auto& order : current_sells) {
            order.repr();
            std::cout << "\n";
        }
    }

    void repr(bool side) {
        if (side) {
            std::cout << "\nBuy Orders:\n";
            auto temp_buy_queue = buy_orders;
            while (!temp_buy_queue.empty()) {
                auto order_pair = temp_buy_queue.top();
                temp_buy_queue.pop();
                if (!order_pair.first.is_expired() && 
                    order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                    order_pair.first.repr();
                    std::cout << "\n";
                }
            }
        } else {
            std::cout << "\nSell Orders:\n";
            auto temp_sell_queue = sell_orders;
            while (!temp_sell_queue.empty()) {
                auto order_pair = temp_sell_queue.top();
                temp_sell_queue.pop();
                if (!order_pair.first.is_expired() && 
                    order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                    order_pair.first.repr();
                    std::cout << "\n";
                }
            }
        }
    }

    Order get_best_bid() const {
        if (!buy_orders.empty()) {
            auto temp_queue = buy_orders;
            while (!temp_queue.empty()) {
                auto order_pair = temp_queue.top();
                temp_queue.pop();
                if (!order_pair.first.is_expired() && 
                    order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                    return order_pair.first;
                }
            }
        }
        throw std::runtime_error("No buy order available");
    }

    Order get_best_ask() const {
        if (!sell_orders.empty()) {
            auto temp_queue = sell_orders;
            while (!temp_queue.empty()) {
                auto order_pair = temp_queue.top();
                temp_queue.pop();
                if (!order_pair.first.is_expired() && 
                    order_lookup.find(order_pair.first.get_order_id()) != order_lookup.end()) {
                    return order_pair.first;
                }
            }
        }
        throw std::runtime_error("No sell order available");
    }

    // Enhanced update_order method
    void update_order(int order_id, std::optional<double> new_price, 
                     std::optional<unsigned int> new_qty, std::optional<bool> new_side) {
        auto it = order_lookup.find(order_id);
        if (it == order_lookup.end()) {
            std::cout << "Order not found\n";
            return;
        }

        auto order_ptr = it->second;
        
        // Remove the old order first
        remove_order(order_id);
        
        // Create updated order
        Order updated_order = *order_ptr;
        
        if (new_price.has_value()) {
            updated_order.set_price(new_price.value());
        }
        
        if (new_qty.has_value()) {
            updated_order.set_qty(new_qty.value());
        }
        
        if (new_side.has_value()) {
            updated_order.set_side(new_side.value());
        }
        
        // Re-add the updated order
        add_order(updated_order);
    }
    
    // Get trade history
    const std::vector<Trade>& get_trade_log() const {
        return trade_log;
    }
    
    // Repr trade history
    void repr_trade_log() const {
        std::cout << "\n=== Trade Log ===\n";
        for (const auto& trade : trade_log) {
            trade.repr();
        }
        std::cout << "================\n";
    }

private:
    void flip_order(int order_id) {
        auto it = order_lookup.find(order_id);
        if (it != order_lookup.end() && it->second) {
            Order temp = *(it->second);
            remove_order(order_id);
            temp.toggle_side();
            add_order(temp);
        }
    }
};

// Example usage demonstrating new features
int main() {
    OrderBook book;
    
    // Create some limit orders
    Order limit_buy(1, 100.0, 10, true, std::time(nullptr), OrderType::LIMIT);
    Order limit_sell(2, 101.0, 5, false, std::time(nullptr), OrderType::LIMIT);
    
    // Create a market order
    Order market_buy(3, 0.0, 7, true, std::time(nullptr), OrderType::MARKET);
    
    // Create order with expiry (expires in 5 seconds)
    Order expiring_order(4, 99.0, 3, true, std::time(nullptr), OrderType::LIMIT, 
                        std::chrono::seconds(5));
    
    std::cout << "Adding limit orders...\n";
    book.add_order(limit_buy);
    book.add_order(limit_sell);
    
    std::cout << "\nOrder book state:\n";
    book.repr();
    
    std::cout << "\nAdding market buy order (should match with sell order)...\n";
    auto trades = book.add_order(market_buy);
    
    std::cout << "\nOrder book state after market order:\n";
    book.repr();
    
    std::cout << "\nTrade log:\n";
    book.repr_trade_log();
    
    // Test is_expired
    std::cout << "\nTesting is_expired (wait 6 seconds for expiry):\n";
    std::this_thread::sleep_for(std::chrono::seconds(6));
    std::cout << "Order 4 expired? " << (expiring_order.is_expired() ? "Yes" : "No") << std::endl;

    // Test add_order_legacy
    std::cout << "\nTesting add_order_legacy:\n";
    Order legacy_order(5, 98.0, 2, false, std::time(nullptr), OrderType::LIMIT);
    book.add_order_legacy(legacy_order);
    book.repr();

    // Test remove_order
    std::cout << "\nTesting remove_order (removing order 5):\n";
    book.remove_order(5);
    book.repr();

    // Test update_order (change price and side of order 1)
    std::cout << "\nTesting update_order (change price and side of order 1):\n";
    book.update_order(1, 105.0, std::nullopt, false);
    book.repr();

    // Test clean_expired_orders
    std::cout << "\nTesting clean_expired_orders:\n";
    book.clean_expired_orders();

    // Test get_best_bid and get_best_ask
    try {
        auto best_bid = book.get_best_bid();
        std::cout << "\nBest Bid:\n";
        best_bid.repr();
    } catch (const std::exception& e) {
        std::cout << "No best bid: " << e.what() << std::endl;
    }
    try {
        auto best_ask = book.get_best_ask();
        std::cout << "\nBest Ask:\n";
        best_ask.repr();
    } catch (const std::exception& e) {
        std::cout << "No best ask: " << e.what() << std::endl;
    }

    // Test repr(bool side)
    std::cout << "\nTesting repr(true) for buy orders:\n";
    book.repr(true);
    std::cout << "\nTesting repr(false) for sell orders:\n";
    book.repr(false);

    // Test repr_trade_log
    std::cout << "\nTesting repr_trade_log:\n";
    book.repr_trade_log();
    
    return 0;
}