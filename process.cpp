#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <random>
#include <set>
#include <thread>
#include <fstream>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "process.hpp"

#define BASE_PATH "/tmp/logical_clocks"
#define SOCKET_PATH BASE_PATH"/socket"
#define LOG_PATH BASE_PATH"/log"

std::vector<std::string> get_peer_paths()
{
	std::vector<std::string> paths;
	for (const auto& entry : std::filesystem::directory_iterator(SOCKET_PATH))
	{
		if (entry.is_socket())
		{
			std::string pathname = entry.path().string();
			paths.push_back(pathname);
		}
	}

	std::sort(paths.begin(), paths.end());
	return paths;
}

int get_process_number()
{
	int process_num = 0;
	std::set<int> available_numbers;
	for (int i = 0; i < 3; i++)
	{
		available_numbers.insert(i);
	}

	for (const auto& filename : get_peer_paths())
	{
		int found_num = 0;
		try
		{
			int index = filename.find_last_of("_");
			found_num = std::stoi(filename.substr(index + 1));
			available_numbers.erase(found_num);
		}
		catch(const std::exception& e)
		{
			// Not one of ours...
			continue;
		}
	}
	
	return *available_numbers.begin();
}

int uniform_random_number(int start_range, int end_range)
{
	std::random_device rd; // Obtain a random number from hardware.
    std::mt19937 gen(rd()); // Seed the generator.
    std::uniform_int_distribution<> distr(start_range, end_range); // Define the range.
	return distr(gen);
}

int Process::setup_network_and_log()
{
	// Make temporary directory for socket files.
	if (system("mkdir -p " SOCKET_PATH) < 0)
	{
		perror("Failed to create directory /tmp/logical_clocks");
		return -1;
	}

	// Make temporary directory for log files.
	if (system("mkdir -p " LOG_PATH) < 0)
	{
		perror("Failed to create directory /tmp/logical_clocks");
		return -1;
	}

	// Network setup.
	server_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (server_fd < 0)
	{
		perror("socket()");
		return -1;
	}

	process_num = get_process_number();

	struct sockaddr_un servaddr {};
	servaddr.sun_family = AF_UNIX;
	std::string socket_name = std::string(SOCKET_PATH) + "/process_" + std::to_string(process_num) + ".socket";
	socket_path = socket_name;
	strncpy(servaddr.sun_path, socket_path.c_str(), sizeof(servaddr.sun_path) - 1);

	std::cout << socket_path << "\n";

	// Bind to file path.
	unlink(socket_path.c_str());
	if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind()");
		return -1;
	}

	// Set up log.
	std::string log_path = std::string(LOG_PATH) + "/process_" + std::to_string(process_num) + ".log";
	log_file = std::ofstream(log_path);

	return 0;
}

void Process::cleanup_network_and_log()
{
	unlink(socket_path.c_str());
	shutdown(server_fd, SHUT_RDWR);
	close(server_fd);
}

Process::Process()
{
	int ret = setup_network_and_log();
	if (ret < 0)
	{
		exit(-1);
	}

	local_clock[0] = 0;
	local_clock[1] = 0;
	local_clock[2] = 0;

	recv_thread_running = true;
	std::thread recv_thread (&Process::recv_loop, this);
	recv_thread.detach();
}

Process::~Process()
{
	recv_thread_running = false;
	cleanup_network_and_log();
}

void Process::send_message(std::string socket_path, uint32_t clocks[3])
{
	struct sockaddr_un client_addr {};
	client_addr.sun_family = AF_UNIX;
	strncpy(client_addr.sun_path, socket_path.c_str(), sizeof(client_addr.sun_path) - 1);

	char buf[12];
	memcpy(&buf[0], (unsigned char*) &clocks[0], 4);
	memcpy(&buf[4], (unsigned char*) &clocks[1], 4);
	memcpy(&buf[8], (unsigned char*) &clocks[2], 4);
	
	sendto(server_fd, buf, sizeof(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
}

bool Process::recv_message(uint32_t clocks_ret[3])
{
	std::unique_lock lk(queue_mutex);
	if (message_queue.size() == 0)
	{
		return false;
	}

	uint32_t* clocks_recvd = message_queue.front();
	message_queue.pop();
	
	memcpy(clocks_ret, clocks_recvd, 12);

	delete[] clocks_recvd;

	return true;
}

void Process::log(std::string log_message)
{
	auto system_time = std::chrono::high_resolution_clock::now();
	std::unique_lock lk(queue_mutex);
	int queue_size = message_queue.size();
	lk.unlock();
	log_file << "[" << std::to_string(system_time.time_since_epoch().count()) << "]";
	log_file << " | " << local_clock[0] << "," << local_clock[1] << "," << local_clock[2];
	log_file << " | " << queue_size << " | " << log_message << "\n";
	log_file.flush();
}

void Process::wake_up(int roll)
{
	// Check that peers are still running.
	std::vector<std::string> peer_paths = get_peer_paths();
	if (peer_paths.size() != 3)
	{
		return;
	}

	// If a message is available, then do a receive.
	uint32_t recvd_clocks[3];
	bool recvd_message = recv_message(recvd_clocks);
	if (recvd_message) {
		for (int i = 0; i < 3; ++i) {
			local_clock[i] = std::max(local_clock[i], recvd_clocks[i]);
		}
		++local_clock[process_num];
		log("RECEIVE");
		return;
	}

	// Calculate which indices are peers.
	int to_min = std::max(1 - process_num, 0);
	int to_max = std::min(3 - process_num, 2);
	
	// Perform operation based on roll.
	switch(roll)
	{
		case 1:
			send_message(peer_paths[to_min], local_clock);
			++local_clock[process_num];
			log("SEND");
			break;
		case 2:
			send_message(peer_paths[to_max], local_clock);
			++local_clock[process_num];
			log("SEND");
			break;
		case 3:
			send_message(peer_paths[to_min], local_clock);
			send_message(peer_paths[to_max], local_clock);
			++local_clock[process_num];
			log("DOUBLE SEND");
			break;
		default:
			++local_clock[process_num];
			log("INTERNAL");
			break;
	}
}

void Process::recv_loop()
{
	while (recv_thread_running)
	{
		std::string buf(12, '\0');
		int n, len;
		n = recvfrom(server_fd, buf.data(), buf.size(), MSG_WAITALL, nullptr, nullptr);

		uint32_t* clocks_ret = new uint32_t[3];
		uint32_t* tmp = (uint32_t*) &buf[0];
		clocks_ret[0] = *tmp;
		tmp = (uint32_t*) &buf[4];
		clocks_ret[1] = *tmp;
		tmp = (uint32_t*) &buf[8];
		clocks_ret[2] = *tmp;

		std::unique_lock lk(queue_mutex);
		message_queue.push(clocks_ret);
	}
}
