// motherboard_counter.h - Motherboard Detection Counter Module
#ifndef MOTHERBOARD_COUNTER_H
#define MOTHERBOARD_COUNTER_H

#include "config.h"

// ===== GLOBAL MOTHERBOARD COUNTER =====
extern motherboard_counter_t motherboard_counter;

// ===== MOTHERBOARD COUNTER FUNCTIONS =====
void motherboard_counter_init();
void motherboard_counter_add_detection(uint32_t timestamp);
bool motherboard_counter_check_trigger();
uint32_t motherboard_counter_get_count_in_window();
void motherboard_counter_reset();
void motherboard_counter_print_stats();

// ===== MOTHERBOARD COUNTER CONFIGURATION =====
bool motherboard_counter_set_enabled(bool enabled);
bool motherboard_counter_set_threshold(uint32_t threshold);
bool motherboard_counter_set_window(uint32_t window_seconds);

// ===== LORA TRIGGER FUNCTIONS =====
void send_motherboard_trigger_lora();

#endif // MOTHERBOARD_COUNTER_H