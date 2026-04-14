#include "CacheController.h"
#include <iostream>
#include <algorithm>

CacheController::CacheController(SimpleMemory& mem, int cache_sz, int blk_sz)
    : memory(mem), cache_size(cache_sz), block_size(blk_sz), current_cycle(0),
      state(CacheState::IDLE), pending_addr(0), pending_data(0),
      is_write(false), hit_flag(false), evict_addr(0), evict_data(0), 
      cpu_req_ready(true), mem_req_pending(false), cache_data_valid(false), 
      cache_data_out(0), read_miss_counter(0), write_miss_counter(0), 
      write_back_counter(0) {}
      
std::pair<uint32_t, uint32_t> get_tag_index(uint32_t addr, int cache_size) {
    uint32_t index = addr % cache_size;
    uint32_t tag = addr / cache_size;
    return {tag, index};
}

uint32_t CacheController::find_victim(uint32_t index) {
    // Simple LRU: find least recently used block
    uint32_t victim_tag = 0;
    uint32_t min_used = UINT32_MAX;
    
    for (auto& [idx, entry] : cache) {
        if (idx == index) {
            if (entry.valid && entry.last_used < min_used) {
                min_used = entry.last_used;
                victim_tag = entry.tag;
            }
        }
    }
    return victim_tag;
}

void CacheController::evict_block(uint32_t index) {
    auto it = cache.find(index);
    if (it != cache.end() && it->second.valid && it->second.dirty) {
        // Write back dirty block to memory
        uint32_t addr = (it->second.tag * cache_size) + index;
        std::cout << "[WRITE-BACK] Evicting dirty block at addr=" << addr 
                  << ", data=" << it->second.data << "\n";
        memory.write(addr, it->second.data);
        it->second.dirty = false;
    }
}

void CacheController::step(std::optional<Request> cpu_request) {
    current_cycle++;
    
    switch (state) {
        case CacheState::IDLE:
            if (cpu_request.has_value()) {
                pending_addr = cpu_request->addr;
                pending_data = cpu_request->data.value_or(0);
                is_write = cpu_request->is_write;
                state = CacheState::COMPARE;
                cpu_req_ready = false;
                std::cout << "[Cycle " << current_cycle << "] IDLE -> COMPARE (addr=" 
                          << pending_addr << ", " << (is_write ? "WRITE" : "READ") << ")\n";
            } else {
                cpu_req_ready = true;
            }
            break;
            
        case CacheState::COMPARE: {
            auto [tag, idx] = get_tag_index(pending_addr, cache_size);
            bool hit = (cache.find(idx) != cache.end() && 
                        cache[idx].valid && 
                        cache[idx].tag == tag);
            
            if (hit) {
                hit_flag = true;
                cache[idx].last_used = current_cycle;
                
                if (is_write) {
                    // WRITE HIT: Just update cache, mark dirty
                    cache[idx].data = pending_data;
                    cache[idx].dirty = true;
                    cache_data_valid = true;
                    cache_data_out = pending_data;
                    std::cout << "[Cycle " << current_cycle << "] WRITE HIT (idx=" << idx 
                              << ") - Marked DIRTY\n";
                    state = CacheState::HIT;
                } else {
                    // READ HIT
                    cache_data_valid = true;
                    cache_data_out = cache[idx].data;
                    std::cout << "[Cycle " << current_cycle << "] READ HIT (idx=" << idx 
                              << ", data=" << cache_data_out << ")\n";
                    state = CacheState::HIT;
                }
            } else {
                hit_flag = false;
                std::cout << "[Cycle " << current_cycle << "] MISS (idx=" << idx 
                          << ") - Need to fetch from memory\n";
                
                // Check if we need to evict
                if (cache.find(idx) != cache.end() && cache[idx].valid) {
                    std::cout << "[Cycle " << current_cycle << "] Slot " << idx 
                              << " is occupied - need eviction\n";
                    
                    if (cache[idx].dirty) {
                        // Need to write back dirty block first
                        state = CacheState::WRITE_BACK;
                        evict_addr = (cache[idx].tag * cache_size) + idx;
                        evict_data = cache[idx].data;
                        write_back_counter = 0;
                        mem_req_pending = true;
                        std::cout << "[Cycle " << current_cycle << "] WRITE_BACK needed for dirty block\n";
                        break;
                    }
                }
                
                // No dirty block to evict, proceed with miss handling
                if (is_write) {
                    state = CacheState::WRITE_MISS;
                    write_miss_counter = 0;
                } else {
                    state = CacheState::READ_MISS;
                    read_miss_counter = 0;
                }
                mem_req_pending = true;
            }
            break;
        }
        
        case CacheState::WRITE_BACK:
            write_back_counter++;
            std::cout << "[Cycle " << current_cycle << "] WRITE_BACK: writing dirty block to memory (" 
                      << write_back_counter << "/2)\n";
            
            if (write_back_counter >= 2) {
                // Write back to memory
                memory.write(evict_addr, evict_data);
                mem_req_pending = false;
                std::cout << "[Cycle " << current_cycle << "] WRITE_BACK complete\n";
                
                // Now handle the original miss
                auto [tag, idx] = get_tag_index(pending_addr, cache_size);
                if (is_write) {
                    state = CacheState::WRITE_MISS;
                    write_miss_counter = 0;
                } else {
                    state = CacheState::READ_MISS;
                    read_miss_counter = 0;
                }
            }
            break;
            
        case CacheState::READ_MISS:
            read_miss_counter++;
            std::cout << "[Cycle " << current_cycle << "] READ_MISS: fetching from memory (" 
                      << read_miss_counter << "/2)\n";
            
            if (read_miss_counter >= 2) {
                // Fetch block from memory
                uint32_t data = memory.read(pending_addr);
                auto [tag, idx] = get_tag_index(pending_addr, cache_size);
                
                // Evict if slot is occupied (but not dirty - already handled)
                if (cache.find(idx) != cache.end() && cache[idx].valid) {
                    cache[idx].valid = false;
                }
                
                // Allocate new block
                cache[idx].tag = tag;
                cache[idx].valid = true;
                cache[idx].dirty = false;
                cache[idx].data = data;
                cache[idx].last_used = current_cycle;
                
                cache_data_valid = true;
                cache_data_out = data;
                mem_req_pending = false;
                state = CacheState::HIT;
                std::cout << "[Cycle " << current_cycle << "] READ_MISS -> HIT (loaded data=" << data << ")\n";
            }
            break;
            
        case CacheState::WRITE_MISS:
            write_miss_counter++;
            std::cout << "[Cycle " << current_cycle << "] WRITE_MISS: write-allocate (" 
                      << write_miss_counter << "/2)\n";
            
            if (write_miss_counter >= 2) {
                // Write-allocate: fetch block first, then write
                auto [tag, idx] = get_tag_index(pending_addr, cache_size);
                
                // Fetch from memory (simulate latency)
                uint32_t old_data = memory.read(pending_addr);
                
                // Evict if slot is occupied
                if (cache.find(idx) != cache.end() && cache[idx].valid) {
                    cache[idx].valid = false;
                }
                
                // Allocate and write
                cache[idx].tag = tag;
                cache[idx].valid = true;
                cache[idx].dirty = true;  // Mark dirty because we're writing
                cache[idx].data = pending_data;
                cache[idx].last_used = current_cycle;
                
                // Don't write to memory yet (write-back!)
                std::cout << "[Cycle " << current_cycle << "] WRITE_MISS: allocated block and marked DIRTY\n";
                
                cache_data_valid = true;
                cache_data_out = pending_data;
                mem_req_pending = false;
                state = CacheState::HIT;
            }
            break;
            
        case CacheState::HIT:
            std::cout << "[Cycle " << current_cycle << "] HIT: completing request\n";
            cpu_req_ready = true;
            cache_data_valid = false;
            state = CacheState::IDLE;
            break;
    }
}

Signals CacheController::get_signals() const {
    Signals sigs;
    sigs.cpu_req_ready = cpu_req_ready;
    sigs.mem_req_pending = mem_req_pending;
    sigs.cache_data_valid = cache_data_valid;
    sigs.hit = hit_flag;
    sigs.cache_data = cache_data_out;
    return sigs;
}

void CacheController::print_cache() const {
    std::cout << "\n=== Cache Contents (Write-Back) ===\n";
    for (const auto& [idx, entry] : cache) {
        if (entry.valid) {
            std::cout << "Index " << idx << ": tag=" << entry.tag 
                      << ", dirty=" << (entry.dirty ? "YES" : "NO")
                      << ", data=" << entry.data 
                      << ", last_used=" << entry.last_used << "\n";
        } else {
            std::cout << "Index " << idx << ": INVALID\n";
        }
    }
    std::cout << "===================================\n";
}