#include "OrderParser.h"
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <unordered_map>
#include <map>

namespace Pricer
{
	class OrderBook
	{
	private:
		using quantity_t = uint32_t;
		using price_t = double;

		using order_book_t = std::map <price_t, quantity_t>;
		using order_book_it = order_book_t::iterator;
		using order_book_id_t = std::unordered_map < char, order_book_it > ;

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

			auto &order_book = _bid_orders;
			auto &order_id_book = _bid_order_ids;
			if (add_order_info.order_type == Pricer::ADD_ORDER_TYPE::ASK)
			{
				order_book = _ask_orders;
				order_id_book = _ask_order_ids;
			}

			const std::pair<order_book_it, bool> insert_result = order_book.insert(price_quantity);
			order_id_book[order.id] = insert_result.first;
		}

		void reduce_order(const Pricer::Order &order)
		{
			bool found_bid = _bid_order_ids.end() != _bid_order_ids.find(order.id);
			bool found_ask = _ask_order_ids.end() != _ask_order_ids.find(order.id);
			if (!found_bid || !found_ask)
				throw std::exception("did not find reduce order id");

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
		OrderBook(){}
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

	};
	class PricingEngine
	{
	private:
		long int _target_size;
	public:
		PricingEngine(long int target_size) : _target_size(target_size){}

		void init(){}
		void process(const Pricer::Order &order)
		{

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
