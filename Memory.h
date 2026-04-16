#ifndef MEMORY_H
#define MEMORY_H

#include <unordered_map>
#include <cstdint>

class SimpleMemory {
private:
    std::unordered_map<uint32_t, uint32_t> data;
    int latency_cycles;
    
public:
    SimpleMemory(int latency = 2);
    uint32_t read(uint32_t addr);
    void write(uint32_t addr, uint32_t value);
    bool has_pending() const { return false; }
};

#endif