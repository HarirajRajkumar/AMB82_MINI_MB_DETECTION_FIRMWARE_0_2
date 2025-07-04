// amb82_flash.cpp - Flash Memory Operations Implementation
#include "amb82_flash.h"

// ===== GLOBAL VARIABLES =====
static bool flash_initialized = false;
static uint32_t current_log_index = 0;

// ===== FLASH INITIALIZATION =====
flash_result_t flash_init() {
    INFO_PRINT("Initializing flash memory...");
    
    // Initialize flash memory with base address and size
    FlashMemory.begin(FLASH_MEMORY_APP_BASE, FLASH_SIZE);
    
    // Test basic flash operations
    uint32_t test_value = 0xDEADBEEF;
    uint32_t test_offset = 0x100; // Temporary test location
    
    FlashMemory.writeWord(test_offset, test_value);
    uint32_t read_value = FlashMemory.readWord(test_offset);
    
    if (read_value != test_value) {
        ERROR_PRINT("Flash initialization test failed!");
        return FLASH_ERROR_INIT;
    }
    
    // Load current log index
    current_log_index = flash_get_log_count();
    
    flash_initialized = true;
    INFO_PRINT("Flash memory initialized successfully");
    INFO_PRINT("Max log entries: " + String(MAX_LOG_ENTRIES));
    INFO_PRINT("Current log count: " + String(current_log_index));
    
    return FLASH_SUCCESS;
}

bool flash_is_initialized() {
    return flash_initialized;
}

// ===== CONFIGURATION MANAGEMENT =====
flash_result_t config_save_to_flash() {
    if (!flash_initialized) {
        return FLASH_ERROR_INIT;
    }
    
    INFO_PRINT("Saving configuration to flash...");
    
    // Calculate and set checksum
    system_config.checksum = config_calculate_checksum(&system_config);
    
    // Write configuration structure word by word
    uint32_t* config_ptr = (uint32_t*)&system_config;
    uint32_t config_size = sizeof(system_config_t);
    uint32_t word_count = (config_size + 3) / 4; // Round up to word boundary
    
    for (uint32_t i = 0; i < word_count; i++) {
        FlashMemory.writeWord(FLASH_CONFIG_OFFSET + (i * 4), config_ptr[i]);
    }
    
    // Verify write operation
    flash_result_t verify_result = config_load_from_flash();
    if (verify_result != FLASH_SUCCESS) {
        ERROR_PRINT("Configuration save verification failed!");
        return FLASH_ERROR_VERIFY;
    }
    
    INFO_PRINT("Configuration saved successfully");
    return FLASH_SUCCESS;
}

flash_result_t config_load_from_flash() {
    if (!flash_initialized) {
        return FLASH_ERROR_INIT;
    }
    
    INFO_PRINT("Loading configuration from flash...");
    
    // Create temporary config structure
    system_config_t temp_config;
    
    // Read configuration structure word by word
    uint32_t* config_ptr = (uint32_t*)&temp_config;
    uint32_t config_size = sizeof(system_config_t);
    uint32_t word_count = (config_size + 3) / 4; // Round up to word boundary
    
    for (uint32_t i = 0; i < word_count; i++) {
        config_ptr[i] = FlashMemory.readWord(FLASH_CONFIG_OFFSET + (i * 4));
    }
    
    // Validate version
    if (temp_config.config_version != CONFIG_VERSION) {
        INFO_PRINT("Config version mismatch, using defaults");
        return config_reset_to_defaults();
    }
    
    // Validate checksum
    if (!config_validate_checksum(&temp_config)) {
        ERROR_PRINT("Configuration checksum validation failed!");
        return config_reset_to_defaults();
    }
    
    // Copy validated config to global
    system_config = temp_config;
    
    INFO_PRINT("Configuration loaded successfully");
    return FLASH_SUCCESS;
}

flash_result_t config_reset_to_defaults() {
    INFO_PRINT("Resetting configuration to defaults...");
    
    // Load default configuration
    system_config_t default_config = DEFAULT_CONFIG;
    system_config = default_config;
    
    // Save to flash
    return config_save_to_flash();
}

uint32_t config_calculate_checksum(system_config_t* config) {
    uint32_t checksum = 0;
    uint8_t* data = (uint8_t*)config;
    uint32_t size = sizeof(system_config_t) - sizeof(config->checksum); // Exclude checksum field
    
    for (uint32_t i = 0; i < size; i++) {
        checksum += data[i];
        checksum ^= (checksum << 1);
    }
    
    return checksum;
}

bool config_validate_checksum(system_config_t* config) {
    uint32_t calculated_checksum = config_calculate_checksum(config);
    return (calculated_checksum == config->checksum);
}

// ===== DETECTION LOG MANAGEMENT =====
flash_result_t flash_write_detection_log(detection_result_t* result) {
    if (!flash_initialized || !result) {
        return FLASH_ERROR_INIT;
    }
    
    // Check if log buffer is full
    if (current_log_index >= MAX_LOG_ENTRIES) {
        // Implement circular buffer - overwrite oldest entry
        current_log_index = 0;
        DEBUG_PRINT(3, "Log buffer full, wrapping to beginning");
    }
    
    // Calculate log entry offset
    uint32_t log_offset = FLASH_LOG_OFFSET + (current_log_index * sizeof(detection_result_t));
    
    // Write log entry word by word
    uint32_t* log_ptr = (uint32_t*)result;
    uint32_t log_size = sizeof(detection_result_t);
    uint32_t word_count = (log_size + 3) / 4; // Round up to word boundary
    
    for (uint32_t i = 0; i < word_count; i++) {
        FlashMemory.writeWord(log_offset + (i * 4), log_ptr[i]);
    }
    
    current_log_index++;
    
    // Update statistics
    system_config.total_detections++;
    
    DEBUG_PRINT(3, "Detection log written: index=" + String(current_log_index-1) + 
                   ", class=" + String(result->object_class) + 
                   ", confidence=" + String(result->confidence));
    
    return FLASH_SUCCESS;
}

flash_result_t flash_read_detection_log(uint32_t index, detection_result_t* result) {
    if (!flash_initialized || !result || index >= MAX_LOG_ENTRIES) {
        return FLASH_ERROR_READ;
    }
    
    // Calculate log entry offset
    uint32_t log_offset = FLASH_LOG_OFFSET + (index * sizeof(detection_result_t));
    
    // Read log entry word by word
    uint32_t* log_ptr = (uint32_t*)result;
    uint32_t log_size = sizeof(detection_result_t);
    uint32_t word_count = (log_size + 3) / 4; // Round up to word boundary
    
    for (uint32_t i = 0; i < word_count; i++) {
        log_ptr[i] = FlashMemory.readWord(log_offset + (i * 4));
    }
    
    return FLASH_SUCCESS;
}

uint32_t flash_get_log_count() {
    if (!flash_initialized) {
        return 0;
    }
    
    // Count valid log entries by checking for valid flag
    uint32_t count = 0;
    detection_result_t temp_result;
    
    for (uint32_t i = 0; i < MAX_LOG_ENTRIES; i++) {
        if (flash_read_detection_log(i, &temp_result) == FLASH_SUCCESS) {
            if (temp_result.valid == 1) {
                count++;
            } else {
                break; // Found first invalid entry, stop counting
            }
        }
    }
    
    return count;
}

flash_result_t flash_clear_logs() {
    if (!flash_initialized) {
        return FLASH_ERROR_INIT;
    }
    
    INFO_PRINT("Clearing detection logs...");
    
    // Clear log area by writing zeros
    uint32_t log_area_size = MAX_LOG_ENTRIES * sizeof(detection_result_t);
    uint32_t word_count = (log_area_size + 3) / 4;
    
    for (uint32_t i = 0; i < word_count; i++) {
        FlashMemory.writeWord(FLASH_LOG_OFFSET + (i * 4), 0x00000000);
    }
    
    current_log_index = 0;
    system_config.total_detections = 0;
    
    INFO_PRINT("Detection logs cleared");
    return FLASH_SUCCESS;
}

flash_result_t flash_get_log_stats(uint32_t* total_count, uint32_t* led_count, uint32_t* motherboard_count) {
    if (!flash_initialized || !total_count || !led_count || !motherboard_count) {
        return FLASH_ERROR_READ;
    }
    
    *total_count = 0;
    *led_count = 0;
    *motherboard_count = 0;
    
    detection_result_t temp_result;
    uint32_t log_count = flash_get_log_count();
    
    for (uint32_t i = 0; i < log_count; i++) {
        if (flash_read_detection_log(i, &temp_result) == FLASH_SUCCESS) {
            if (temp_result.valid == 1) {
                (*total_count)++;
                if (temp_result.object_class == CLASS_LED_ON) {
                    (*led_count)++;
                } else if (temp_result.object_class == CLASS_MOTHERBOARD) {
                    (*motherboard_count)++;
                }
            }
        }
    }
    
    return FLASH_SUCCESS;
}

// ===== UTILITY FUNCTIONS =====
void flash_print_config() {
    Serial.println("\n=== FLASH CONFIGURATION ===");
    Serial.println("Config Version: " + String(system_config.config_version));
    Serial.println("System ID: 0x" + String(system_config.system_id, HEX));
    Serial.println("LoRa Interval: " + String(system_config.lora_send_interval) + "ms");
    Serial.println("Detection Threshold: " + String(system_config.detection_threshold));
    Serial.println("Motherboard Threshold: " + String(system_config.motherboard_threshold));
    Serial.println("MB Count Enabled: " + String(system_config.motherboard_count_enabled ? "YES" : "NO"));
    Serial.println("MB Count Threshold: " + String(system_config.motherboard_count_threshold));
    Serial.println("MB Count Window: " + String(system_config.motherboard_count_window_ms/1000) + "s");
    Serial.println("Fan Cycle: " + String(system_config.fan_cycle_interval) + "ms");
    Serial.println("Debug Level: " + String(system_config.debug_level));
    Serial.println("Total Detections: " + String(system_config.total_detections));
    Serial.println("MB Triggers: " + String(system_config.total_motherboard_count_triggers));
    Serial.println("Checksum: 0x" + String(system_config.checksum, HEX));
    Serial.println("==========================\n");
}

void flash_print_logs(uint32_t max_entries) {
    Serial.println("\n=== DETECTION LOGS ===");
    
    uint32_t log_count = flash_get_log_count();
    uint32_t entries_to_show = min(max_entries, log_count);
    
    Serial.println("Total Logs: " + String(log_count));
    Serial.println("Showing last " + String(entries_to_show) + " entries:");
    
    detection_result_t temp_result;
    for (uint32_t i = log_count - entries_to_show; i < log_count; i++) {
        if (flash_read_detection_log(i, &temp_result) == FLASH_SUCCESS) {
            const char* class_name = (temp_result.object_class == CLASS_LED_ON) ? "LED" : 
                                    (temp_result.object_class == CLASS_MOTHERBOARD) ? "MB" : "UNK";
            Serial.println("Log " + String(i) + ": " +
                          String(class_name) + ", " +
                          "Conf=" + String(temp_result.confidence) + ", " +
                          "Time=" + String(temp_result.timestamp));
        }
    }
    Serial.println("======================\n");
}

flash_result_t flash_test_operations() {
    INFO_PRINT("Testing flash operations...");
    
    // Test configuration save/load
    system_config_t backup_config = system_config;
    system_config.debug_level = 99; // Change a value
    
    if (config_save_to_flash() != FLASH_SUCCESS) {
        ERROR_PRINT("Flash test: config save failed");
        return FLASH_ERROR_WRITE;
    }
    
    if (config_load_from_flash() != FLASH_SUCCESS) {
        ERROR_PRINT("Flash test: config load failed");
        return FLASH_ERROR_READ;
    }
    
    if (system_config.debug_level != 99) {
        ERROR_PRINT("Flash test: config verification failed");
        return FLASH_ERROR_VERIFY;
    }
    
    // Restore original config
    system_config = backup_config;
    config_save_to_flash();
    
    INFO_PRINT("Flash operations test passed");
    return FLASH_SUCCESS;
}

const char* flash_result_to_string(flash_result_t result) {
    switch (result) {
        case FLASH_SUCCESS: return "SUCCESS";
        case FLASH_ERROR_INIT: return "INIT_ERROR";
        case FLASH_ERROR_READ: return "READ_ERROR";
        case FLASH_ERROR_WRITE: return "WRITE_ERROR";
        case FLASH_ERROR_VERIFY: return "VERIFY_ERROR";
        case FLASH_ERROR_CHECKSUM: return "CHECKSUM_ERROR";
        case FLASH_ERROR_VERSION: return "VERSION_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}