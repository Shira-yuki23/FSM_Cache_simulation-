#ifndef CACHE_CONTROLLER_H
#define CACHE_CONTROLLER_H

#include "Types.h"
#include "Memory.h"
#include <unordered_map>
#include <tuple>
#include <queue>

class CacheController {
private:
    struct CacheEntry {
        uint32_t tag;
        bool valid;
        bool dirty;     
        uint32_t data;
        uint32_t last_used; 
        CacheEntry() : tag(0), valid(false), dirty(false), data(0), last_used(0) {}
    };
    
    SimpleMemory& memory;
    std::unordered_map<uint32_t, CacheEntry> cache;
    int cache_size;
    int block_size;
    int current_cycle;  
    
    CacheState state;
    uint32_t pending_addr;
    uint32_t pending_data;
    bool is_write;
    bool hit_flag;
    uint32_t evict_addr;
    uint32_t evict_data;

    bool cpu_req_ready;
    bool mem_req_pending;
    bool cache_data_valid;
    uint32_t cache_data_out;

    int read_miss_counter;
    int write_miss_counter;
    int write_back_counter;

    uint32_t find_victim(uint32_t index);
    void evict_block(uint32_t index);
    
public:
    CacheController(SimpleMemory& mem, int cache_sz = 4, int blk_sz = 2);
    void step(std::optional<Request> cpu_request = std::nullopt);
    Signals get_signals() const;
    CacheState get_state() const { return state; }
    void print_cache() const;
};

#endif