#pragma once
#include "PricerCommon.h"
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <unordered_map>
#include <deque>
#include <list>
#include <map>
#include <functional>

namespace Pricer
{
    template<typename CMP_FUNC>
    class OrderBook
    {
    public:
        using quantity_t = uint32_t;
        using price_t = uint32_t;

        using price_levels_t = std::unordered_map<std::string, quantity_t>;
        using order_book_t = std::map<price_t, price_levels_t, CMP_FUNC>;

        typedef typename order_book_t::iterator order_book_it_t;
        using order_book_id_t = std::unordered_map<std::string, order_book_it_t>;

    private:
        uint32_t _total_orders;
        order_book_t _orders;
        order_book_id_t _order_ids;

		std::list<std::function<void(const Pricer::Order&)>> _observers;
		void notify_observers(const Pricer::Order &order) const
		{
			for (const auto &observer : _observers)
			{
				observer(order);
			}
		}

    public:
        Pricer::ErrorCode add_order(const Pricer::Order &order) noexcept
        {
            const bool found_order = _order_ids.end() != _order_ids.find(order.id);

            if( found_order )
                return ErrorCode::ADD_FAILED;

            const auto &add_order_info = boost::get<Pricer::AddOrder>(order._order);
            const price_t price = add_order_info.limit_price;

            const auto price_level_found_it = _orders.find(price);
            if( _orders.end() != price_level_found_it )
            {
                price_levels_t &levels = price_level_found_it->second;
                levels[order.id] = order.size;
                _order_ids[order.id] = price_level_found_it;
            }
            else
            {
                const auto key_value = std::make_pair(price, price_levels_t{ {order.id, order.size} });
                const auto insert_result = _orders.insert( key_value );
                _order_ids[order.id] = insert_result.first;
            }
            _total_orders += order.size;

			notify_observers(order);
            return ErrorCode::NONE;
        }

        Pricer::ErrorCode reduce_order(Pricer::Order &order) noexcept
        {
            auto found_it = _order_ids.find(order.id);
            const bool found = found_it != _order_ids.end();
            if (!found)
                return ErrorCode::REDUCE_FAILED;

            const auto order_it = found_it->second;
            price_levels_t &levels = order_it->second;
            uint32_t previous_size = levels[order.id];
			auto &reduce_order = boost::get<ReduceOrder>(order._order);
			reduce_order.price = order_it->first;
            if (order.size >= previous_size)
            {
                levels.erase(order.id);
				if (levels.empty())
					_orders.erase(order_it);
            }
            else
            {
                levels[order.id] = previous_size - order.size;
            }
			notify_observers(order);
            return ErrorCode::NONE;
        }

         template<typename FUNC>
         void register_observer(FUNC &&callback)
         {
             _observers.push_back(std::forward<FUNC>(callback));
         }


        const order_book_t &get_orders() const noexcept { return _orders; }
        const order_book_id_t &get_order_ids() const noexcept { return _order_ids; }
        uint32_t get_total_orders() const noexcept{ return _total_orders; }
    };

}
