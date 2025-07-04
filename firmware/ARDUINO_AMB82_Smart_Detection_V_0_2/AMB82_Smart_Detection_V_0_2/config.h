// config.h - System Configuration for AMB82 Smart Detection V2.0
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===== SYSTEM VERSION =====
#define SYSTEM_VERSION "2.0.0"
#define CONFIG_VERSION 2

// ===== PIN DEFINITIONS =====
#define PIN_FAN                10
#define PIN_CROSSHAIR_LASER    9
#define PIN_RESET_CONTROL      7
#define PIN_STATUS_LED         LED_BUILTIN

// ===== TIMING CONSTANTS =====
#define FAN_CYCLE_INTERVAL     (3 * 60 * 1000)  // 3 minutes
#define LORA_DEFAULT_INTERVAL  (30 * 1000)      // 30 seconds
#define DETECTION_INTERVAL     100              // 100ms
#define LASER_BLINK_INTERVAL   500              // 500ms
#define SERIAL_TIMEOUT         50               // 50ms

// ===== DETECTION THRESHOLDS =====
#define DEFAULT_DETECTION_THRESHOLD     0.7f    // 70%
#define DEFAULT_MOTHERBOARD_THRESHOLD   0.6f    // 60%
#define MAX_DETECTION_RESULTS          10

// ===== MOTHERBOARD COUNTER SETTINGS =====
#define DEFAULT_MOTHERBOARD_COUNT_THRESHOLD    50       // 50 detections
#define DEFAULT_MOTHERBOARD_COUNT_WINDOW       10000    // 10 seconds
#define DEFAULT_MOTHERBOARD_COUNT_ENABLED      1        // Enabled
#define MOTHERBOARD_DETECTION_BUFFER_SIZE      100      // Buffer size

// ===== LORA SETTINGS =====
#define LORA_BAUD_RATE         115200
#define LORA_RETRY_COUNT       3
#define LORA_TIMEOUT           5000

// ===== FLASH MEMORY LAYOUT =====
#define FLASH_CONFIG_OFFSET    0x1E00
#define FLASH_LOG_OFFSET       0x1F00
#define FLASH_SIZE             0x1000

// ===== DETECTION CLASSES =====
#define CLASS_LED_ON           0
#define CLASS_MOTHERBOARD      1
#define CLASS_UNKNOWN          255

// ===== MOTHERBOARD DETECTION COUNTER =====
typedef struct {
    uint32_t detection_timestamps[MOTHERBOARD_DETECTION_BUFFER_SIZE];
    uint8_t buffer_index;
    uint8_t buffer_count;
    uint32_t total_motherboard_detections;
    uint32_t lora_triggers_sent;
    uint32_t last_lora_trigger_time;
    bool enabled;
    uint32_t count_threshold;
    uint32_t time_window_ms;
} motherboard_counter_t;

// ===== SYSTEM CONFIGURATION =====
typedef struct {
    // System Settings
    uint16_t config_version;
    uint32_t system_id;
    
    // LoRa Settings
    uint32_t lora_send_interval;
    uint8_t lora_retry_count;
    uint32_t lora_timeout;
    uint8_t lora_enabled;
    
    // Detection Settings
    float detection_threshold;
    float motherboard_threshold;
    uint8_t detection_enabled;
    uint8_t crosshair_enabled;
    
    // Motherboard Counter Settings
    uint8_t motherboard_count_enabled;
    uint32_t motherboard_count_threshold;
    uint32_t motherboard_count_window_ms;
    
    // GPIO Settings
    uint32_t fan_cycle_interval;
    uint8_t fan_enabled;
    uint32_t laser_blink_interval;
    
    // Debug Settings
    uint8_t debug_level;
    uint8_t serial_commands_enabled;
    
    // Statistics
    uint32_t total_detections;
    uint32_t system_uptime;
    uint32_t total_motherboard_count_triggers;
    uint32_t last_motherboard_trigger_time;
    
    // Validation
    uint32_t checksum;
} system_config_t;

// ===== DETECTION RESULT =====
typedef struct {
    uint32_t timestamp;
    uint8_t object_class;
    float confidence;
    float x_min, y_min, x_max, y_max;
    uint8_t valid;
} detection_result_t;

// ===== LORA MESSAGE TYPES =====
typedef enum {
    LORA_MSG_STATUS = 0,
    LORA_MSG_DETECTION,
    LORA_MSG_ALERT,
    LORA_MSG_CONFIG,
    LORA_MSG_HEARTBEAT,
    LORA_MSG_MOTHERBOARD_TRIGGER
} lora_message_type_t;

// ===== SYSTEM STATES =====
typedef enum {
    SYS_STATE_INIT = 0,
    SYS_STATE_RUNNING,
    SYS_STATE_ERROR,
    SYS_STATE_MAINTENANCE
} system_state_t;

// ===== GLOBAL CONFIGURATION =====
extern system_config_t system_config;
extern system_state_t system_state;

// ===== UTILITY MACROS =====
#define DEBUG_PRINT(level, ...) do { \
    if (system_config.debug_level >= level) { \
        Serial.print("[DEBUG] "); \
        Serial.println(__VA_ARGS__); \
    } \
} while(0)

#define ERROR_PRINT(...) do { \
    Serial.print("[ERROR] "); \
    Serial.println(__VA_ARGS__); \
} while(0)

#define INFO_PRINT(...) do { \
    if (system_config.debug_level >= 2) { \
        Serial.print("[INFO] "); \
        Serial.println(__VA_ARGS__); \
    } \
} while(0)

// ===== DEFAULT CONFIGURATION =====
#define DEFAULT_CONFIG { \
    .config_version = CONFIG_VERSION, \
    .system_id = 0x12345678, \
    .lora_send_interval = LORA_DEFAULT_INTERVAL, \
    .lora_retry_count = LORA_RETRY_COUNT, \
    .lora_timeout = LORA_TIMEOUT, \
    .lora_enabled = 1, \
    .detection_threshold = DEFAULT_DETECTION_THRESHOLD, \
    .motherboard_threshold = DEFAULT_MOTHERBOARD_THRESHOLD, \
    .detection_enabled = 1, \
    .crosshair_enabled = 1, \
    .motherboard_count_enabled = DEFAULT_MOTHERBOARD_COUNT_ENABLED, \
    .motherboard_count_threshold = DEFAULT_MOTHERBOARD_COUNT_THRESHOLD, \
    .motherboard_count_window_ms = DEFAULT_MOTHERBOARD_COUNT_WINDOW, \
    .fan_cycle_interval = FAN_CYCLE_INTERVAL, \
    .fan_enabled = 1, \
    .laser_blink_interval = LASER_BLINK_INTERVAL, \
    .debug_level = 2, \
    .serial_commands_enabled = 1, \
    .total_detections = 0, \
    .system_uptime = 0, \
    .total_motherboard_count_triggers = 0, \
    .last_motherboard_trigger_time = 0, \
    .checksum = 0 \
}

#endif // CONFIG_H