// lora_rak3172.h - RAK3172 LoRa Module Communications
#ifndef LORA_RAK3172_H
#define LORA_RAK3172_H

#include "config.h"

// ===== LORA OPERATION RESULTS =====
typedef enum {
    LORA_SUCCESS = 0,
    LORA_ERROR_INIT,
    LORA_ERROR_TIMEOUT,
    LORA_ERROR_SEND,
    LORA_ERROR_RECEIVE,
    LORA_ERROR_AT_COMMAND,
    LORA_ERROR_NETWORK,
    LORA_ERROR_BUFFER_FULL
} lora_result_t;

// ===== LORA STATES =====
typedef enum {
    LORA_STATE_DISCONNECTED = 0,
    LORA_STATE_INITIALIZING,
    LORA_STATE_CONNECTED,
    LORA_STATE_SENDING,
    LORA_STATE_ERROR
} lora_state_t;

// ===== LORA MESSAGE STRUCTURE =====
typedef struct {
    lora_message_type_t type;
    uint32_t timestamp;
    uint8_t payload_length;
    char payload[200];
    uint8_t retry_count;
    bool pending;
} lora_message_t;

// ===== LORA STATISTICS =====
typedef struct {
    uint32_t messages_sent;
    uint32_t messages_failed;
    uint32_t messages_received;
    uint32_t last_send_timestamp;
    uint32_t last_receive_timestamp;
    uint32_t total_send_attempts;
    uint32_t connection_failures;
} lora_stats_t;

// ===== LORA MODULE STATE =====
typedef struct {
    lora_state_t state;
    uint32_t last_send_time;
    uint32_t last_heartbeat;
    lora_message_t pending_message;
    lora_stats_t stats;
    bool initialization_complete;
    char response_buffer[256];
    uint8_t response_index;
    uint32_t command_timeout;
} lora_module_t;

// ===== LORA INITIALIZATION =====
lora_result_t lora_init();
lora_result_t lora_reset();
bool lora_is_initialized();
lora_state_t lora_get_state();

// ===== LORA CONFIGURATION =====
lora_result_t lora_configure_network();
lora_result_t lora_join_network();

// ===== LORA COMMUNICATION =====
lora_result_t lora_send_message(lora_message_type_t type, const char* payload);
lora_result_t lora_send_detection_data(detection_result_t* result);
lora_result_t lora_send_status_update();
lora_result_t lora_send_heartbeat();
lora_result_t lora_send_alert(const char* alert_message);

// ===== LORA PROCESSING =====
void lora_process();
lora_result_t lora_process_pending_messages();
void lora_handle_received_data();
bool lora_should_send_heartbeat();

// ===== LORA AT COMMANDS =====
lora_result_t lora_send_at_command(const char* command, char* response, uint32_t timeout);
void lora_clear_response_buffer();

// ===== LORA COMMAND PROCESSING =====
lora_result_t lora_process_downlink_command(const char* command);
bool lora_parse_reset_command(const char* data);
void lora_execute_reset_command();
void lora_print_stats();
void lora_print_status();
void lora_run_diagnostics();
const char* lora_state_to_string(lora_state_t state);
const char* lora_result_to_string(lora_result_t result);
void lora_set_send_interval(uint32_t interval_ms);
uint32_t lora_get_send_interval();

// ===== LORA MESSAGE FORMATTING =====
lora_result_t lora_format_detection_message(detection_result_t* result, char* buffer, size_t buffer_size);
lora_result_t lora_format_status_message(char* buffer, size_t buffer_size);
lora_result_t lora_format_heartbeat_message(char* buffer, size_t buffer_size);

// ===== UTILITY FUNCTIONS =====
String string_to_hex(const char* str);

// ===== GLOBAL LORA INSTANCE =====
extern lora_module_t lora_module;

// ===== LORA CONFIGURATION CONSTANTS =====
#define LORA_MAX_PAYLOAD_SIZE       200
#define LORA_AT_TIMEOUT             5000    // 5 seconds
#define LORA_JOIN_TIMEOUT           30000   // 30 seconds
#define LORA_SEND_TIMEOUT           10000   // 10 seconds
#define LORA_HEARTBEAT_INTERVAL     (5 * 60 * 1000) // 5 minutes
#define LORA_MAX_RETRY_COUNT        3

// ===== LORA FREQUENCY BANDS =====
#define LORA_BAND_EU868             4
#define LORA_BAND_US915             5
#define LORA_BAND_AU915             6
#define LORA_BAND_AS923             7

// ===== LORA NETWORK MODES =====
#define LORA_MODE_P2P               0
#define LORA_MODE_LORAWAN           1

#endif // LORA_RAK3172_H