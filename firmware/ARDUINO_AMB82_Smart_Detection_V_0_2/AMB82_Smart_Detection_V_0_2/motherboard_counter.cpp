// motherboard_counter.cpp - Motherboard Detection Counter Implementation
#include "motherboard_counter.h"
#include "lora_rak3172.h"

// ===== GLOBAL MOTHERBOARD COUNTER =====
motherboard_counter_t motherboard_counter = {0};

// ===== MOTHERBOARD COUNTER INITIALIZATION =====
void motherboard_counter_init() {
    INFO_PRINT("Initializing motherboard detection counter...");
    
    // Initialize counter structure
    memset(motherboard_counter.detection_timestamps, 0, sizeof(motherboard_counter.detection_timestamps));
    motherboard_counter.buffer_index = 0;
    motherboard_counter.buffer_count = 0;
    motherboard_counter.total_motherboard_detections = 0;
    motherboard_counter.lora_triggers_sent = 0;
    motherboard_counter.last_lora_trigger_time = 0;
    
    // Load settings from config
    motherboard_counter.enabled = system_config.motherboard_count_enabled;
    motherboard_counter.count_threshold = system_config.motherboard_count_threshold;
    motherboard_counter.time_window_ms = system_config.motherboard_count_window_ms;
    
    INFO_PRINT("Motherboard counter initialized:");
    INFO_PRINT("- Enabled: " + String(motherboard_counter.enabled ? "YES" : "NO"));
    INFO_PRINT("- Threshold: " + String(motherboard_counter.count_threshold) + " detections");
    INFO_PRINT("- Window: " + String(motherboard_counter.time_window_ms/1000) + " seconds");
}

// ===== ADD MOTHERBOARD DETECTION =====
void motherboard_counter_add_detection(uint32_t timestamp) {
    if (!motherboard_counter.enabled) {
        return;
    }
    
    // Add timestamp to circular buffer
    motherboard_counter.detection_timestamps[motherboard_counter.buffer_index] = timestamp;
    motherboard_counter.buffer_index = (motherboard_counter.buffer_index + 1) % MOTHERBOARD_DETECTION_BUFFER_SIZE;
    
    // Update count (max is buffer size)
    if (motherboard_counter.buffer_count < MOTHERBOARD_DETECTION_BUFFER_SIZE) {
        motherboard_counter.buffer_count++;
    }
    
    // Update total counter
    motherboard_counter.total_motherboard_detections++;
    
    DEBUG_PRINT(3, "MB detection added: total=" + String(motherboard_counter.total_motherboard_detections) + 
                   ", in_window=" + String(motherboard_counter_get_count_in_window()));
}

// ===== CHECK IF TRIGGER THRESHOLD REACHED =====
bool motherboard_counter_check_trigger() {
    if (!motherboard_counter.enabled) {
        return false;
    }
    
    uint32_t current_count = motherboard_counter_get_count_in_window();
    
    // Check if we've reached the threshold
    if (current_count >= motherboard_counter.count_threshold) {
        
        // Prevent rapid repeated triggers (minimum 30 seconds between triggers)
        uint32_t current_time = millis();
        if (current_time - motherboard_counter.last_lora_trigger_time > 30000) {
            
            motherboard_counter.last_lora_trigger_time = current_time;
            motherboard_counter.lora_triggers_sent++;
            
            // Update config statistics
            system_config.total_motherboard_count_triggers++;
            system_config.last_motherboard_trigger_time = current_time;
            
            INFO_PRINT("🚨 MOTHERBOARD TRIGGER: " + String(current_count) + " detections in " + 
                      String(motherboard_counter.time_window_ms/1000) + "s window");
            
            return true;
        } else {
            DEBUG_PRINT(3, "MB trigger suppressed (too soon since last trigger)");
        }
    }
    
    return false;
}

// ===== GET COUNT IN CURRENT TIME WINDOW =====
uint32_t motherboard_counter_get_count_in_window() {
    if (!motherboard_counter.enabled || motherboard_counter.buffer_count == 0) {
        return 0;
    }
    
    uint32_t current_time = millis();
    uint32_t window_start = current_time - motherboard_counter.time_window_ms;
    uint32_t count = 0;
    
    // Count detections within the time window
    for (int i = 0; i < motherboard_counter.buffer_count; i++) {
        uint32_t detection_time = motherboard_counter.detection_timestamps[i];
        
        // Handle timer overflow (rare but possible)
        if (detection_time > current_time) {
            continue;
        }
        
        if (detection_time >= window_start) {
            count++;
        }
    }
    
    return count;
}

// ===== RESET MOTHERBOARD COUNTER =====
void motherboard_counter_reset() {
    INFO_PRINT("Resetting motherboard detection counter...");
    
    memset(motherboard_counter.detection_timestamps, 0, sizeof(motherboard_counter.detection_timestamps));
    motherboard_counter.buffer_index = 0;
    motherboard_counter.buffer_count = 0;
    motherboard_counter.total_motherboard_detections = 0;
    motherboard_counter.lora_triggers_sent = 0;
    motherboard_counter.last_lora_trigger_time = 0;
    
    // Reset config statistics
    system_config.total_motherboard_count_triggers = 0;
    system_config.last_motherboard_trigger_time = 0;
    
    INFO_PRINT("Motherboard counter reset complete");
}

// ===== PRINT MOTHERBOARD COUNTER STATISTICS =====
void motherboard_counter_print_stats() {
    Serial.println("\n=== MOTHERBOARD COUNTER STATS ===");
    Serial.println("Enabled: " + String(motherboard_counter.enabled ? "YES" : "NO"));
    Serial.println("Threshold: " + String(motherboard_counter.count_threshold) + " detections");
    Serial.println("Time Window: " + String(motherboard_counter.time_window_ms/1000) + " seconds");
    Serial.println("Total MB Detections: " + String(motherboard_counter.total_motherboard_detections));
    Serial.println("Current Window Count: " + String(motherboard_counter_get_count_in_window()));
    Serial.println("LoRa Triggers Sent: " + String(motherboard_counter.lora_triggers_sent));
    
    if (motherboard_counter.last_lora_trigger_time > 0) {
        Serial.println("Last Trigger: " + String((millis() - motherboard_counter.last_lora_trigger_time)/1000) + "s ago");
    } else {
        Serial.println("Last Trigger: Never");
    }
    
    Serial.println("Buffer Usage: " + String(motherboard_counter.buffer_count) + "/" + String(MOTHERBOARD_DETECTION_BUFFER_SIZE));
    Serial.println("==================================\n");
}

// ===== SEND MOTHERBOARD TRIGGER LORA MESSAGE =====
void send_motherboard_trigger_lora() {
    if (!lora_is_initialized()) {
        return;
    }
    
    uint32_t current_count = motherboard_counter_get_count_in_window();
    
    // Format: MT,<count>,<threshold>,<window_seconds>,<timestamp>
    char message_buffer[200];
    snprintf(message_buffer, sizeof(message_buffer),
             "MT,%lu,%lu,%lu,%lu",
             current_count, 
             motherboard_counter.count_threshold,
             motherboard_counter.time_window_ms/1000,
             millis()/1000);
    
    lora_result_t result = lora_send_message(LORA_MSG_MOTHERBOARD_TRIGGER, message_buffer);
    
    if (result == LORA_SUCCESS) {
        INFO_PRINT("[LoRa] Motherboard trigger sent: " + String(message_buffer));
    } else {
        ERROR_PRINT("[LoRa] Motherboard trigger failed: " + String(lora_result_to_string(result)));
    }
}

// ===== MOTHERBOARD COUNTER CONFIGURATION FUNCTIONS =====
bool motherboard_counter_set_enabled(bool enabled) {
    motherboard_counter.enabled = enabled;
    system_config.motherboard_count_enabled = enabled;
    
    INFO_PRINT("Motherboard counter " + String(enabled ? "enabled" : "disabled"));
    return true;
}

bool motherboard_counter_set_threshold(uint32_t threshold) {
    if (threshold < 1 || threshold > 1000) {
        ERROR_PRINT("Invalid threshold range. Use 1-1000.");
        return false;
    }
    
    motherboard_counter.count_threshold = threshold;
    system_config.motherboard_count_threshold = threshold;
    
    INFO_PRINT("Motherboard counter threshold set to " + String(threshold));
    return true;
}

bool motherboard_counter_set_window(uint32_t window_seconds) {
    if (window_seconds < 1 || window_seconds > 300) {
        ERROR_PRINT("Invalid window range. Use 1-300 seconds.");
        return false;
    }
    
    motherboard_counter.time_window_ms = window_seconds * 1000;
    system_config.motherboard_count_window_ms = window_seconds * 1000;
    
    INFO_PRINT("Motherboard counter window set to " + String(window_seconds) + " seconds");
    return true;
}