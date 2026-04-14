#ifndef TESTS_H
#define TESTS_H

void run_all_tests();
void run_all_tests_continuous();
void test_read_miss_hit();
void test_write_allocate();
void test_write_back_no_immediate_write();
void test_dirty_eviction_write_back();
void test_multiple_writes_same_address();
void test_mixed_sequence();
void test_cache_capacity();
void test_clean_eviction();

#endif