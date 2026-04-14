#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <optional>

struct Request {
    uint32_t addr;
    bool is_write;
    std::optional<uint32_t> data;  
};

struct Signals {
    bool cpu_req_ready;
    bool mem_req_pending;
    bool cache_data_valid;
    bool hit;
    uint32_t cache_data;
};

enum class CacheState {
    IDLE,
    COMPARE,
    READ_MISS,
    WRITE_MISS,
    WRITE_BACK,
    HIT
};

#endif