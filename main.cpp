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
