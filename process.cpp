#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <random>
#include <set>
#include <thread>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/logical_clocks"

int server_fd;
const char* socket_path;

std::set<std::string> get_peer_paths()
{
	std::set<std::string> paths;
	for (const auto& entry : std::filesystem::directory_iterator(SOCKET_PATH))
	{
		if (entry.is_socket())
		{
			paths.insert(entry.path().string());
		}
	}

	return paths;
}

std::string get_socket_name()
{
	int process_num = 0;
	std::set<int> available_numbers;
	for (int i = 0; i < 100; i++)
	{
		available_numbers.insert(i);
	}

	for (const auto& filename : get_peer_paths())
	{
		int found_num = 0;
		try
		{
			int index = filename.find("_");
			found_num = std::stoi(filename.substr(index + 1));
			available_numbers.erase(found_num);
		}
		catch(const std::exception& e)
		{
			// Not one of ours...
			continue;
		}
	}
	
	process_num = *available_numbers.begin();
	std::string socket_name = std::string(SOCKET_PATH) + "/process_" + std::to_string(process_num);
	return socket_name;
}

int uniform_random_number(int start_range, int end_range)
{
	std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(start_range, end_range); // define the range
	return distr(gen);
}

void send_message(std::string socket_path, std::string msg)
{
	struct sockaddr_un client_addr {};
	client_addr.sun_family = AF_UNIX;
	strncpy(client_addr.sun_path, socket_path.c_str(), sizeof(client_addr.sun_path) - 1);
	sendto(server_fd, msg.c_str(), msg.size(), MSG_CONFIRM, (struct sockaddr*)&client_addr, sizeof(client_addr));
}

std::string recv_message()
{
	std::string buf(1024, '\0');
	int n, len;
	n = recvfrom(server_fd, buf.data(), buf.size(), MSG_WAITALL, nullptr, nullptr);

	return buf;
}

void wake_up()
{
	int roll = uniform_random_number(1, 10);

	// Do things
	switch(roll)
	{
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		default:
			break;
	}
}

void interrupt_handler(int signnum)
{
	unlink(socket_path);
	shutdown(server_fd, SHUT_RDWR);
	close(server_fd);
	exit(0);
}

int main(int argc, char **argv)
{
	// TODO: Replace this with get_random_clock_rate()
	if (argc < 2)
	{
		std::cerr << "Usage: ./process CLOCK_RATE" << std::endl;
		return -1;
	}

	// Make temporary directory for socket files.
	if (system("mkdir -p " SOCKET_PATH) < 0)
	{
		perror("Failed to create directory /tmp/logical_clocks");
		return -1;
	}

	// Setup keyboard interrupt handler.
	std::signal(SIGINT, interrupt_handler);

	int clock_rate_hz = std::stoi(argv[1]);
	int sleep_time_ms = (int)(1.0 / clock_rate_hz * 1000);

	// Network Setup.
	std::cout << "Setuping up process network...\n";
	server_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (server_fd < 0)
	{
		perror("socket()");
		return -1;
	}

	struct sockaddr_un servaddr {};
	servaddr.sun_family = AF_UNIX;
	std::string socket_name = get_socket_name();
	socket_path = socket_name.c_str();
	std::cout << socket_path << "\n";
	strncpy(servaddr.sun_path, socket_path, sizeof(servaddr.sun_path) - 1);

	// Bind to file path.
	unlink(socket_path);
	if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind()");
		return -1;
	}

	std::cout << buf << "\n";

	while (true)
	{
		wake_up();

		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
	}

	return 0;
}
