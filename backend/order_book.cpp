#include <iostream>
#include <string>
#include <ctime>
#include <map>
#include <functional>
#include <utility>
#include <stdexcept>
#include <unordered_map>
#include <memory>


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
                        << "Timestamp : " << timestamp << ",\n";
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
            // get buy and sell
             auto& bought_order_ptr = order_lookup[order_id].first;
             auto& sold_order_ptr = order_lookup[order_id].second;
            
            // remove buy
            if(bought_order_ptr){
                auto range = buy_orders.equal_range(bought_order_ptr -> get_price());

                for(auto it = range.first; it != range.second; ++it){
                    if(it->second == *bought_order_ptr){
                        buy_orders.erase(it);
                        break;
                    }
                }
            }

            //remove sell
            if(sold_order_ptr){
                auto range  = sell_orders.equal_range(sold_order_ptr -> get_price());

                for(auto it = range.first; it != range.second; ++it){
                    if(it->second == *sold_order_ptr){
                        sell_orders.erase(it);
                        break;
                    }
                }
            }

            // remove from lookup
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
    };

int main(){
    std::time_t now = std::time(0); // current time
    
    OrderBook order_book;

    Order order1(1, 100.4, 10, true, now);
    Order order2(2, 200.4, 5, true, now);
    Order order3(3, 80.4, 1, false, now);
    Order order4(4, 14.78, 40, false, now);

    order_book.add_order(order1);
    order_book.add_order(order2);
    order_book.add_order(order3);
    order_book.add_order(order4);

    std::cout << "Order Book after adding orders" << "\n";
    order_book.repr();

    order_book.remove_order(3);
    std::cout << "Order Book after removing order 3" << "\n";
    order_book.repr();

    // std::cout << "The best bid is " << order_book.get_best_bid().repr() << "\n";
    std::cout << "The best bid is \n";
    order_book.get_best_bid().repr();
    return 0;
}