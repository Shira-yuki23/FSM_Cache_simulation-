#include "CPU.h"

CPU::CPU(const std::vector<Request>& requests) : request_queue(requests), current_index(0) {}
std::optional<Request> CPU::get_next_request() {
    if (current_index < request_queue.size()) {
        return request_queue[current_index++];
    }
    return std::nullopt;
}

bool CPU::has_requests() const {
    return current_index < request_queue.size();
}

void CPU::reset() {
    current_index = 0;
}