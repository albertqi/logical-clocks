#pragma once

#include <string>
#include <vector>

std::vector<std::string> get_peer_paths();

int get_process_number();

int uniform_random_number(int start_range, int end_range);

void send_message(std::string socket_path, uint32_t clocks[3]);

bool recv_message(uint32_t clocks_ret[3]);

void log(std::string log_message);

void wake_up(int roll);

int setup_network_and_log();

void cleanup_network_and_log();

void start_process();

void stop_process();
