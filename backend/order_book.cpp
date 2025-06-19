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


class Order{
    int order_id;
    double price;
    unsigned int quantity;
    bool side;
    std::time_t timestamp;
    /*
    Buy is true; sell is false
    */

    public:
        // constructor
        Order(int id, double p, unsigned int qty, bool s, std::time_t t) : order_id(id), price(p),  quantity(qty), side(s), timestamp(t){};

        void repr() const {
            std::cout   << "Order Id : " << order_id << ",\n"
                        << "Price : " << price << ",\n"
                        << "Quantity : " << quantity << ",\n"
                        << "Side : " << (side ? "Buy" : "Sell") << ",\n"
                        << "Timestamp : " << timestamp << "\n";
        }

        // compare two orders
        bool operator==(const Order& other) const{
            return (order_id == other.order_id) &&
                    (price ==  other.price) &&
                    (quantity == other.quantity)&&
                    (side == other.side) &&
                    (timestamp == other.timestamp);
        }
        
        // getters
        int get_order_id() const {return order_id;}
        double get_price() const {return price;}
        unsigned int get_qty() const {return quantity;}
        bool get_side() const {return side;}
        time_t get_timestamp() const {return timestamp;}

        // public setters
        void set_price(double new_price){
            change_price(new_price);
        }

        void set_qty(unsigned int new_qty){
            change_qty(new_qty);
        }

        void set_side(bool new_side){
            change_side(new_side);
        }

        void  toggle_side(){
            side = 1 - side;
        }

        // deconstructor
        ~Order(){

        };
    private:
        void change_price(double new_price){
            price = new_price;
        }

        void change_qty(unsigned int new_quantity){
            quantity = new_quantity;
        }

        void change_side(bool new_side){
            side = new_side;
        }
        

};

class OrderBook{
    private:
        std::multimap<double, Order, std::greater<double>> buy_orders;
        std::multimap<double, Order, std::less<double>> sell_orders; 
        std::unordered_map<int, std::pair<std::shared_ptr<Order>, std::shared_ptr<Order>>> order_lookup;

    public:
        // constructor
        OrderBook() = default;

        void add_order(const Order& order){
            std::shared_ptr<Order> order_ptr = std::make_shared<Order>(order);

            if (order.get_side()){
                order_lookup[order.get_order_id()].first = order_ptr;
                buy_orders.insert(std::make_pair(order.get_price(), *order_ptr));
                return;
            }
            order_lookup[order.get_order_id()].second = order_ptr;
            sell_orders.insert(std::make_pair(order.get_price(), *order_ptr));
        }

        void remove_order(const int order_id){
            // Remove from buy_orders
            for(auto it = buy_orders.begin(); it != buy_orders.end(); ){
                if(it->second.get_order_id() == order_id){
                    it = buy_orders.erase(it);
                } else {
                    ++it;
                }
            }
            // Remove from sell_orders
            for(auto it = sell_orders.begin(); it != sell_orders.end(); ){
                if(it->second.get_order_id() == order_id){
                    it = sell_orders.erase(it);
                } else {
                    ++it;
                }
            }
            // Remove from lookup
            order_lookup.erase(order_id);
        }

        void repr(){
            // print buy first then print sell
            std::cout << "\nBuy Orders:\n";
            for(auto& order_item : buy_orders){
                order_item.second.repr();
                std::cout << "\n";

            }

            //print sell
            std::cout << "\nSell Orders:\n";
            for(auto& order_item : sell_orders){
                order_item.second.repr();
                std::cout << "\n";
            }
        }

        void repr(bool side){
            // if buy
            if(side){
                std::cout << "\nBuy Orders:\n";
                for(auto& order_item : buy_orders){
                    order_item.second.repr();
                    std::cout << "\n";

                }
                return;
            }

            //print sell
            std::cout << "\nSell Orders:\n";
            for(const auto& order_item : sell_orders){
                order_item.second.repr();
                std::cout << "\n";
            }
        }

        Order get_best_bid() const {
            if(!buy_orders.empty()){
                return buy_orders.begin() -> second;
            }
            throw std::runtime_error("No buy order available");
        }

        Order get_best_ask() const {
            if(!sell_orders.empty()){
                return sell_orders.begin() -> second;
            }
            throw std::runtime_error("No sell order available");
        }

        void update_order(int order_id, std::optional<double> new_price, std::optional<unsigned int> new_qty, std::optional<bool> new_side){
            auto& order_ptr = (order_lookup[order_id].first != nullptr) ? order_lookup[order_id].first : order_lookup[order_id].second;


            // checking if null ptr
            if(!order_ptr){
                std::cout << "Order not found\n";
                return;
            }

            if (new_price.has_value()) {
                double old_price = order_ptr->get_price();
                order_ptr->set_price(new_price.value());
                if (order_ptr->get_side()) {
                    // Remove only the specific order
                    for (auto it = buy_orders.begin(); it != buy_orders.end(); ++it) {
                        if (it->second.get_order_id() == order_ptr->get_order_id()) {
                            buy_orders.erase(it);
                            break;
                        }
                    }
                    buy_orders.insert(std::make_pair(order_ptr->get_price(), *order_ptr));
                } else {
                    for (auto it = sell_orders.begin(); it != sell_orders.end(); ++it) {
                        if (it->second.get_order_id() == order_ptr->get_order_id()) {
                            sell_orders.erase(it);
                            break;
                        }
                    }
                    sell_orders.insert(std::make_pair(order_ptr->get_price(), *order_ptr));
                }
            }            

            if(new_qty.has_value()){
                remove_order(order_id);
                order_ptr->set_qty(new_qty.value());
                add_order(*order_ptr);
            }

            if(new_side.has_value()){
                if(new_side.value() == order_ptr->get_side()){
                    std::cout << "No change in side. No side change will be made\n";
                } else{
                    Order temp = *order_ptr; // Make a copy
                    temp.set_side(new_side.value());
                    remove_order(order_id);
                    add_order(temp);
                }
                
                
            }
        }
    
    private:
        void flip_order(int order_id){
            auto& order_ptr = order_lookup[order_id].first ? order_lookup[order_id].first : order_lookup[order_id].second;
            // bool curr_side = order_lookup[order_id].second == nullptr ? order_lookup[order_id].first->get_side() : order_lookup[order_id].second->get_side();
            
            /*
            Order change_order = *order_ptr;
            change_order.toggle_side();

            remove_order(order_id);
            add_order(change_order);
            */
           order_ptr->toggle_side();
           remove_order(order_id);
           add_order(*order_ptr);
        }

    };

int main(){
    OrderBook order_book;

    std::time_t now = std::time(0);

    // Adding orders with the same price
    Order order1(1, 100.68, 10, true, now);  // buy
    Order order2(2, 102.5, 15, false, now);  // sell
    Order order3(3, 100.68, 20, true, now);  // buy (same price as order1)
    Order order4(4, 101.0, 30, false, now); // sell
    Order order5(5, 100.68, 25, true, now);  // buy (same price as order1)
    Order order6(6, 438.93, 30, true, now); // buy (different price)

    order_book.add_order(order1);
    order_book.add_order(order2);
    order_book.add_order(order3);
    order_book.add_order(order4);
    order_book.add_order(order5);
    order_book.add_order(order6);

    std::cout << "Initial Orders:\n";
    order_book.repr();

    std::cout << "\nAfter updating order 1's price:\n";
    order_book.update_order(1, 101.0, 12, std::nullopt);  // Update order1's price
    order_book.repr();

    std::cout << "\nAfter flipping order 1's side (from buy to sell):\n";
    order_book.update_order(1, std::nullopt, std::nullopt, false);  // Flip order1's side
    order_book.repr();

    return 0;
}
