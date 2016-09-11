#include "PricingEngine.h"
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
	OrderBookController controller;
	PricingEngine pricing_engine(target_size, controller);
	pricing_engine.initialize();

	while (!cancelled && std::getline(std::cin, strbuf))
	{
		if (strbuf.empty())
			continue;

		Order order;
		ErrorCode ec = OrderParser::parse_order(strbuf, order);

		if( ec == ErrorCode::NONE )
			controller.process( order );
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

		run_pricing_engine(static_cast<uint32_t>(target_size));
	}
	catch (const std::exception &ex)
	{
		std::cout << ex.what() << std::endl;
	}
	catch (...)
	{
	}
	std::cout << "exiting" << std::endl;
	getchar();
	return 0;
}
