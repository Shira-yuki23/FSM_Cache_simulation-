#include "Memory.h"
#include <iostream>
#include <iomanip>

SimpleMemory::SimpleMemory(int latency) 
    : latency_cycles(latency), bank_bits(2), row_bits(10), col_bits(6), 
      reads(0), writes(0), row_buffer_hits(0), row_buffer_misses(0), bank_conflicts(0),
      verbose_mode(false), name("MainMemory"), state(MemState::IDLE), 
      remaining_latency(0), pending_addr(0), pending_data(0) {
    
    bank_states.resize(1 << bank_bits);
}

SimpleMemory::~SimpleMemory() {
    if (verbose_mode && (reads + writes > 0)) {
        print_stats();
    }
}

int SimpleMemory::calculate_dram_latency(uint32_t addr, bool is_write) {
    if (is_write) writes++;
    else reads++;

    uint32_t col_mask  = (1 << col_bits) - 1;
    uint32_t bank_mask = (1 << bank_bits) - 1;
    uint32_t row_mask  = (1 << row_bits) - 1;

    uint32_t col = (addr >> 2) & col_mask;
    uint32_t bank = (addr >> (2 + col_bits)) & bank_mask;
    uint32_t row = (addr >> (2 + col_bits + bank_bits)) & row_mask;

    int latency = 10;

    if (verbose_mode) {
        std::cout << "[DRAM] " << (is_write ? "WRITE" : "READ ") 
                  << " Addr: 0x" << std::hex << std::setw(8) << std::setfill('0') << addr 
                  << std::dec << " -> Bank " << bank << ", Row " << row << ", Col " << col;
    }

    BankState& bs = bank_states[bank];
    if (bs.row_active) {
        if (bs.active_row == row) {
            row_buffer_hits++;
            latency = 2;
            if (verbose_mode) std::cout << " (Row HIT: 2 cycles)\n";
        } else {
            row_buffer_misses++;
            bank_conflicts++;
            latency = 10;
            if (verbose_mode) std::cout << " (Bank CONFLICT w/ Row " << bs.active_row << ": 10 cycles)\n";
            bs.active_row = row;
        }
    } else {
        row_buffer_misses++;
        latency = 10;
        if (verbose_mode) std::cout << " (Row MISS - Empty Bank: 10 cycles)\n";
        bs.row_active = true;
        bs.active_row = row;
    }
    
    return latency;
}

bool SimpleMemory::start_access(uint32_t addr, bool is_write, uint32_t data) {
    if (is_busy()) {
        return false;
    }
    
    pending_addr = addr;
    pending_data = data;
    state = is_write ? MemState::WRITING : MemState::READING;
    remaining_latency = calculate_dram_latency(addr, is_write);
    
    return true;
}

bool SimpleMemory::is_busy() const {
    return state != MemState::IDLE;
}

bool SimpleMemory::is_ready(uint32_t& result) {
    if (state == MemState::IDLE) {
        return false;
    }
    
    if (remaining_latency > 0) {
        remaining_latency--;
    }
    
    if (remaining_latency == 0) {
        if (state == MemState::READING) {
            auto it = data.find(pending_addr);
            result = (it != data.end()) ? it->second : 0;
            if (verbose_mode) std::cout << "[DRAM] Read Complete: Data = " << result << "\n";
        } else if (state == MemState::WRITING) {
            auto it = data.find(pending_addr);
            if (it == data.end()) {
                data.insert({pending_addr, pending_data});
            } else {
                it->second = pending_data;
            }
            if (verbose_mode) std::cout << "[DRAM] Write Complete\n";
        }
        
        state = MemState::IDLE;
        return true;
    }
    
    return false;
}

uint32_t SimpleMemory::read(uint32_t addr) {
    calculate_dram_latency(addr, false);
    
    auto it = data.find(addr);
    if (it != data.end()) {
        return it->second;
    }
    return 0;
}

void SimpleMemory::write(uint32_t addr, uint32_t value) {
    calculate_dram_latency(addr, true);
    auto it = data.find(addr);
    if (it == data.end()) {
        data.insert({addr, value});
    } else {
        it->second = value;
    }
}

void SimpleMemory::print_stats() const {
    std::cout << "\n======================================================\n";
    std::cout << " DRAM Memory Statistics: " << name << "\n";
    std::cout << "======================================================\n";
    std::cout << " Total Accesses      : " << (reads + writes) << "\n";
    std::cout << "   - Reads           : " << reads << "\n";
    std::cout << "   - Writes          : " << writes << "\n";
    std::cout << "\n Row Buffer Activity:\n";
    std::cout << "   - Hits            : " << row_buffer_hits << "\n";
    std::cout << "   - Misses          : " << row_buffer_misses << "\n";
    std::cout << "   - Bank Conflicts  : " << bank_conflicts << "\n";
    
    if (reads + writes > 0) {
        float hit_rate = (float)row_buffer_hits / (row_buffer_hits + row_buffer_misses) * 100.0f;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << " Row Buffer Hit Rate : " << hit_rate << "%\n";
    }
    std::cout << "======================================================\n\n";
}

void SimpleMemory::reset_stats() {
    reads = 0; writes = 0;
    row_buffer_hits = 0; row_buffer_misses = 0; bank_conflicts = 0;
    for (auto& bs : bank_states) bs.row_active = false;
    state = MemState::IDLE;
    remaining_latency = 0;
}

void SimpleMemory::hex_dump(uint32_t start_addr, uint32_t length) {
    std::cout << "\n[Memory Dump] 0x" << std::hex << start_addr << " - 0x" << (start_addr + length) << ":\n";
    for (uint32_t addr = start_addr; addr < start_addr + length; addr += 4) {
        if (addr % 16 == 0 || addr == start_addr) {
            if (addr != start_addr) std::cout << "\n";
            std::cout << "  0x" << std::hex << std::setfill('0') << std::setw(8) << addr << ": ";
        }
        
        auto it = data.find(addr);
        uint32_t val = (it != data.end()) ? it->second : 0;
        
        std::cout << std::setfill('0') << std::setw(8) << val << " ";
    }
    std::cout << std::dec << "\n\n";
}