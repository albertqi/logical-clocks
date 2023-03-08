#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>

#include "process.hpp"

int process_num, server_fd;
std::string socket_path;
std::ofstream log_file;

Process* process;

void interrupt_handler(int signnum)
{
	process->stop_process();
    cleanup_network_and_log(process_num, server_fd, socket_path, log_file);
	exit(0);
}

int main(int argc, char **argv)
{
	int clock_rate_hz;	
	if (argc >= 2)
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
	int ret = setup_network_and_log(process_num, server_fd, socket_path, log_file);
	if (ret < 0)
	{
		return ret;
	}

	// Setup keyboard interrupt handler.
	std::signal(SIGINT, interrupt_handler);

	process = new Process(process_num, server_fd, socket_path, log_file);

	std::cout << "Waiting for other machines to initialize\n";
	while (get_peer_paths().size() < 3)
	{
		// block
	}
	std::cout << "Starting the model\n";

    process->start_process();

	// Run the model process.
	while (true)
	{
		int roll = uniform_random_number(1, 10);
		process->wake_up(roll);

		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
	}

    delete process;

	return 0;
}
