#include "OrderParser.h"
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <unordered_map>
#include <deque>
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
    class OrderBook2
    {
    public:
        using quantity_t = uint32_t;
        using price_t = uint64_t;
        using price_levels_t = std::deque<quantity_t>;

        using order_book_t = std::map<price_t, price_levels_t>;
        using order_book_it_t = order_book_t::iterator;

        using order_id_t = std::pair<order_book_it_t, size_t>;
        using order_book_id_t = std::unordered_map<char, order_id_t>;
        //using price_level_map_t = std::unordered_map<price_t, order_book_it_t>;
    private:
        uint64_t _total_orders;

        order_book_t _orders;
        order_book_id_t _order_ids;
        //price_level_map_t _price_levels;

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
				std::deque<uint32_t> &levels = price_level_found_it->second;
				levels.push_back(order.size);
				_order_ids[order.id] = std::make_pair(price_level_found_it, levels.size() - 1);
            }
			else
			{
				const std::pair<order_book_it_t, bool> insert_result = _orders.insert(price_as_int);
				std::deque<uint32_t> &levels = insert_result.first->second;
				levels.push_back(order.size);
				_order_ids[order.id] = std::make_pair(insert_result.first, 0);
			}
			_total_orders += order.size;
        }

        void reduce_order(const Pricer::Order &order)
        {
			auto found_it = _order_ids.find(order.id);
			const bool found = found_it != _order_ids.end();
			if (!found)
				return;

			std::deque<uint32_t> &levels = found_it->second;			
			size_t index = found_it->second.second;
			uint32_t previous_size = levels[index];
			if (order.size >= previous_size)
			{
				levels.erase(index);
			}
			else
			{
				levels[index] -= order.size;
			}
        }
    };

	class OrderBook1
	{
	public:
		using quantity_t = uint32_t;
		using price_t = double;

		using order_book_t = std::map <price_t, quantity_t>;
		using order_book_it = order_book_t::iterator;
		using order_book_id_t = std::unordered_map < char, order_book_it >;

	private:
		uint64_t _total_asks;
		uint64_t _total_bids;

		order_book_t _bid_orders;
		order_book_id_t _bid_order_ids;

		order_book_t _ask_orders;
		order_book_id_t _ask_order_ids;

		void add_order(const Pricer::Order &order)
		{
			bool found_bid = _bid_order_ids.end() != _bid_order_ids.find(order.id);
			bool found_ask = _ask_order_ids.end() != _ask_order_ids.find(order.id);
			if (found_bid || found_ask)
				throw std::exception("add order duplicate id");

			const auto &add_order_info = boost::get<Pricer::AddOrder>(order._order);

			const order_book_t::value_type price_quantity = std::make_pair(add_order_info.limit_price, order.size);

			if (add_order_info.order_type == Pricer::ADD_ORDER_TYPE::ASK)
			{
				auto &order_book = _ask_orders;
				auto &order_id_book = _ask_order_ids;
				_total_asks += order.size;
				const std::pair<order_book_it, bool> insert_result = order_book.insert(price_quantity);
				order_id_book[order.id] = insert_result.first;
			}
			else
			{
				auto &order_book = _bid_orders;
				auto &order_id_book = _bid_order_ids;
				_total_bids += order.size;
				const std::pair<order_book_it, bool> insert_result = order_book.insert(price_quantity);
				order_id_book[order.id] = insert_result.first;
			}
		}

		void reduce_order(const Pricer::Order &order)
		{
			bool found_bid = _bid_order_ids.end() != _bid_order_ids.find(order.id);
			bool found_ask = _ask_order_ids.end() != _ask_order_ids.find(order.id);
			if (!found_bid || !found_ask)
				throw std::exception("did not find order id");

			auto &order_book = _bid_orders;
			auto &order_id_book = _bid_order_ids;
            if( found_ask )
            {
                order_book = _ask_orders;
                order_id_book = _ask_order_ids;
            }

            order_book_it it = order_id_book[order.id];
			quantity_t order_quantity = it->second;
            if( order.size >= order_quantity)
            {
                order_book.erase( it );
                order_id_book.erase( order.id );
            }
            else
            {
                it->second -= order.size;
            }
		}

	public:
		OrderBook1() :_total_asks(0), _total_bids(0) {}

		void process_order(const Pricer::Order &order)
		{
			switch (order.type)
			{
			case ORDER_TYPE::ADD:
				add_order(order);
				break;
			case ORDER_TYPE::REDUCE:
				reduce_order(order);
				break;
			}
		}

		const order_book_t &get_bid_orders() const noexcept { return _bid_orders; };
		const order_book_t &get_ask_orders() const noexcept { return _ask_orders; };
	};

	class PricingEngine
	{
	private:
		OrderBook1 order_book;
		long int _target_size;
	public:
		PricingEngine(long int target_size) : _target_size(target_size){}

		void init(){}
		void process(const Pricer::Order &order)
		{
			order_book.process_order(order);
		}
	};
}

using namespace Pricer;
std::string add_order = "28800562 A c B 44.10 100";
std::string reduce_order = "28800744 R b 100";

class cancel_exception : public std::exception
{
public:
	virtual char const * what() const { return "received cancel signal"; }
};

static std::atomic<bool> cancelled = false;

void run_pricing_engine(long int target_size)
{
	std::string strbuf;
	strbuf.reserve(128);
	PricingEngine pricing_engine(target_size);
	pricing_engine.init();

	while (!cancelled)
	{
		std::getline(std::cin, strbuf);
		const Order order = OrderParser::parse_order(strbuf);
		pricing_engine.process(order);
	}
}

static void signal_handler(int sig)
{
	cancelled = true;
	throw cancel_exception();
}

int main(int argc, char *argv[])
{
	signal(SIGINT, signal_handler);
	try
	{
		if (argc != 2)
		{
			throw std::exception("needs to be 1 argument");
		}
		long int target_size = strtol(argv[1], nullptr, 10);

		if (target_size < 0)
		{
			throw std::exception("negative target");
		}

		run_pricing_engine(target_size);
	}
    catch( const cancel_exception& )
    {
        std::cout << "received cancel signal, exiting gracefully";
    }
	catch (const std::exception &ex)
	{
		std::cout << ex.what();
	}
	catch (...)
	{

	}
	getchar();
	return 0;
}
