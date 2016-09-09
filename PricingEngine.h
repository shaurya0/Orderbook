#include "OrderParser.h"
#include "OrderBook.h"
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
    class PricingEngine
    {
    public:
        using BidOrderBook = OrderBook<std::greater<uint64_t>>;
        using AskOrderBook = OrderBook<std::less<uint64_t>>;

    private:
        long int _target_size;
        double _last_income;
        double _last_expense;

        BidOrderBook _bid_order_book;
        AskOrderBook _ask_order_book;

        template<typename T>
        bool id_in_order_book(const T& order_ids, char id) const noexcept
        {
            return order_ids.end() != order_ids.find(id);
        }

        void process_add_order(const Pricer::Order& order)
        {
            const auto& add_order = boost::get<AddOrder>(order._order);
            switch (add_order.order_type)
            {
            case Pricer::ADD_ORDER_TYPE::ASK:
                _ask_order_book.add_order(order);
                break;
            case Pricer::ADD_ORDER_TYPE::BID:
                _bid_order_book.add_order(order);
                break;
            }
        }

        void process_reduce_order(const Pricer::Order &order)
        {
            char id = order.id;
            const auto& bid_ids = _bid_order_book.get_order_ids();
            const auto& ask_ids = _ask_order_book.get_order_ids();
            if( id_in_order_book( bid_ids, id ) )
            {
                _bid_order_book.reduce_order( order );
            }
            else if( id_in_order_book( ask_ids, id ) )
            {
                _ask_order_book.reduce_order( order );
            }
            else
            {
                //TODO: throw exception or return error
            }
        }


        template<typename OB>
        std::pair<uint64_t, uint64_t> order_book_target(const OB& order_book) const
        {
            uint64_t result = 0;
            uint64_t num_shares = 0;
            for( const auto &levels : order_book )
            {
                uint64_t price = levels.first;
                for( const auto &levels: levels.second)
                {
                    uint64_t shares = 0;
                    if( levels.second + num_shares > _target_size )
                    {
                        shares = _target_size - num_shares;
                    }
                    else
                    {
                        shares = levels.second;
                    }
                    result += price*shares;
					num_shares += shares;
                }
            }
			//return std::make_pair(0, 0);
            return std::make_pair( result, num_shares );
        }

    public:
        PricingEngine(long int target_size) :
        _target_size(target_size)
        , _last_expense(0.0)
        , _last_income(0.0) {}

        void init(){}

        void process(const Pricer::Order &order)
        {
            switch( order.type )
            {
            case Pricer::ORDER_TYPE::ADD:
                process_add_order(order);
                break;
            case Pricer::ORDER_TYPE::REDUCE:
                process_reduce_order(order);
                break;
            }

            auto income_shares = order_book_target( _ask_order_book.get_orders() );
            if( income_shares.second == _target_size )
            {
                double income = static_cast<double>(income_shares.first/100.0);
                std::cout << order.milliseconds << " B " << income << std::endl;
            }

            auto expenses_shares = order_book_target( _bid_order_book.get_orders() );
            if( expenses_shares.second == _target_size )
            {
                double expenses = static_cast<double>(expenses_shares.first/100.0);
                std::cout << order.milliseconds << " S " << expenses << std::endl;
            }

        }
    };
} // Pricer
