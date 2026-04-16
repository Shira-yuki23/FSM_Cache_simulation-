#ifndef MEMORY_H
#define MEMORY_H

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <string>

class SimpleMemory {
public:
    enum class MemState { IDLE, READING, WRITING };

private:
    std::unordered_map<uint32_t, uint32_t> data;
    int latency_cycles;
    
    uint32_t bank_bits;
    uint32_t row_bits;
    uint32_t col_bits;
    
    struct BankState {
        bool row_active;
        uint32_t active_row;
        BankState() : row_active(false), active_row(0) {}
    };
    std::vector<BankState> bank_states;
    
    uint64_t reads;
    uint64_t writes;
    uint64_t row_buffer_hits;
    uint64_t row_buffer_misses;
    uint64_t bank_conflicts;

    bool verbose_mode;
    std::string name;

    MemState state;
    int remaining_latency;
    uint32_t pending_addr;
    uint32_t pending_data;

    int calculate_dram_latency(uint32_t addr, bool is_write);

public:
    SimpleMemory(int latency = 2);
    ~SimpleMemory();

    bool start_access(uint32_t addr, bool is_write, uint32_t data = 0);

    bool is_ready(uint32_t& result);

    bool is_busy() const;

    uint32_t read(uint32_t addr);
    void write(uint32_t addr, uint32_t value);
    bool has_pending() const { return is_busy(); }
    
    void set_name(const std::string& n) { name = n; }
    void set_verbose(bool v) { verbose_mode = v; }
    void print_stats() const;
    void reset_stats();
    void hex_dump(uint32_t start_addr, uint32_t length);
};

#endif