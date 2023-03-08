#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

int setup_network_and_log(int& process_num, int& server_fd, std::string& socket_path, std::ofstream& log_file);

void cleanup_network_and_log(int process_num, int server_fd, std::string socket_path, std::ofstream& log_file);

std::vector<std::string> get_peer_paths();

int get_process_number();

int uniform_random_number(int start_range, int end_range);

class Process
{
public:
    Process();

    ~Process();

    void send_message(std::string socket_path, uint32_t clocks[3]);

    bool recv_message(uint32_t clocks_ret[3]);

    void log(std::string log_message);

    void wake_up(int roll);

    void start_process();

    void stop_process();

    int setup_network_and_log();

    void cleanup_network_and_log();

private:
    int server_fd;
    int process_num;
    std::string socket_path;
    std::ofstream log_file;

    std::atomic<bool> recv_thread_running;
    std::mutex queue_mutex;
    std::queue<uint32_t*> message_queue;

    uint32_t local_clock[3];

    void recv_loop();
};
