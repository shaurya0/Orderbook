#include "PricingEngine.h"
#include <chrono>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <unordered_map>
#include <deque>
#include <list>
#include <map>

using namespace Pricer;

static std::atomic<bool> cancelled = false;

void run_pricing_engine(uint32_t target_size)
{
	std::string strbuf;
	strbuf.reserve(128);
	OrderBookManager order_book_manager;
	PricingEngine pricing_engine(target_size, order_book_manager);

	while (!cancelled && std::getline(std::cin, strbuf))
	{
		if (strbuf.empty())
			continue;

		Order order;
		ErrorCode ec = OrderParser::parse_order(strbuf, order);

		if( ec == ErrorCode::NONE )
			order_book_manager.process( order );
		else
			print_error_code( ec );
	}
}

static void signal_handler(int sig)
{
	cancelled = true;
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
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		run_pricing_engine(static_cast<uint32_t>(target_size));
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cerr << "elapsed time (ms) = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;
	}
	catch (const std::exception &ex)
	{
		std::cout << ex.what() << std::endl;
	}
	catch (...)
	{
	}
	return 0;
}
