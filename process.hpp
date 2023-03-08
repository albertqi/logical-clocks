#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <fstream>

/**
 * Returns list of all sockets found in SOCEKT_PATH
 */
std::vector<std::string> get_peer_paths();

/**
 * Returns an available process number.
 */
int get_process_number();

/**
 * Uniform random number in the closed range [start_range, end_range].
 */
int uniform_random_number(int start_range, int end_range);

/**
 * A model process that keeps track of a vector clock.
 */
class Process
{
public:
    Process();

    ~Process();

    /**
     * Sends a datagram to socket_path
     */
    void send_message(std::string socket_path, uint32_t clocks[3]);

    /**
     * Pulls the next datagram from the rx queue. If no message is available,
     * returns false.
     */
    bool recv_message(uint32_t clocks_ret[3]);

    /**
     * Writes to the log file in LOG_PATH.
     */
    void log(std::string log_message);

    /**
     * Performs the actions defined in the sepc.
     */
    void wake_up(int roll);

private:
    int server_fd;
    int process_num;
    std::string socket_path;
    std::ofstream log_file;

    std::atomic<bool> recv_thread_running;
    std::mutex queue_mutex;
    std::queue<uint32_t*> message_queue;

    uint32_t local_clock[3];

    /**
     * Receives from socket and puts datagrams into rx queue.
     */
    void recv_loop();

    /**
     * Creates socket and log file. Sets up internal data.
     */
    int setup_network_and_log();

    /**
     * Cleans up socket and log file.
     */
    void cleanup_network_and_log();
};
