#include <iostream>
#include <string>
#include <thread>

#include "process.hpp"

#define BASE_PATH "/tmp/logical_clocks"
#define SOCKET_PATH BASE_PATH "/socket"
#define LOG_PATH BASE_PATH "/log"

#define PADDED_LENGTH 50

bool allTestsPassed = true;

void test(bool b, std::string msg)
{
    int i = msg.size();
    msg += b ? "PASSED" : "FAILED";
    msg.insert(i, PADDED_LENGTH - msg.size(), ' ');
    allTestsPassed = allTestsPassed && b;
    std::cerr << msg << std::endl;
}

void run_tests()
{
    Process p0, p1, p2;

    for (auto &m : get_peer_paths())
    {
        std::cerr << m << std::endl;
    }

    p0.send_message(get_peer_paths()[1], (uint32_t[]){0, 0, 0});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    p1.wake_up(0);
    // std::ifstream log_file(LOG_PATH "/process_1.log");
    FILE *log_file = fopen(LOG_PATH "/process_1.log", "r");
    char sys_time[100];
    char clock[100];
    int queue_size;
    char operation[100];
    fscanf(log_file, "%s | %s | %d | %s", sys_time, clock, &queue_size, operation);
    fclose(log_file);

    std::cerr << sys_time << std::endl;
    std::cerr << clock << std::endl;
    std::cerr << queue_size << std::endl;
    std::cerr << operation << std::endl;

    // log_file.
    // if (log_file.is_open()) {
    //     log_file >> s >> s >> s;
    //     std::cerr << s << std::endl;
    // }

    // "[%d] | %s | %d | %s"

    // Test `get_process_number`
    // test(get_process_number() == 1, "get process number");

    // std::cerr << get_process_number() << std::endl;
    // std::cerr << get_process_number() << std::endl;
}

int main()
{
    std::cerr << "\nRUNNING TESTS..." << std::endl;

    run_tests();

    std::string msg = allTestsPassed ? "\nALL TESTS PASSED :)\n" : "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;
}