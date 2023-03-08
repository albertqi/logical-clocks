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

    // Test `get_peer_paths`
    std::vector<std::string> paths = get_peer_paths();
    test(paths.size() == 3, "get_peer_paths size");
    test(paths[0] == SOCKET_PATH "/process_0.socket", "get_peer_paths process 0");
    test(paths[1] == SOCKET_PATH "/process_1.socket", "get_peer_paths process 1");
    test(paths[2] == SOCKET_PATH "/process_2.socket", "get_peer_paths process 2");

    // Test `send_message`
    p0.send_message(paths[1], (uint32_t[]){0, 0, 0});
    test(true, "send_message runs");

    // Test `recv_message`
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    uint32_t ret_clocks[3];
    bool recvd_message = p1.recv_message(ret_clocks);
    test(recvd_message, "recv_message simple");
    test(ret_clocks[0] == 0, "recv_message clock 0");
    test(ret_clocks[1] == 0, "recv_message clock 1");
    test(ret_clocks[2] == 0, "recv_message clock 2");

    // Test `wake_up`
    p0.wake_up(10);
    test(true, "wake_up runs");

    // Test `log`
    FILE *log_file = fopen(LOG_PATH "/process_0.log", "r");
    test(log_file != nullptr, "log file exists");
    if (log_file != nullptr)
    {
        char buffer[256], clocks[256], operation[256];
        int queue_size;
        fscanf(log_file, "%s | %s | %d | %s", buffer, clocks, &queue_size, operation);
        fclose(log_file);
        test((std::string)clocks == "1,0,0", "log clocks");
        test(queue_size == 0, "log queue size");
        test((std::string)operation == "INTERNAL", "log operation");
    }

    // More tests for `recv_message`
    p0.wake_up(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    recvd_message = p2.recv_message(ret_clocks);
    test(recvd_message, "recv_message process 2");
    test(ret_clocks[0] == 1, "recv_message process 2 clock 0");
    test(ret_clocks[1] == 0, "recv_message process 2 clock 1");
    test(ret_clocks[2] == 0, "recv_message process 2 clock 2");

    // More tests for `log`
    log_file = fopen(LOG_PATH "/process_0.log", "r");
    test(log_file != nullptr, "log file exists again");
    if (log_file != nullptr)
    {
        char buffer[256], clocks[256], operation[256];
        int queue_size;
        fgets(buffer, 256, log_file);
        fscanf(log_file, "%s | %s | %d | %s", buffer, clocks, &queue_size, operation);
        fclose(log_file);
        test((std::string)clocks == "2,0,0", "log clocks again");
        test(queue_size == 0, "log queue size again");
        test((std::string)operation == "SEND", "log operation again");
    }

    // Even more tests for `log`
    p2.wake_up(2);
    p2.wake_up(3);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    p0.wake_up(10);
    p1.wake_up(10);
    p1.wake_up(10);
    log_file = fopen(LOG_PATH "/process_0.log", "r");
    test(log_file != nullptr, "log file exists process 0");
    if (log_file != nullptr)
    {
        char buffer[256], clocks[256], operation[256];
        int queue_size;
        fgets(buffer, 256, log_file);
        fgets(buffer, 256, log_file);
        fscanf(log_file, "%s | %s | %d | %s", buffer, clocks, &queue_size, operation);
        fclose(log_file);
        test((std::string)clocks == "3,0,1", "log clocks process 0");
        test(queue_size == 0, "log queue size process 0");
        test((std::string)operation == "RECEIVE", "log operation process 0");
    }
    log_file = fopen(LOG_PATH "/process_1.log", "r");
    test(log_file != nullptr, "log file exists process 0");
    if (log_file != nullptr)
    {
        char buffer[256], clocks[256], operation[256];
        int queue_size;
        fscanf(log_file, "%s | %s | %d | %s", buffer, clocks, &queue_size, operation);
        test((std::string)clocks == "0,1,0", "log clocks process 1");
        test(queue_size == 1, "log queue size process 1");
        test((std::string)operation == "RECEIVE", "log operation process 1");
        fscanf(log_file, "%s | %s | %d | %s", buffer, clocks, &queue_size, operation);
        test((std::string)clocks == "0,2,1", "log clocks process 1 again");
        test(queue_size == 0, "log queue size process 1 again");
        test((std::string)operation == "RECEIVE", "log operation process 1 again");
        fclose(log_file);
    }
    log_file = fopen(LOG_PATH "/process_2.log", "r");
    test(log_file != nullptr, "log file exists process 2");
    if (log_file != nullptr)
    {
        char buffer[256], clocks[256], operation[256];
        int queue_size;
        fscanf(log_file, "%s | %s | %d | %s", buffer, clocks, &queue_size, operation);
        test((std::string)clocks == "0,0,1", "log clocks process 2");
        test(queue_size == 0, "log queue size process 2");
        test((std::string)operation == "SEND", "log operation process 2");
        fscanf(log_file, "%s | %s | %d | %s", buffer, clocks, &queue_size, operation);
        test((std::string)clocks == "0,0,2", "log clocks process 2 again");
        test(queue_size == 0, "log queue size process 2 again");
        test((std::string)operation == "DOUBLE", "log operation process 2 again");
        fclose(log_file);
    }
}

int main()
{
    std::cerr << "\nRUNNING TESTS..." << std::endl;

    run_tests();

    std::string msg = allTestsPassed ? "\nALL TESTS PASSED :)\n" : "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;
}