#pragma once
#include "PricerCommon.h"
#include <stdlib.h>
#include <utility>
#include <unordered_map>
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

		using level_sizes_t = std::list<quantity_t>;
        using order_book_t = std::map<price_t, level_sizes_t, CMP_FUNC>;
        typedef typename order_book_t::iterator order_book_it_t;
		using order_book_hash_t = std::unordered_map<price_t, order_book_it_t>;
		using order_book_id_t = std::unordered_map <std::string, std::pair <order_book_it_t, level_sizes_t::iterator>>;

    private:
        uint32_t _total_orders;
        order_book_t _orders;
		order_book_hash_t _price_order_hash;
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
		OrderBook() : _total_orders(0){}
        Pricer::ErrorCode add_order(const Pricer::Order &order) noexcept
        {
            const bool found_order = _order_ids.end() != _order_ids.find(order.id);

            if( found_order )
                return ErrorCode::ADD_FAILED;

            const auto &add_order_info = boost::get<Pricer::AddOrder>(order._order);
            const price_t price = add_order_info.limit_price;
			const auto price_found_it = _price_order_hash.find(price);

			if (_price_order_hash.end() != price_found_it)
			{
				order_book_it_t order_book_it = price_found_it->second;
				level_sizes_t &levels = order_book_it->second;
				levels.push_front(order.size);
				_order_ids[order.id] = std::make_pair(order_book_it, levels.begin());
			}
			else
			{
	            const auto key_value = std::make_pair(price, level_sizes_t{order.size});
	            const auto insert_result = _orders.insert( key_value );
				_order_ids[order.id] = std::make_pair(insert_result.first, insert_result.first->second.begin());
				_price_order_hash[price] = insert_result.first;
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

            const std::pair<order_book_it_t, level_sizes_t::iterator> order_kv = found_it->second;
			level_sizes_t &levels = order_kv.first->second;

            level_sizes_t::iterator order_ptr = order_kv.second;
			uint32_t previous_size = *order_ptr;

			auto &reduce_order = boost::get<ReduceOrder>(order._order);
			const uint32_t order_price = order_kv.first->first;
			reduce_order.price = order_price;

            if (order.size >= previous_size)
            {
                levels.erase(order_ptr);
				if (levels.empty())
				{
					_orders.erase(order_kv.first);
					_price_order_hash.erase(order_price);
				}

				_order_ids.erase(order.id);
				_total_orders -= previous_size;
            }
            else
            {
                *order_ptr = previous_size - order.size;
				_total_orders -= order.size;
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
