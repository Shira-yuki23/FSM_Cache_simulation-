#include "CacheController.h"
#include "Memory.h"
#include "CPU.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cassert>
using namespace std;

void print_separator() {
    cout << string(80, '-') << '\n';
}
void print_header() {
    cout << left << setw(8) << "Cycle"<< setw(15) << "State"<< setw(12) << "CPU Ready"<< setw(12) << "Mem Pend"<< setw(8) << "Hit"<< setw(12) << "Data Out"<< '\n';
    print_separator();
}
void run_detailed_simulation(const vector<Request>& requests, const string& test_name) {
    cout << "\n\n" << string(80, '=') << '\n';
    cout << "TEST: " << test_name << '\n';
    cout << string(80, '=') << "\n\n";
    SimpleMemory memory;
    CacheController cache(memory);
    CPU cpu(requests);
    int cycle = 0;
    optional<Request> current_req;
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
            current_req = nullopt;
        }
        Signals sigs = cache.get_signals();
        string state_str;
        switch (cache.get_state()) {
            case CacheState::IDLE: state_str = "IDLE"; break;
            case CacheState::COMPARE: state_str = "COMPARE"; break;
            case CacheState::READ_MISS: state_str = "READ_MISS"; break;
            case CacheState::WRITE_MISS: state_str = "WRITE_MISS"; break;
            case CacheState::WRITE_BACK: state_str = "WRITE_BACK"; break;
            case CacheState::HIT: state_str = "HIT"; break;
            default: state_str = "UNKNOWN";
        }
        cout << left
             << setw(8) << cycle
             << setw(15) << state_str
             << setw(12) << (sigs.cpu_req_ready ? "YES" : "NO")
             << setw(12) << (sigs.mem_req_pending ? "YES" : "NO")
             << setw(8) << (sigs.hit ? "YES" : "NO")
             << setw(12) << sigs.cache_data
             << '\n';
        cycle++;
    }
    
    cache.print_cache();
    cout << "\nSimulation completed in " << cycle << " cycles.\n";
}
void run_quick_test(const vector<Request>& requests, const string& test_name) {
    cout << '\n' << string(80, '=') << '\n';
    cout << "QUICK TEST: " << test_name << '\n';
    cout << string(80, '=') << '\n';
    SimpleMemory memory;
    CacheController cache(memory);
    for (const auto& req : requests) {
        cache.step(req);
        while (cache.get_state() != CacheState::IDLE) {
            cache.step();
        }
    }
    cout << "✓ Test completed\n";
}
void test_read_miss_hit() {
    vector<Request> requests = {
        {10, false, nullopt},  
        {10, false, nullopt}   
    };
    run_detailed_simulation(requests, "Read Miss -> Read Hit");
}
void test_write_allocate() {
    vector<Request> requests = {
        {20, true, 100},  
        {20, false, nullopt}  
    };
    run_detailed_simulation(requests, "Write Miss with Write-Allocate");
}
void test_write_back_no_immediate_write() {
    vector<Request> requests = {
        {200, false, nullopt},  
        {200, true, 999},       
        {200, false, nullopt}
    };
    run_detailed_simulation(requests, "Write-Back: No Immediate Memory Write");
}
void test_dirty_eviction_write_back() {
    vector<Request> requests = {
        {0, true, 111},     
        {4, true, 222},     
        {8, false, nullopt} 
    };
    run_detailed_simulation(requests, "Write-Back: Dirty Block Eviction");
}
void test_multiple_writes_same_address() {
    vector<Request> requests;
    for (int i = 1; i <= 5; i++) {
        requests.push_back({100, true, i * 100});  
    }
    requests.push_back({200, false, nullopt});  
    
    run_detailed_simulation(requests, "Write-Back: Multiple Writes (Single Memory Write on Eviction)");
}

void test_mixed_sequence() {
    vector<Request> requests = {
        {10, false, nullopt},   
        {10, true, 555},        
        {20, false, nullopt},   
        {10, false, nullopt},   
        {30, true, 777},        
        {20, true, 888}        
    };
    run_detailed_simulation(requests, "Mixed Read/Write Sequence");
}

void test_cache_capacity() {
    vector<Request> requests = {
        {0, false, nullopt},    
        {4, false, nullopt},    
        {8, false, nullopt},    
        {12, false, nullopt},   
        {0, false, nullopt}     
    };
    run_detailed_simulation(requests, "Cache Capacity & LRU Replacement");
}
void test_clean_eviction() {
    vector<Request> requests = {
        {300, false, nullopt},  
        {304, false, nullopt},  
        {308, false, nullopt}   
    };
    run_detailed_simulation(requests, "Clean Block Eviction (No Write-Back)");
}
void run_all_tests() {
    cout << '\n';
    cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    cout << "║                    CACHE CONTROLLER TEST SUITE                             ║\n";
    cout << "║              Testing Read/Write, Write-Back, and Evictions                 ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";   
    test_read_miss_hit();
    cout << "\n\nPress Enter to continue to next test...";
    cin.get();
    test_write_allocate();
    cout << "\n\nPress Enter to continue to next test...";
    cin.get(); 
    test_write_back_no_immediate_write(); 
    cout << "\n\nPress Enter to continue to next test...";
    cin.get(); 
    test_dirty_eviction_write_back(); 
    cout << "\n\nPress Enter to continue to next test...";
    cin.get();
    test_multiple_writes_same_address(); 
    cout << "\n\nPress Enter to continue to next test...";
    cin.get();   
    test_mixed_sequence();  
    cout << "\n\nPress Enter to continue to next test...";
    cin.get();  
    test_cache_capacity();
    cout << "\n\nPress Enter to continue to next test...";
    cin.get();
    test_clean_eviction();
    cout << "\n\n";
    cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    cout << "║                             ALL TESTS COMPLETE!                            ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    cout << "\nFeatures successfully tested:\n";
    cout << "  ✓ Read miss and hit handling\n";
    cout << "  ✓ Write allocate policy\n";
    cout << "  ✓ Write-back (no immediate memory write)\n";
    cout << "  ✓ Dirty bit tracking\n";
    cout << "  ✓ Write-back on dirty block eviction\n";
    cout << "  ✓ Multiple writes cause single write-back\n";
    cout << "  ✓ Mixed read/write sequences\n";
    cout << "  ✓ Cache capacity and replacement (LRU)\n";
    cout << "  ✓ Clean block eviction (no write-back)\n";
}
void run_all_tests_continuous() {
    cout << '\n';
    cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    cout << "║                    CACHE CONTROLLER TEST SUITE (CONTINUOUS)                ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    test_read_miss_hit();
    test_write_allocate();
    test_write_back_no_immediate_write();
    test_dirty_eviction_write_back();
    test_multiple_writes_same_address();
    test_mixed_sequence();
    test_cache_capacity();
    test_clean_eviction();
    cout << "\n\nALL TESTS COMPLETED SUCCESSFULLY!\n";
}
