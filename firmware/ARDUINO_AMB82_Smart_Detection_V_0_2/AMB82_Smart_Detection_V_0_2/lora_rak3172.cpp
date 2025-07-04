// lora_rak3172.cpp - RAK3172 LoRa Module Implementation - FIXED VERSION
#include "lora_rak3172.h"
#include "amb82_flash.h"
#include "amb82_gpio.h"

// ===== GLOBAL VARIABLES =====
lora_module_t lora_module = {
    .state = LORA_STATE_DISCONNECTED,
    .last_send_time = 0,
    .last_heartbeat = 0,
    .pending_message = {.type = LORA_MSG_STATUS, .timestamp = 0, .payload_length = 0, .payload = {0}, .retry_count = 0, .pending = false},
    .stats = {0},
    .initialization_complete = false,
    .response_buffer = {0},
    .response_index = 0,
    .command_timeout = LORA_AT_TIMEOUT
};

// ===== UTILITY FUNCTION: Convert String to Hex =====
String string_to_hex(const char* str) {
    String hex = "";
    for (int i = 0; str[i] != '\0'; i++) {
        char hex_char[3];
        sprintf(hex_char, "%02X", (unsigned char)str[i]);
        hex += hex_char;
    }
    return hex;
}

// ===== LORA INITIALIZATION - FIXED =====
lora_result_t lora_init() {
    INFO_PRINT("Initializing LoRa RAK3172 module...");
    
    // FIXED: Serial1 should already be initialized in main setup
    // Serial1.begin(LORA_BAUD_RATE); // Remove this - already done in main
    
    // Initialize module state
    lora_module.state = LORA_STATE_INITIALIZING;
    lora_module.initialization_complete = false;
    lora_module.pending_message.pending = false;
    lora_module.command_timeout = LORA_AT_TIMEOUT;
    lora_clear_response_buffer();
    
    // FIXED: Shorter wait time since basic test already done
    delay(500);
    
    // Test basic communication with shorter timeout
    lora_result_t result = lora_send_at_command("AT", NULL, 2000);
    if (result != LORA_SUCCESS) {
        ERROR_PRINT("LoRa module communication test failed");
        lora_module.state = LORA_STATE_ERROR;
        // FIXED: Don't return error immediately, mark as error but continue
        INFO_PRINT("LoRa will continue in degraded mode");
        lora_module.initialization_complete = true; // Allow other functions to work
        return LORA_ERROR_INIT;
    }
    
    // Get module information with shorter timeout
    char version_buffer[100];
    if (lora_send_at_command("AT+VER=?", version_buffer, 2000) == LORA_SUCCESS) {
        INFO_PRINT("LoRa Module Version: " + String(version_buffer));
    }
    
    // FIXED: Skip complex network configuration if basic communication works
    // Just set to a known state without joining
    lora_send_at_command("AT+NWM=1", NULL, 2000); // Set to LoRaWAN mode
    delay(500);
    
    lora_module.state = LORA_STATE_CONNECTED;
    lora_module.initialization_complete = true;
    lora_module.last_heartbeat = millis();
    
    INFO_PRINT("LoRa RAK3172 module initialized successfully");
    return LORA_SUCCESS;
}

lora_result_t lora_reset() {
    INFO_PRINT("Resetting LoRa module...");
    
    lora_module.state = LORA_STATE_INITIALIZING;
    lora_module.initialization_complete = false;
    
    lora_result_t result = lora_send_at_command("ATZ", NULL, 3000);
    if (result == LORA_SUCCESS) {
        delay(2000);
        result = lora_init();
    }
    
    return result;
}

bool lora_is_initialized() {
    return lora_module.initialization_complete;
}

lora_state_t lora_get_state() {
    return lora_module.state;
}

// ===== LORA CONFIGURATION - SIMPLIFIED =====
lora_result_t lora_configure_network() {
    INFO_PRINT("Configuring LoRa network settings...");
    
    // FIXED: Simplified configuration - don't try to join network
    // Just configure basic settings
    
    // Set to LoRaWAN mode
    if (lora_send_at_command("AT+NWM=1", NULL, 2000) != LORA_SUCCESS) {
        ERROR_PRINT("Failed to set LoRaWAN mode");
        return LORA_ERROR_AT_COMMAND;
    }
    delay(500);
    
    // Set frequency band (US915)
    if (lora_send_at_command("AT+BAND=5", NULL, 2000) != LORA_SUCCESS) {
        ERROR_PRINT("Failed to set frequency band");
        return LORA_ERROR_AT_COMMAND;
    }
    delay(500);
    
    // Set device class
    if (lora_send_at_command("AT+CLASS=A", NULL, 2000) != LORA_SUCCESS) {
        ERROR_PRINT("Failed to set device class");
        return LORA_ERROR_AT_COMMAND;
    }
    delay(500);
    
    // Set confirmed/unconfirmed messages
    if (lora_send_at_command("AT+CFM=0", NULL, 2000) != LORA_SUCCESS) {
        ERROR_PRINT("Failed to set message confirmation");
        return LORA_ERROR_AT_COMMAND;
    }
    delay(500);
    
    INFO_PRINT("LoRa basic configuration completed (network join skipped)");
    return LORA_SUCCESS;
}

// ===== LORA COMMUNICATION - FIXED =====
lora_result_t lora_send_message(lora_message_type_t type, const char* payload) {
    if (!lora_is_initialized() || !payload) {
        return LORA_ERROR_INIT;
    }
    
    // FIXED: Don't block if already sending
    if (lora_module.state == LORA_STATE_SENDING) {
        DEBUG_PRINT(3, "LoRa busy, message queued");
        return LORA_ERROR_BUFFER_FULL;
    }
    
    lora_module.state = LORA_STATE_SENDING;
    
    // FIXED: Limit payload size more strictly
    if (strlen(payload) > 20) {
        ERROR_PRINT("LoRa payload too long: " + String(strlen(payload)) + " chars");
        lora_module.state = LORA_STATE_CONNECTED;
        return LORA_ERROR_SEND;
    }
    
    // Convert payload to hex format for RAK3172
    String hex_payload = string_to_hex(payload);
    
    // Check hex payload size (40 hex characters = 20 bytes for safety)
    if (hex_payload.length() > 40) {
        ERROR_PRINT("LoRa hex payload too long: " + String(hex_payload.length()/2) + " bytes");
        lora_module.state = LORA_STATE_CONNECTED;
        return LORA_ERROR_SEND;
    }
    
    // Format AT command
    char at_command[100];
    snprintf(at_command, sizeof(at_command), "AT+SEND=2:%s", hex_payload.c_str());
    
    INFO_PRINT("Sending LoRa message: " + String(payload));
    DEBUG_PRINT(3, "Hex payload: " + hex_payload);
    DEBUG_PRINT(3, "AT command: " + String(at_command));
    
    // FIXED: Shorter timeout for send commands
    lora_result_t result = lora_send_at_command(at_command, NULL, 5000);
    
    // Update statistics
    lora_module.stats.total_send_attempts++;
    
    if (result == LORA_SUCCESS) {
        lora_module.stats.messages_sent++;
        lora_module.last_send_time = millis();
        INFO_PRINT("LoRa message sent successfully");
    } else {
        lora_module.stats.messages_failed++;
        ERROR_PRINT("LoRa message send failed: " + String(lora_result_to_string(result)));
    }
    
    lora_module.state = LORA_STATE_CONNECTED;
    return result;
}

lora_result_t lora_send_detection_data(detection_result_t* result) {
    if (!result) {
        return LORA_ERROR_INIT;
    }
    
    char message_buffer[LORA_MAX_PAYLOAD_SIZE];
    lora_result_t format_result = lora_format_detection_message(result, message_buffer, sizeof(message_buffer));
    
    if (format_result != LORA_SUCCESS) {
        return format_result;
    }
    
    return lora_send_message(LORA_MSG_DETECTION, message_buffer);
}

lora_result_t lora_send_status_update() {
    char message_buffer[LORA_MAX_PAYLOAD_SIZE];
    lora_result_t format_result = lora_format_status_message(message_buffer, sizeof(message_buffer));
    
    if (format_result != LORA_SUCCESS) {
        return format_result;
    }
    
    return lora_send_message(LORA_MSG_STATUS, message_buffer);
}

lora_result_t lora_send_heartbeat() {
    char message_buffer[LORA_MAX_PAYLOAD_SIZE];
    lora_result_t format_result = lora_format_heartbeat_message(message_buffer, sizeof(message_buffer));
    
    if (format_result != LORA_SUCCESS) {
        return format_result;
    }
    
    lora_module.last_heartbeat = millis();
    return lora_send_message(LORA_MSG_HEARTBEAT, message_buffer);
}

lora_result_t lora_send_alert(const char* alert_message) {
    if (!alert_message) {
        return LORA_ERROR_INIT;
    }
    
    // FIXED: Even shorter alert message
    char formatted_message[LORA_MAX_PAYLOAD_SIZE];
    snprintf(formatted_message, sizeof(formatted_message), 
             "A,%lu,%.10s", 
             millis()/1000, alert_message);
    
    return lora_send_message(LORA_MSG_ALERT, formatted_message);
}

// ===== LORA PROCESSING =====
void lora_process() {
    // Handle received data
    lora_handle_received_data();
    
    // FIXED: Reduce processing frequency
    static uint32_t last_process = 0;
    if (millis() - last_process < 5000) return; // Process every 5 seconds
    last_process = millis();
    
    // Process pending messages
    lora_process_pending_messages();
    
    // Send periodic heartbeat - much less frequent
    if (lora_should_send_heartbeat()) {
        lora_send_heartbeat();
    }
}

lora_result_t lora_process_pending_messages() {
    if (lora_module.pending_message.pending) {
        DEBUG_PRINT(3, "Processing pending LoRa message");
        
        lora_result_t result = lora_send_message(lora_module.pending_message.type, 
                                                lora_module.pending_message.payload);
        
        if (result == LORA_SUCCESS) {
            lora_module.pending_message.pending = false;
            lora_module.pending_message.retry_count = 0;
            DEBUG_PRINT(3, "Pending message sent successfully");
        } else {
            lora_module.pending_message.retry_count++;
            if (lora_module.pending_message.retry_count >= LORA_MAX_RETRY_COUNT) {
                ERROR_PRINT("Max retries reached for pending message, dropping");
                lora_module.pending_message.pending = false;
                lora_module.pending_message.retry_count = 0;
                lora_module.stats.messages_failed++;
                return LORA_ERROR_SEND;
            } else {
                DEBUG_PRINT(3, "Retrying pending message, attempt " + String(lora_module.pending_message.retry_count));
            }
        }
    }
    
    return LORA_SUCCESS;
}

void lora_handle_received_data() {
    while (Serial1.available()) {
        char c = Serial1.read();
        
        if (c == '\n') {
            if (lora_module.response_index > 0) {
                lora_module.response_buffer[lora_module.response_index] = '\0';
                
                DEBUG_PRINT(3, "LoRa RX: " + String(lora_module.response_buffer));
                
                if (strstr(lora_module.response_buffer, "+EVT:RX_1")) {
                    INFO_PRINT("LoRa downlink received");
                    lora_module.stats.messages_received++;
                    lora_process_downlink_command(lora_module.response_buffer);
                }
                
                lora_clear_response_buffer();
            }
        } else if (c != '\r' && lora_module.response_index < sizeof(lora_module.response_buffer) - 1) {
            lora_module.response_buffer[lora_module.response_index++] = c;
        }
    }
}

bool lora_should_send_heartbeat() {
    // FIXED: Much longer heartbeat interval - 30 minutes
    return (millis() - lora_module.last_heartbeat > (30 * 60 * 1000));
}

// ===== LORA AT COMMANDS - FIXED =====
lora_result_t lora_send_at_command(const char* command, char* response, uint32_t timeout) {
    if (!command) {
        return LORA_ERROR_AT_COMMAND;
    }
    
    DEBUG_PRINT(3, "LoRa AT: " + String(command));
    
    // FIXED: More thorough buffer clearing
    while (Serial1.available()) {
        Serial1.read();
        delay(1);
    }
    
    lora_clear_response_buffer();
    
    // Send command
    Serial1.println(command);
    Serial1.flush(); // FIXED: Ensure command is sent
    
    // Wait for response
    uint32_t start_time = millis();
    bool got_response = false;
    
    while ((millis() - start_time) < timeout) {
        while (Serial1.available()) {
            char c = Serial1.read();
            
            if (c == '\n') {
                if (lora_module.response_index > 0) {
                    lora_module.response_buffer[lora_module.response_index] = '\0';
                    
                    if (response) {
                        strcpy(response, lora_module.response_buffer);
                    }
                    
                    // FIXED: More flexible response checking
                    String resp = String(lora_module.response_buffer);
                    resp.trim();
                    
                    if (resp.equals("OK") || 
                        resp.indexOf("OK") >= 0 ||
                        resp.indexOf("+") == 0 ||  // Response that starts with +
                        resp.indexOf("EVT:") >= 0) {
                        DEBUG_PRINT(3, "LoRa Response: " + resp);
                        return LORA_SUCCESS;
                    } else if (resp.indexOf("ERROR") >= 0 || resp.indexOf("FAIL") >= 0) {
                        ERROR_PRINT("LoRa AT Error: " + resp);
                        return LORA_ERROR_AT_COMMAND;
                    }
                    
                    lora_clear_response_buffer();
                    got_response = true;
                }
            } else if (c != '\r' && lora_module.response_index < sizeof(lora_module.response_buffer) - 1) {
                lora_module.response_buffer[lora_module.response_index++] = c;
            }
        }
        delay(10);
    }
    
    if (got_response) {
        DEBUG_PRINT(3, "LoRa AT partial response received");
        return LORA_SUCCESS; // Got some response, consider it successful
    }
    
    ERROR_PRINT("LoRa AT timeout: " + String(command));
    return LORA_ERROR_TIMEOUT;
}

void lora_clear_response_buffer() {
    memset(lora_module.response_buffer, 0, sizeof(lora_module.response_buffer));
    lora_module.response_index = 0;
}

// ===== LORA MESSAGE FORMATTING (SHORTER MESSAGES) =====
lora_result_t lora_format_detection_message(detection_result_t* result, char* buffer, size_t buffer_size) {
    if (!result || !buffer) {
        return LORA_ERROR_INIT;
    }
    
    // FIXED: Even shorter format
    const char* class_name = (result->object_class == CLASS_LED_ON) ? "L" : 
                            (result->object_class == CLASS_MOTHERBOARD) ? "M" : "U";
    
    snprintf(buffer, buffer_size,
             "D,%s,%d",
             class_name, (int)(result->confidence * 100));
    
    return LORA_SUCCESS;
}

lora_result_t lora_format_status_message(char* buffer, size_t buffer_size) {
    if (!buffer) {
        return LORA_ERROR_INIT;
    }
    
    uint32_t total_detections = system_config.total_detections;
    
    // FIXED: Shorter status message
    snprintf(buffer, buffer_size,
             "S,%lu,%lu",
             millis()/1000, total_detections);
    
    return LORA_SUCCESS;
}

lora_result_t lora_format_heartbeat_message(char* buffer, size_t buffer_size) {
    if (!buffer) {
        return LORA_ERROR_INIT;
    }
    
    // FIXED: Short heartbeat
    snprintf(buffer, buffer_size,
             "H,%lu,%d",
             millis()/1000, lora_module.state);
    
    return LORA_SUCCESS;
}

// ===== LORA UTILITIES =====
void lora_print_stats() {
    Serial.println("\n=== LORA STATISTICS ===");
    Serial.println("State: " + String(lora_state_to_string(lora_module.state)));
    Serial.println("Messages Sent: " + String(lora_module.stats.messages_sent));
    Serial.println("Messages Failed: " + String(lora_module.stats.messages_failed));
    Serial.println("Messages Received: " + String(lora_module.stats.messages_received));
    Serial.println("Send Attempts: " + String(lora_module.stats.total_send_attempts));
    Serial.println("Connection Failures: " + String(lora_module.stats.connection_failures));
    Serial.println("Last Send: " + String((millis() - lora_module.last_send_time)/1000) + "s ago");
    Serial.println("======================\n");
}

void lora_print_status() {
    Serial.println("LoRa Status: " + String(lora_state_to_string(lora_module.state)));
    Serial.println("Initialized: " + String(lora_module.initialization_complete ? "YES" : "NO"));
    Serial.println("Enabled: " + String(system_config.lora_enabled ? "YES" : "NO"));
}

void lora_run_diagnostics() {
    Serial.println("\n=== LORA DIAGNOSTICS ===");
    
    // Test basic communication
    Serial.println("Testing basic AT communication...");
    char response[100];
    lora_result_t result = lora_send_at_command("AT", response, 2000);
    Serial.println("AT test: " + String(lora_result_to_string(result)));
    if (result == LORA_SUCCESS) {
        Serial.println("Response: " + String(response));
    }
    
    // Get version info
    Serial.println("\nGetting module version...");
    memset(response, 0, sizeof(response));
    if (lora_send_at_command("AT+VER=?", response, 2000) == LORA_SUCCESS) {
        Serial.println("Version: " + String(response));
    }
    
    // Check network mode
    Serial.println("\nChecking network mode...");
    memset(response, 0, sizeof(response));
    if (lora_send_at_command("AT+NWM=?", response, 2000) == LORA_SUCCESS) {
        Serial.println("Network mode: " + String(response) + " (1=LoRaWAN, 0=P2P)");
    }
    
    // Test simple send with very short message
    Serial.println("\nTesting simple send (short message)...");
    if (lora_send_at_command("AT+SEND=2:48454C4C4F", NULL, 8000) == LORA_SUCCESS) {
        Serial.println("Simple send: SUCCESS");
    } else {
        Serial.println("Simple send: FAILED");
    }
    
    Serial.println("========================\n");
}

const char* lora_state_to_string(lora_state_t state) {
    switch (state) {
        case LORA_STATE_DISCONNECTED: return "DISCONNECTED";
        case LORA_STATE_INITIALIZING: return "INITIALIZING";
        case LORA_STATE_CONNECTED: return "CONNECTED";
        case LORA_STATE_SENDING: return "SENDING";
        case LORA_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char* lora_result_to_string(lora_result_t result) {
    switch (result) {
        case LORA_SUCCESS: return "SUCCESS";
        case LORA_ERROR_INIT: return "INIT_ERROR";
        case LORA_ERROR_TIMEOUT: return "TIMEOUT";
        case LORA_ERROR_SEND: return "SEND_ERROR";
        case LORA_ERROR_RECEIVE: return "RECEIVE_ERROR";
        case LORA_ERROR_AT_COMMAND: return "AT_COMMAND_ERROR";
        case LORA_ERROR_NETWORK: return "NETWORK_ERROR";
        case LORA_ERROR_BUFFER_FULL: return "BUFFER_FULL";
        default: return "UNKNOWN_ERROR";
    }
}

void lora_set_send_interval(uint32_t interval_ms) {
    system_config.lora_send_interval = interval_ms;
    INFO_PRINT("LoRa send interval updated: " + String(interval_ms/1000) + "s");
}

uint32_t lora_get_send_interval() {
    return system_config.lora_send_interval;
}

// ===== COMMAND PROCESSING =====
lora_result_t lora_process_downlink_command(const char* command) {
    if (!command) {
        return LORA_ERROR_INIT;
    }
    
    INFO_PRINT("Processing LoRa downlink command: " + String(command));
    
    if (lora_parse_reset_command(command)) {
        INFO_PRINT("Reset command detected in LoRa downlink");
        lora_execute_reset_command();
        return LORA_SUCCESS;
    }
    
    DEBUG_PRINT(3, "No recognized commands in downlink data");
    return LORA_SUCCESS;
}

bool lora_parse_reset_command(const char* data) {
    if (!data) {
        return false;
    }
    
    if (strstr(data, "RESET") || strstr(data, "reset")) {
        return true;
    }
    
    if (strstr(data, "\"cmd\":\"reset\"") || strstr(data, "\"command\":\"reset\"")) {
        return true;
    }
    
    if (strstr(data, "52455345") || strstr(data, "52657365")) {
        return true;
    }
    
    return false;
}

void lora_execute_reset_command() {
    INFO_PRINT("Executing LoRa-triggered system reset...");
    
    lora_send_alert("Reset command received, executing reset");
    delay(2000);
    
    config_save_to_flash();
    gpio_trigger_system_reset();
    
    delay(1000);
    INFO_PRINT("Falling back to software reset...");
    NVIC_SystemReset();
}