// amb82_flash.h - Flash Memory Operations
#ifndef AMB82_FLASH_H
#define AMB82_FLASH_H

#include "config.h"
#include <FlashMemory.h>

// ===== FLASH OPERATION RESULTS =====
typedef enum {
    FLASH_SUCCESS = 0,
    FLASH_ERROR_INIT,
    FLASH_ERROR_READ,
    FLASH_ERROR_WRITE,
    FLASH_ERROR_VERIFY,
    FLASH_ERROR_CHECKSUM,
    FLASH_ERROR_VERSION
} flash_result_t;

// ===== FLASH INITIALIZATION =====
flash_result_t flash_init();
bool flash_is_initialized();

// ===== CONFIGURATION MANAGEMENT =====
flash_result_t config_save_to_flash();
flash_result_t config_load_from_flash();
flash_result_t config_reset_to_defaults();
uint32_t config_calculate_checksum(system_config_t* config);
bool config_validate_checksum(system_config_t* config);

// ===== DETECTION LOG MANAGEMENT =====
flash_result_t flash_write_detection_log(detection_result_t* result);
flash_result_t flash_read_detection_log(uint32_t index, detection_result_t* result);
uint32_t flash_get_log_count();
flash_result_t flash_clear_logs();
flash_result_t flash_get_log_stats(uint32_t* total_count, uint32_t* led_count, uint32_t* motherboard_count);

// ===== UTILITY FUNCTIONS =====
void flash_print_config();
void flash_print_logs(uint32_t max_entries);
flash_result_t flash_test_operations();
const char* flash_result_to_string(flash_result_t result);

// ===== FLASH MEMORY ORGANIZATION =====
// Base address: FLASH_MEMORY_APP_BASE
// Config area:  FLASH_CONFIG_OFFSET (0x1E00) - stores system_config_t
// Log area:     FLASH_LOG_OFFSET (0x1F00) - stores detection logs
// Each log entry is sizeof(detection_result_t) bytes
// Maximum log entries: calculated based on available space

#define MAX_LOG_ENTRIES ((FLASH_SIZE - (FLASH_LOG_OFFSET - FLASH_CONFIG_OFFSET) - sizeof(system_config_t)) / sizeof(detection_result_t))

#endif // AMB82_FLASH_H