#include "Memory.h"

SimpleMemory::SimpleMemory(int latency) : latency_cycles(latency) {}

uint32_t SimpleMemory::read(uint32_t addr) {
    auto it = data.find(addr);
    if (it != data.end()) {
        return it->second;
    }
    return 0;  
}

void SimpleMemory::write(uint32_t addr, uint32_t value) {
    data[addr] = value;
}