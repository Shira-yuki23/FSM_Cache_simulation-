#include "CacheController.h"
#include "CPU.h"
#include <iostream>
#include <iomanip>
#include <vector>

void print_separator() {
    std::cout << std::string(80, '-') << "\n";
}

void print_header() {
    std::cout << std::left 
              << std::setw(8) << "Cycle"
              << std::setw(15) << "State"
              << std::setw(12) << "CPU Ready"
              << std::setw(12) << "Mem Pend"
              << std::setw(8) << "Hit"
              << std::setw(12) << "Data Out"
              << "\n";
    print_separator();
}

void run_simulation(const std::vector<Request>& requests, const std::string& test_name) {
    std::cout << "\n\n" << std::string(80, '=') << "\n";
    std::cout << "TEST: " << test_name << "\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    SimpleMemory memory;
    CacheController cache(memory);
    CPU cpu(requests);
    
    int cycle = 0;
    std::optional<Request> current_req;
    const int MAX_CYCLES = 100;
    
    print_header();
    
    while (cycle < MAX_CYCLES) {
        if (cache.get_signals().cpu_req_ready && !current_req.has_value()) {
            current_req = cpu.get_next_request();
            if (!current_req.has_value() && !cache.get_signals().mem_req_pending) {
                break;  
            }
        }
        

        cache.step(current_req);

        if (cache.get_state() == CacheState::IDLE && current_req.has_value()) {
            current_req = std::nullopt;
        }

        Signals sigs = cache.get_signals();
        std::string state_str;
        switch (cache.get_state()) {
            case CacheState::IDLE: state_str = "IDLE"; break;
            case CacheState::COMPARE: state_str = "COMPARE"; break;
            case CacheState::READ_MISS: state_str = "READ_MISS"; break;
            case CacheState::WRITE_MISS: state_str = "WRITE_MISS"; break;
            case CacheState::HIT: state_str = "HIT"; break;
            default: state_str = "UNKNOWN";
        }
        
        std::cout << std::left
                  << std::setw(8) << cycle
                  << std::setw(15) << state_str
                  << std::setw(12) << (sigs.cpu_req_ready ? "YES" : "NO")
                  << std::setw(12) << (sigs.mem_req_pending ? "YES" : "NO")
                  << std::setw(8) << (sigs.hit ? "YES" : "NO")
                  << std::setw(12) << sigs.cache_data
                  << "\n";
        
        cycle++;
    }
    
    cache.print_cache();
    std::cout << "\nSimulation completed in " << cycle << " cycles.\n";
}

int main() {
    std::vector<Request> test1 = {
        {10, false, std::nullopt},  
        {10, false, std::nullopt}   
    };
    run_simulation(test1, "Read Miss -> Read Hit");
    
    std::vector<Request> test2 = {
        {20, true, 100},   
        {20, false, std::nullopt}  
    };
    run_simulation(test2, "Write Miss with Write-Allocate");

    std::vector<Request> test3 = {
        {0, false, std::nullopt},   
        {4, false, std::nullopt},  
        {8, false, std::nullopt},   
        {12, false, std::nullopt}, 
        {0, false, std::nullopt}    
    };
    run_simulation(test3, "Multiple Accesses with Eviction");

    std::vector<Request> test4 = {
        {100, false, std::nullopt},  
        {100, true, 999},            
        {100, false, std::nullopt},  
        {200, false, std::nullopt},  
        {200, false, std::nullopt}   
    };
    run_simulation(test4, "Mixed Read/Write Sequence");
    
    return 0;
}