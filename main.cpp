#include "OrderParser.h"
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
                const auto key_value = std::make_pair( order.id, order.size );
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

        const order_book_t &get_order_book() const noexcept { return _orders; }
		const order_book_id_t &get_order_id_book() const noexcept { return _order_ids; }
    };

	class PricingEngine
	{
    public:
        using BidOrderBook = OrderBook<std::greater<uint64_t>>;
        using AskOrderBook = OrderBook<std::less<uint64_t>>;

	private:
        long int _target_size;

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
            const auto& bid_ids = _bid_order_book.get_order_id_book();
            const auto& ask_ids = _ask_order_book.get_order_id_book();
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

	public:
		PricingEngine(long int target_size) : _target_size(target_size){}

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

            //print maximum selling income
            //print minimum buying amount
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
