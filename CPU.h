#ifndef CPU_H
#define CPU_H
#include "Types.h"
#include <vector>

class CPU {
private:
    std::vector<Request> request_queue;
    size_t current_index;
public:
    CPU(const std::vector<Request>& requests);
    std::optional<Request> get_next_request();
    bool has_requests() const;
    void reset();
};

#endif