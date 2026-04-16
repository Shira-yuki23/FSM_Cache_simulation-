#include "Memory.h"
#include "CacheController.h"
#include "CPU.h"
#include "tests.h"
#include <iostream>
#include <string>

static void print_banner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║          FSM-Based Cache Controller Simulation               ║\n";
    std::cout << "  ║                                                              ║\n";
    std::cout << "  ║  Memory  : Advanced DRAM Model (Bank/Row/Col simulation)     ║\n";
    std::cout << "  ║  Cache   : 4-entry direct-mapped, LRU eviction               ║\n";
    std::cout << "  ║  FSM     : IDLE → COMPARE → HIT / READ_MISS / WRITE_MISS     ║\n";
    std::cout << "  ║            → WRITE_BACK → [back to miss handling]            ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

static void advanced_memory_demo() {
    std::cout << "  [DRAM Demo] Initializing advanced memory system...\n";
    SimpleMemory dram(2);
    dram.set_name("Main System DRAM");
    dram.set_verbose(true);
    
    std::cout << "\n  [DRAM Demo] Simulating spatial locality (Row Buffer Hits)...\n";
    for(uint32_t addr = 0x1000; addr < 0x1010; addr += 4) {
        dram.write(addr, addr * 2);
    }
    for(uint32_t addr = 0x1000; addr < 0x1010; addr += 4) {
        volatile uint32_t val = dram.read(addr);
        (void)val;
    }
    
    std::cout << "\n  [DRAM Demo] Simulating Bank Conflicts / Row Buffer Misses...\n";
    dram.write(0x2000, 0xAABBCCDD);
    dram.write(0x8000, 0x11223344);
    dram.read(0x2004);
    
    dram.hex_dump(0x1000, 16);
    
    dram.print_stats();
    dram.set_verbose(false);
    
    std::cout << "  [DRAM Demo] Demo complete. Proceeding to Cache tests:\n";
    std::cout << "  --------------------------------------------------------------\n";
}

int main() {
    print_banner();
    advanced_memory_demo();

    run_all_tests_continuous();

    std::cout << "\n  [Main] Simulation finished. Exiting cleanly.\n\n";
    return 0;
}