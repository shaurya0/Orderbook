#pragma once
#include "PricerCommon.h"
#include "OrderParser.h"
#include "OrderBook.h"
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <string>
#include <unordered_map>
#include <numeric>
#include <deque>
#include <list>
#include <map>

namespace Pricer
{
    class OrderBookManager
    {
    public:
        using BidOrderBook = OrderBook<std::greater<uint64_t>>;
        using AskOrderBook = OrderBook<std::less<uint64_t>>;

    private:
        BidOrderBook _bid_order_book;
        AskOrderBook _ask_order_book;

        template<typename T>
        bool id_in_order_book(const T& order_ids, const std::string &id) const noexcept
        {
            return order_ids.end() != order_ids.find(id);
        }

        void process_add_order(const Pricer::Order& order)
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
			print_error_code(ec);
        }

        void process_reduce_order(Pricer::Order &order)
        {
            const std::string &id = order.id;
            const auto& bid_ids = _bid_order_book.get_order_ids();
            const auto& ask_ids = _ask_order_book.get_order_ids();

            Pricer::ErrorCode ec = ErrorCode::NONE;
            if( id_in_order_book( bid_ids, id ) )
            {
                ec = _bid_order_book.reduce_order( order );
            }
            else if( id_in_order_book( ask_ids, id ) )
            {
                ec = _ask_order_book.reduce_order( order );
            }
            else
            {
                ec = ErrorCode::REDUCE_FAILED;
            }

			print_error_code(ec);
        }

    public:
        void process(Pricer::Order &order)
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
        }

        const BidOrderBook &get_bid_order_book() const noexcept { return _bid_order_book; }
		BidOrderBook &get_bid_order_book() noexcept { return _bid_order_book; }
        const AskOrderBook &get_ask_order_book() const noexcept { return _ask_order_book; }
		AskOrderBook &get_ask_order_book() noexcept { return _ask_order_book; }
    };
}
