#pragma once
#include "PricerCommon.h"
#include "OrderParser.h"
#include "OrderBook.h"
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <unordered_map>
#include <numeric>
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
        uint64_t _last_income;
        uint64_t _last_expense;

        BidOrderBook _bid_order_book;
        AskOrderBook _ask_order_book;

        template<typename T>
        bool id_in_order_book(const T& order_ids, char id) const noexcept
        {
            return order_ids.end() != order_ids.find(id);
        }

        Pricer::ErrorCode process_add_order(const Pricer::Order& order)
        {
            const auto& add_order = boost::get<AddOrder>(order._order);
            Pricer::ErrorCode ec = ErrorCode::NONE;
            switch (add_order.order_type)
            {
            case Pricer::ADD_ORDER_TYPE::ASK:
                ec = _ask_order_book.add_order(order);
                break;
            case Pricer::ADD_ORDER_TYPE::BID:
                ec = _bid_order_book.add_order(order);
                break;
            }
            return ec;
        }

        Pricer::ErrorCode process_reduce_order(const Pricer::Order &order)
        {
            char id = order.id;
            const auto& bid_ids = _bid_order_book.get_order_ids();
            const auto& ask_ids = _ask_order_book.get_order_ids();
            Pricer::ErrorCode result = ErrorCode::NONE;
            if( id_in_order_book( bid_ids, id ) )
            {
                result = _bid_order_book.reduce_order( order );
            }
            else if( id_in_order_book( ask_ids, id ) )
            {
                result = _ask_order_book.reduce_order( order );
            }
            else
            {
                result = ErrorCode::REDUCE_FAILED;
            }

            return result;
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
            return std::make_pair( result, num_shares );
        }

    public:
        PricingEngine(long int target_size) :
          _target_size(target_size)
        , _last_expense(std::numeric_limits<uint64_t>::max())
        , _last_income(std::numeric_limits<uint64_t>::max()) {}

        void init(){}

        // optimization oppoprtunities:
        // if last order is a sell then there is no need to compute expenses
        // if last order is a bid then there is no need to compute income
        // cache the smallest bid in the sell : if a new bid comes in which is smaller and the previous sell was fulfilled then we don't need to recompute
        // cache the largest ask in the buy : if a new sell comes in which is larger and the previous buy was fulfilled then we don't need to recompute

        //

        void compute_targets(const Order &order)
        {
            auto income_shares = order_book_target( _ask_order_book.get_orders() );
            if( income_shares.second == _target_size )
            {
                double income = Utils::convert_price( income_shares.first );
                std::cout << order.milliseconds << " B " << income << std::endl;
                _last_income = income_shares.first;
            }

            auto expenses_shares = order_book_target( _bid_order_book.get_orders() );
            if( expenses_shares.second == _target_size )
            {
                double expenses = Utils::convert_price( expenses_shares.first );
                std::cout << order.milliseconds << " S " << expenses << std::endl;
                _last_expense = expenses_shares.first;
            }
        }

        void process(const Pricer::Order &order)
        {
            Pricer::ErrorCode ec = ErrorCode::NONE;
            switch( order.type )
            {
            case Pricer::ORDER_TYPE::ADD:
                ec = process_add_order(order);
                break;
            case Pricer::ORDER_TYPE::REDUCE:
                ec = process_reduce_order(order);
                break;
            }

            if( ec != ErrorCode::NONE )
                print_error_code( ec );
            else
                compute_targets(order);
        }
    };
} // Pricer
