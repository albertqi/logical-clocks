#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <random>
#include <set>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <fstream>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define BASE_PATH "/tmp/logical_clocks"
#define SOCKET_PATH BASE_PATH"/socket"
#define LOG_PATH BASE_PATH"/log"

int server_fd;
int process_num;
const char* socket_path;
std::ofstream log_file;

std::atomic<bool> recv_thread_running;
std::mutex queue_mutex;
std::queue<uint32_t*> message_queue;

uint32_t local_clock[3];

std::vector<std::string> get_peer_paths()
{
	std::vector<std::string> paths;
	for (const auto& entry : std::filesystem::directory_iterator(SOCKET_PATH))
	{
		if (entry.is_socket())
		{
			std::string pathname = entry.path().string();
			if (socket_path == nullptr || strncmp(socket_path, pathname.c_str(), 107) != 0)
			{
				paths.push_back(pathname);
			}
		}
	}

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
	std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(start_range, end_range); // define the range
	return distr(gen);
}

void send_message(std::string socket_path, uint32_t clocks[3])
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

bool recv_message(uint32_t clocks_ret[3])
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

void log(std::string log_message)
{
	auto system_time = std::chrono::high_resolution_clock::now();
	std::unique_lock lk(queue_mutex);
	int queue_size = message_queue.size();
	lk.unlock();
	log_file << "[" << std::to_string(system_time) << "]";
	log_file << " - " << local_clock[0] << "," << local_clock[1] << "," << local_clock[2];
	log_file << " - " << queue_size << ": " << log_message << "\n";
}

void wake_up()
{
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

	int roll = uniform_random_number(1, 10);

	int to_min = std::max(1 - process_num, 0);
	int to_max = std::min(3 - process_num, 2);

	// Do things
	switch(roll)
	{
		case 1:
			send_message(get_peer_paths()[to_min], local_clock);
			++local_clock[process_num];
			log("SEND");
			break;
		case 2:
			send_message(get_peer_paths()[to_max], local_clock);
			++local_clock[process_num];
			log("SEND");
			break;
		case 3:
			send_message(get_peer_paths()[to_min], local_clock);
			send_message(get_peer_paths()[to_max], local_clock);
			++local_clock[process_num];
			log("SEND");
			break;
		default:
			++local_clock[process_num];
			log("INTERNAL");
			break;
	}
}

void recv_loop()
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

void interrupt_handler(int signnum)
{
	recv_thread_running = false;
	unlink(socket_path);
	shutdown(server_fd, SHUT_RDWR);
	close(server_fd);
	log_file.flush();
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
	// Make temporary directory for socket files.
	if (system("mkdir -p " LOG_PATH) < 0)
	{
		perror("Failed to create directory /tmp/logical_clocks");
		return -1;
	}

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

	process_num = get_process_number();

	struct sockaddr_un servaddr {};
	servaddr.sun_family = AF_UNIX;
	std::string socket_name = std::string(SOCKET_PATH) + "/process_" + std::to_string(process_num) + ".log";
	socket_path = socket_name.c_str();
	strncpy(servaddr.sun_path, socket_path, sizeof(servaddr.sun_path) - 1);

	std::cout << socket_path << "\n";

	// Bind to file path.
	unlink(socket_path);
	if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind()");
		return -1;
	}

	// Setup log
	std::string log_path = std::string(LOG_PATH) + "process_" + std::to_string(process_num);
	log_file = std::ofstream(log_path);

	// Setup keyboard interrupt handler.
	std::signal(SIGINT, interrupt_handler);

	std::thread recv_thread (recv_loop);
	recv_thread.detach();

	// Run the model process.
	while (true)
	{
		wake_up();

		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
	}

	return 0;
}
