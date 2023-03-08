#include <csignal>
#include <iostream>
#include <thread>

#include "process.hpp"

void interrupt_handler(int signnum)
{
	stop_process();
    cleanup_network_and_log();
	exit(0);
}

int main(int argc, char **argv)
{
	int clock_rate_hz;	
	if (argc > 2)
	{
		clock_rate_hz = std::stoi(argv[1]);
	}
	else
	{
		clock_rate_hz = uniform_random_number(1, 6);
	}
	int sleep_time_ms = (int)(1.0 / clock_rate_hz * 1000);

	// Network Setup.
	std::cout << "Setuping up process network...\n";
	int ret = setup_network_and_log();
	if (ret < 0)
	{
		return ret;
	}

	// Setup keyboard interrupt handler.
	std::signal(SIGINT, interrupt_handler);

	start_process();

	std::cout << "Waiting for other machines to initialize\n";
	while (get_peer_paths().size() < 2)
	{
		// block
	}
	std::cout << "Starting the model\n";
	// Run the model process.
	while (true)
	{
		int roll = uniform_random_number(1, 10);
		wake_up(roll);

		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
	}

	return 0;
}
