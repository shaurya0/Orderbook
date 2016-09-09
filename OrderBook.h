#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <unordered_map>
#include <deque>
#include <list>
#include <map>

namespace Pricer
{
    class Utils
    {
    public:
        static uint64_t convert_price( double price )
        {
            return static_cast<uint64_t>( price*100 );
        }

        static double convert_price( uint64_t price )
        {
            return static_cast<double>( price )/100.0;
        }
    };

    template<typename CMP_FUNC>
    class OrderBook
    {
    public:
        using quantity_t = uint32_t;
        using price_t = uint64_t;

        using price_levels_t = std::unordered_map<char, quantity_t>;
        using order_book_t = std::map<price_t, price_levels_t, CMP_FUNC>;

        typedef typename order_book_t::iterator order_book_it_t;
        using order_book_id_t = std::unordered_map<char, order_book_it_t>;
    private:
        uint64_t _total_orders;

        order_book_t _orders;
        order_book_id_t _order_ids;

    public:
        void add_order(const Pricer::Order &order)
        {
            const bool found_order = _order_ids.end() != _order_ids.find(order.id);

            if( found_order )
                throw std::exception( "two outstanding adds with same id" );

            const auto &add_order_info = boost::get<Pricer::AddOrder>(order._order);
            const price_t price_as_int = Pricer::Utils::convert_price( add_order_info.limit_price );

            const auto price_level_found_it = _orders.find(price_as_int);
            if( _orders.end() != price_level_found_it )
            {
                price_levels_t &levels = price_level_found_it->second;
                levels[order.id] = order.size;
                _order_ids[order.id] = price_level_found_it;
            }
            else
            {
                const auto key_value = std::make_pair(price_as_int, price_levels_t{ {order.id, order.size} });
                const auto insert_result = _orders.insert( key_value );
                _order_ids[order.id] = insert_result.first;
            }
            _total_orders += order.size;
        }

        void reduce_order(const Pricer::Order &order)
        {
            auto found_it = _order_ids.find(order.id);
            const bool found = found_it != _order_ids.end();
            if (!found)
            {
                std::cout << "could not find order with id : " << order.id << std::endl;
                return;
            }

            const auto order_it = found_it->second;
            price_levels_t &levels = order_it->second;
            uint32_t previous_size = levels[order.id];
            if (order.size >= previous_size)
            {
                levels.erase(order.id);
            }
            else
            {
                levels[order.id] = previous_size - order.size;
            }
        }

        const order_book_t &get_orders() const noexcept { return _orders; }
        const order_book_id_t &get_order_ids() const noexcept { return _order_ids; }
        uint64_t get_total_orders() const noexcept{ return _total_orders; }
    };

}
