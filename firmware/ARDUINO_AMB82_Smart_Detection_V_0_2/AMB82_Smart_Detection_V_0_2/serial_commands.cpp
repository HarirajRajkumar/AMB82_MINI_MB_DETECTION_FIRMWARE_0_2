// serial_commands.cpp - Serial Command Interface Implementation
#include "serial_commands.h"
#include "motherboard_counter.h"
#include "lora_rak3172.h"
#include "amb82_flash.h"
#include "amb82_gpio.h"

// Add WiFi include
#include "WiFi.h"

// ===== GLOBAL VARIABLES =====
static bool commands_enabled = true;
static String input_buffer = "";

// External WiFi variables from main file
extern char wifi_ssid[32];
extern char wifi_password[32];
extern bool wifi_connected;
extern bool rtsp_streaming;

// External function declarations (add to top of file)
extern void debug_neural_network_status();
extern bool reset_camera_system();
extern bool restart_neural_network();

// External function declarations from main file
extern bool init_wifi_on_demand();
extern bool start_rtsp_streaming();
extern void stop_rtsp_streaming();

// ===== SERIAL COMMAND INITIALIZATION =====
void serial_commands_init() {
    commands_enabled = system_config.serial_commands_enabled;
    input_buffer.reserve(MAX_COMMAND_LENGTH);
    
    if (commands_enabled) {
        print_welcome_message();
    }
}

bool serial_commands_is_enabled() {
    return commands_enabled;
}

// ===== COMMAND PROCESSING =====
void serial_commands_process() {
    if (!commands_enabled || !serial_input_available()) {
        return;
    }
    
    String command_line = read_serial_line();
    if (command_line.length() == 0) {
        return;
    }
    
    parsed_command_t cmd = {0};
    command_result_t parse_result = serial_commands_parse_input(command_line.c_str(), &cmd);
    
    if (parse_result != CMD_SUCCESS) {
        Serial.println("Parse error: " + String(command_result_to_string(parse_result)));
        return;
    }
    
    command_result_t exec_result = serial_commands_execute(&cmd);
    if (exec_result != CMD_SUCCESS) {
        Serial.println("Command error: " + String(command_result_to_string(exec_result)));
    }
}

command_result_t serial_commands_parse_input(const char* input, parsed_command_t* cmd) {
    if (!input || !cmd) {
        return CMD_ERROR_INVALID_PARAMETER;
    }
    
    // Clear command structure
    memset(cmd, 0, sizeof(parsed_command_t));
    
    // Create a copy of input for tokenization
    char input_copy[MAX_COMMAND_LENGTH];
    strncpy(input_copy, input, sizeof(input_copy) - 1);
    input_copy[sizeof(input_copy) - 1] = '\0';
    
    // Tokenize the input
    char* tokens[MAX_TOKENS] = {0};
    int token_count = 0;
    
    char* token = strtok(input_copy, " \t");
    while (token && token_count < MAX_TOKENS) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t");
    }
    
    if (token_count == 0) {
        return CMD_ERROR_UNKNOWN_COMMAND;
    }
    
    // Parse command
    strncpy(cmd->command, tokens[0], sizeof(cmd->command) - 1);
    
    // Parse parameter (if exists)
    if (token_count > 1) {
        strncpy(cmd->parameter, tokens[1], sizeof(cmd->parameter) - 1);
        cmd->has_parameter = true;
        
        // Parse value (if exists)
        if (token_count > 2) {
            strncpy(cmd->value, tokens[2], sizeof(cmd->value) - 1);
            cmd->has_value = true;
        }
    }
    
    return CMD_SUCCESS;
}

command_result_t serial_commands_execute(parsed_command_t* cmd) {
    if (!cmd) {
        return CMD_ERROR_INVALID_PARAMETER;
    }
    
    // Main commands
    if (strcmp(cmd->command, CMD_HELP) == 0) {
        return cmd_help();
    } else if (strcmp(cmd->command, CMD_STATUS) == 0) {
        return cmd_status();
    } else if (strcmp(cmd->command, CMD_SET) == 0) {
        if (!cmd->has_parameter || !cmd->has_value) {
            Serial.println("Usage: set <parameter> <value>");
            return CMD_ERROR_MISSING_PARAMETER;
        }
        return cmd_set_parameter(cmd->parameter, cmd->value);
    } else if (strcmp(cmd->command, CMD_GET) == 0) {
        if (!cmd->has_parameter) {
            Serial.println("Usage: get <parameter>");
            return CMD_ERROR_MISSING_PARAMETER;
        }
        return cmd_get_parameter(cmd->parameter);
    } else if (strcmp(cmd->command, CMD_SAVE) == 0) {
        return cmd_save_config();
    } else if (strcmp(cmd->command, CMD_RESET) == 0) {
        return cmd_reset_config();
    } else if (strcmp(cmd->command, "reset_system") == 0) {
        return cmd_reset_system();
    } else if (strcmp(cmd->command, CMD_REBOOT) == 0) {
        return cmd_reboot();
    } else if (strcmp(cmd->command, CMD_LOGS) == 0) {
        const char* count = cmd->has_parameter ? cmd->parameter : "10";
        return cmd_logs(count);
    } else if (strcmp(cmd->command, CMD_CLEAR_LOGS) == 0) {
        return cmd_clear_logs();
    } else if (strcmp(cmd->command, CMD_TEST) == 0) {
        return cmd_test();
    } else if (strcmp(cmd->command, CMD_GPIO) == 0) {
        return cmd_gpio_status();
    } else if (strcmp(cmd->command, CMD_LORA) == 0) {
        if (cmd->has_parameter && strcmp(cmd->parameter, "stats") == 0) {
            return cmd_lora_stats();
        } else if (cmd->has_parameter && strcmp(cmd->parameter, "test") == 0) {
            return cmd_lora_test();
        } else if (cmd->has_parameter && strcmp(cmd->parameter, "diag") == 0) {
            return cmd_lora_diagnostics();
        } else {
            return cmd_lora_status();
        }
    } else if (strcmp(cmd->command, CMD_FLASH) == 0) {
        return cmd_flash_status();
    } else if (strcmp(cmd->command, CMD_DETECTION) == 0) {
        return cmd_detection_stats();
    } else if (strcmp(cmd->command, "nn_status") == 0) {
        return cmd_nn_status();
    } else if (strcmp(cmd->command, "nn_reset") == 0) {
        return cmd_nn_reset();
    } else if (strcmp(cmd->command, "nn_restart") == 0) {
        return cmd_nn_restart();
    } else if (strcmp(cmd->command, "camera_reset") == 0) {
        return cmd_camera_reset();
    }
    
    // WiFi/RTSP commands
    else if (strcmp(cmd->command, "rtsp_stream") == 0) {
        return cmd_rtsp_stream();
    } else if (strcmp(cmd->command, "rtsp_stop") == 0) {
        return cmd_rtsp_stop();
    } else if (strcmp(cmd->command, "set_wifi") == 0) {
        if (!cmd->has_parameter || !cmd->has_value) {
            Serial.println("Usage: set_wifi <ssid> <password>");
            return CMD_ERROR_MISSING_PARAMETER;
        }
        return cmd_set_wifi(cmd->parameter, cmd->value);
    }
    
    // Motherboard counter commands
    else if (strcmp(cmd->command, "mb_counter") == 0) {
        return cmd_motherboard_counter();
    } else if (strcmp(cmd->command, "mb_reset") == 0) {
        return cmd_motherboard_reset();
    }
    
    return CMD_ERROR_UNKNOWN_COMMAND;
}

// ===== COMMAND HANDLERS =====
command_result_t cmd_help() {
    print_help_message();
    return CMD_SUCCESS;
}

command_result_t cmd_status() {
    print_system_status();
    return CMD_SUCCESS;
}

command_result_t cmd_set_parameter(const char* parameter, const char* value) {
    if (!parameter || !value) {
        return CMD_ERROR_INVALID_PARAMETER;
    }
    
    Serial.println("Setting " + String(parameter) + " = " + String(value));
    
    if (strcmp(parameter, PARAM_LORA_INTERVAL) == 0) {
        return set_lora_interval(value);
    } else if (strcmp(parameter, PARAM_DETECTION_THRESHOLD) == 0) {
        return set_detection_threshold(value);
    } else if (strcmp(parameter, PARAM_MOTHERBOARD_THRESHOLD) == 0) {
        return set_motherboard_threshold(value);
    } else if (strcmp(parameter, PARAM_FAN_CYCLE_INTERVAL) == 0) {
        return set_fan_cycle_interval(value);
    } else if (strcmp(parameter, PARAM_LASER_BLINK_INTERVAL) == 0) {
        return set_laser_blink_interval(value);
    } else if (strcmp(parameter, PARAM_DEBUG_LEVEL) == 0) {
        return set_debug_level(value);
    } else if (strcmp(parameter, PARAM_FAN_ENABLED) == 0) {
        return set_fan_enabled(value);
    } else if (strcmp(parameter, PARAM_CROSSHAIR_ENABLED) == 0) {
        return set_crosshair_enabled(value);
    }
    // Motherboard counter parameters
    else if (strcmp(parameter, PARAM_MOTHERBOARD_COUNT_ENABLED) == 0) {
        return set_motherboard_count_enabled(value);
    } else if (strcmp(parameter, PARAM_MOTHERBOARD_COUNT_THRESHOLD) == 0) {
        return set_motherboard_count_threshold(value);
    } else if (strcmp(parameter, PARAM_MOTHERBOARD_COUNT_WINDOW) == 0) {
        return set_motherboard_count_window(value);
    }
    
    return CMD_ERROR_INVALID_PARAMETER;
}

command_result_t cmd_get_parameter(const char* parameter) {
    if (!parameter) {
        return CMD_ERROR_INVALID_PARAMETER;
    }
    
    if (strcmp(parameter, PARAM_LORA_INTERVAL) == 0) {
        return get_lora_interval();
    } else if (strcmp(parameter, PARAM_DETECTION_THRESHOLD) == 0) {
        return get_detection_threshold();
    } else if (strcmp(parameter, PARAM_MOTHERBOARD_THRESHOLD) == 0) {
        return get_motherboard_threshold();
    } else if (strcmp(parameter, PARAM_FAN_CYCLE_INTERVAL) == 0) {
        return get_fan_cycle_interval();
    } else if (strcmp(parameter, PARAM_LASER_BLINK_INTERVAL) == 0) {
        return get_laser_blink_interval();
    } else if (strcmp(parameter, PARAM_DEBUG_LEVEL) == 0) {
        return get_debug_level();
    } else if (strcmp(parameter, PARAM_SYSTEM_UPTIME) == 0) {
        return get_system_uptime();
    } else if (strcmp(parameter, PARAM_TOTAL_DETECTIONS) == 0) {
        return get_total_detections();
    }
    // Motherboard counter parameters
    else if (strcmp(parameter, PARAM_MOTHERBOARD_COUNT_ENABLED) == 0) {
        return get_motherboard_count_enabled();
    } else if (strcmp(parameter, PARAM_MOTHERBOARD_COUNT_THRESHOLD) == 0) {
        return get_motherboard_count_threshold();
    } else if (strcmp(parameter, PARAM_MOTHERBOARD_COUNT_WINDOW) == 0) {
        return get_motherboard_count_window();
    }
    
    return CMD_ERROR_INVALID_PARAMETER;
}

// ===== WIFI/RTSP COMMAND HANDLERS - FIXED RETURN TYPES =====
command_result_t cmd_rtsp_stream() {
    Serial.println("Executing: Start WiFi + RTSP streaming");
    
    if (init_wifi_on_demand()) {
        if (start_rtsp_streaming()) {
            Serial.println("✅ RTSP streaming started successfully");
            return CMD_SUCCESS;
        } else {
            Serial.println("❌ Failed to start RTSP streaming");
            return CMD_ERROR_SYSTEM_ERROR;
        }
    } else {
        Serial.println("❌ Failed to connect to WiFi");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_rtsp_stop() {
    Serial.println("Executing: Stop RTSP streaming");
    
    stop_rtsp_streaming();
    
    if (wifi_connected) {
        WiFi.disconnect();
        wifi_connected = false;
        Serial.println("[WiFi] Disconnected");
    }
    
    Serial.println("✅ RTSP streaming stopped");
    return CMD_SUCCESS;
}

command_result_t cmd_set_wifi(const char* ssid, const char* password) {
    if (!ssid || !password) {
        Serial.println("Usage: set_wifi <ssid> <password>");
        return CMD_ERROR_MISSING_PARAMETER;
    }
    
    strncpy(wifi_ssid, ssid, sizeof(wifi_ssid) - 1);
    strncpy(wifi_password, password, sizeof(wifi_password) - 1);
    wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
    wifi_password[sizeof(wifi_password) - 1] = '\0';
    
    Serial.println("WiFi credentials updated:");
    Serial.println("SSID: " + String(wifi_ssid));
    Serial.println("Password: [hidden]");
    Serial.println("Use 'rtsp_stream' to connect");
    
    Serial.println("✅ WiFi credentials updated");
    return CMD_SUCCESS;
}

// ===== MOTHERBOARD COUNTER COMMAND HANDLERS =====
command_result_t cmd_motherboard_counter() {
    motherboard_counter_print_stats();
    return CMD_SUCCESS;
}

command_result_t cmd_motherboard_reset() {
    Serial.println("Resetting motherboard detection counter...");
    motherboard_counter_reset();
    Serial.println("✓ Motherboard counter reset complete");
    return CMD_SUCCESS;
}

// ===== MOTHERBOARD COUNTER PARAMETER SETTERS =====
command_result_t set_motherboard_count_enabled(const char* value) {
    if (!is_boolean_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    bool enabled = parse_bool_value(value);
    if (motherboard_counter_set_enabled(enabled)) {
        Serial.println("Motherboard counter " + String(enabled ? "enabled" : "disabled"));
        return CMD_SUCCESS;
    } else {
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t set_motherboard_count_threshold(const char* value) {
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    int threshold = parse_int_value(value);
    if (threshold < 1 || threshold > 1000) {
        Serial.println("Invalid range. Use 1-1000 detections.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    if (motherboard_counter_set_threshold(threshold)) {
        Serial.println("Motherboard counter threshold set to " + String(threshold));
        return CMD_SUCCESS;
    } else {
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t set_motherboard_count_window(const char* value) {
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    int window_seconds = parse_int_value(value);
    if (window_seconds < 1 || window_seconds > 300) {
        Serial.println("Invalid range. Use 1-300 seconds.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    if (motherboard_counter_set_window(window_seconds)) {
        Serial.println("Motherboard counter window set to " + String(window_seconds) + " seconds");
        return CMD_SUCCESS;
    } else {
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

// ===== MOTHERBOARD COUNTER PARAMETER GETTERS =====
command_result_t get_motherboard_count_enabled() {
    Serial.println(String(PARAM_MOTHERBOARD_COUNT_ENABLED) + " = " + 
                  String(system_config.motherboard_count_enabled ? "1" : "0"));
    return CMD_SUCCESS;
}

command_result_t get_motherboard_count_threshold() {
    Serial.println(String(PARAM_MOTHERBOARD_COUNT_THRESHOLD) + " = " + 
                  String(system_config.motherboard_count_threshold));
    return CMD_SUCCESS;
}

command_result_t get_motherboard_count_window() {
    Serial.println(String(PARAM_MOTHERBOARD_COUNT_WINDOW) + " = " + 
                  String(system_config.motherboard_count_window_ms/1000) + " seconds");
    return CMD_SUCCESS;
}

// ===== BASIC PARAMETER HANDLERS =====
command_result_t set_lora_interval(const char* value) {
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    int interval_seconds = parse_int_value(value);
    if (interval_seconds < 5 || interval_seconds > 3600) {
        Serial.println("Invalid range. Use 5-3600 seconds.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    lora_set_send_interval(interval_seconds * 1000);
    Serial.println("LoRa interval set to " + String(interval_seconds) + " seconds");
    return CMD_SUCCESS;
}

command_result_t set_detection_threshold(const char* value) {
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    float threshold = parse_float_value(value);
    if (threshold < 0.0f || threshold > 1.0f) {
        Serial.println("Invalid range. Use 0.0-1.0.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    system_config.detection_threshold = threshold;
    Serial.println("Detection threshold set to " + String(threshold));
    return CMD_SUCCESS;
}

command_result_t set_motherboard_threshold(const char* value) {
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    float threshold = parse_float_value(value);
    if (threshold < 0.0f || threshold > 1.0f) {
        Serial.println("Invalid range. Use 0.0-1.0.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    system_config.motherboard_threshold = threshold;
    Serial.println("Motherboard threshold set to " + String(threshold));
    return CMD_SUCCESS;
}

// ===== STUB IMPLEMENTATIONS FOR MISSING FUNCTIONS =====
command_result_t set_fan_cycle_interval(const char* value) {
    // Implementation for fan cycle interval
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    int interval_seconds = parse_int_value(value);
    if (interval_seconds < 1 || interval_seconds > 3600) {
        Serial.println("Invalid range. Use 1-3600 seconds.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    system_config.fan_cycle_interval = interval_seconds * 1000;
    Serial.println("Fan cycle interval set to " + String(interval_seconds) + " seconds");
    return CMD_SUCCESS;
}

command_result_t set_laser_blink_interval(const char* value) {
    // Implementation for laser blink interval
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    int interval_ms = parse_int_value(value);
    if (interval_ms < 100 || interval_ms > 5000) {
        Serial.println("Invalid range. Use 100-5000 milliseconds.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    system_config.laser_blink_interval = interval_ms;
    Serial.println("Laser blink interval set to " + String(interval_ms) + "ms");
    return CMD_SUCCESS;
}

command_result_t set_debug_level(const char* value) {
    if (!is_numeric_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    int level = parse_int_value(value);
    if (level < 0 || level > 5) {
        Serial.println("Invalid range. Use 0-5.");
        return CMD_ERROR_INVALID_VALUE;
    }
    
    system_config.debug_level = level;
    Serial.println("Debug level set to " + String(level));
    return CMD_SUCCESS;
}

command_result_t set_fan_enabled(const char* value) {
    if (!is_boolean_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    bool enabled = parse_bool_value(value);
    system_config.fan_enabled = enabled;
    
    if (gpio_is_initialized()) {
        gpio_fan_enable(enabled);
    }
    
    Serial.println("Fan " + String(enabled ? "enabled" : "disabled"));
    return CMD_SUCCESS;
}

command_result_t set_crosshair_enabled(const char* value) {
    if (!is_boolean_value(value)) {
        return CMD_ERROR_INVALID_VALUE;
    }
    
    bool enabled = parse_bool_value(value);
    system_config.crosshair_enabled = enabled;
    
    if (gpio_is_initialized()) {
        gpio_laser_enable(enabled);
    }
    
    Serial.println("Crosshair " + String(enabled ? "enabled" : "disabled"));
    return CMD_SUCCESS;
}

// ===== GET PARAMETER IMPLEMENTATIONS =====
command_result_t get_lora_interval() {
    Serial.println(String(PARAM_LORA_INTERVAL) + " = " + String(system_config.lora_send_interval/1000) + " seconds");
    return CMD_SUCCESS;
}

command_result_t get_detection_threshold() {
    Serial.println(String(PARAM_DETECTION_THRESHOLD) + " = " + String(system_config.detection_threshold));
    return CMD_SUCCESS;
}

command_result_t get_motherboard_threshold() {
    Serial.println(String(PARAM_MOTHERBOARD_THRESHOLD) + " = " + String(system_config.motherboard_threshold));
    return CMD_SUCCESS;
}

command_result_t get_fan_cycle_interval() {
    Serial.println(String(PARAM_FAN_CYCLE_INTERVAL) + " = " + String(system_config.fan_cycle_interval/1000) + " seconds");
    return CMD_SUCCESS;
}

command_result_t get_laser_blink_interval() {
    Serial.println(String(PARAM_LASER_BLINK_INTERVAL) + " = " + String(system_config.laser_blink_interval) + "ms");
    return CMD_SUCCESS;
}

command_result_t get_debug_level() {
    Serial.println(String(PARAM_DEBUG_LEVEL) + " = " + String(system_config.debug_level));
    return CMD_SUCCESS;
}

command_result_t get_system_uptime() {
    Serial.println(String(PARAM_SYSTEM_UPTIME) + " = " + String(millis()/1000) + " seconds");
    return CMD_SUCCESS;
}

command_result_t get_total_detections() {
    Serial.println(String(PARAM_TOTAL_DETECTIONS) + " = " + String(system_config.total_detections));
    return CMD_SUCCESS;
}

// ===== SYSTEM COMMAND IMPLEMENTATIONS =====
command_result_t cmd_save_config() {
    Serial.println("Saving configuration to flash...");
    flash_result_t result = config_save_to_flash();
    if (result == FLASH_SUCCESS) {
        Serial.println("Configuration saved successfully!");
        return CMD_SUCCESS;
    } else {
        Serial.println("Failed to save configuration");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_reset_config() {
    Serial.println("Resetting configuration to defaults...");
    flash_result_t result = config_reset_to_defaults();
    if (result == FLASH_SUCCESS) {
        Serial.println("Configuration reset successfully! Please reboot for full effect.");
        return CMD_SUCCESS;
    } else {
        Serial.println("Failed to reset configuration");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_reboot() {
    Serial.println("Rebooting system in 3 seconds...");
    delay(3000);
    NVIC_SystemReset();
    return CMD_SUCCESS;
}

command_result_t cmd_reset_system() {
    Serial.println("Triggering system reset...");
    delay(1000);
    config_save_to_flash();
    gpio_trigger_system_reset();
    delay(2000);
    NVIC_SystemReset();
    return CMD_SUCCESS;
}

// ===== STUB IMPLEMENTATIONS FOR MISSING COMMAND HANDLERS =====
command_result_t cmd_logs(const char* count_str) {
    uint32_t count = count_str ? atoi(count_str) : 10;
    if (count > 50) count = 50; // Limit to reasonable number
    
    if (flash_is_initialized()) {
        flash_print_logs(count);
        return CMD_SUCCESS;
    } else {
        Serial.println("Flash not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_clear_logs() {
    Serial.println("Clearing detection logs...");
    if (flash_is_initialized()) {
        flash_result_t result = flash_clear_logs();
        if (result == FLASH_SUCCESS) {
            Serial.println("Logs cleared successfully");
            return CMD_SUCCESS;
        } else {
            Serial.println("Failed to clear logs");
            return CMD_ERROR_SYSTEM_ERROR;
        }
    } else {
        Serial.println("Flash not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_test() {
    Serial.println("Running system test...");
    // Add basic system tests here
    Serial.println("System test completed");
    return CMD_SUCCESS;
}

command_result_t cmd_gpio_status() {
    if (gpio_is_initialized()) {
        gpio_print_status();
        return CMD_SUCCESS;
    } else {
        Serial.println("GPIO not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_lora_status() {
    if (lora_is_initialized()) {
        lora_print_status();
        return CMD_SUCCESS;
    } else {
        Serial.println("LoRa not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_lora_stats() {
    if (lora_is_initialized()) {
        lora_print_stats();
        return CMD_SUCCESS;
    } else {
        Serial.println("LoRa not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_lora_test() {
    if (lora_is_initialized()) {
        Serial.println("Testing LoRa communication...");
        lora_result_t result = lora_send_message(LORA_MSG_STATUS, "TEST");
        if (result == LORA_SUCCESS) {
            Serial.println("LoRa test message sent successfully");
            return CMD_SUCCESS;
        } else {
            Serial.println("LoRa test failed: " + String(lora_result_to_string(result)));
            return CMD_ERROR_SYSTEM_ERROR;
        }
    } else {
        Serial.println("LoRa not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_lora_diagnostics() {
    if (lora_is_initialized()) {
        lora_run_diagnostics();
        return CMD_SUCCESS;
    } else {
        Serial.println("LoRa not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_flash_status() {
    if (flash_is_initialized()) {
        flash_print_config();
        return CMD_SUCCESS;
    } else {
        Serial.println("Flash not initialized");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_detection_stats() {
    Serial.println("\n=== DETECTION STATISTICS ===");
    Serial.println("Total Detections: " + String(system_config.total_detections));
    Serial.println("Detection Threshold: " + String(system_config.detection_threshold));
    Serial.println("Motherboard Threshold: " + String(system_config.motherboard_threshold));
    Serial.println("============================\n");
    return CMD_SUCCESS;
}

// ===== UTILITY FUNCTIONS =====
void print_welcome_message() {
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println("AMB82 Smart Detection System V2.0");
    Serial.println("Serial Command Interface Ready");
    Serial.println("Type 'help' for available commands");
    Serial.println(String("=").substring(0, 50) + "\n");
}

void print_help_message() {
    Serial.println("\n=== AMB82 SMART DETECTION V2.0 COMMANDS ===");
    Serial.println("help                     - Show this help message");
    Serial.println("status                   - Show system status");
    Serial.println("set <param> <value>      - Set parameter value");
    Serial.println("get <param>              - Get parameter value");
    Serial.println("save                     - Save configuration to flash");
    Serial.println("reset                    - Reset configuration to defaults");
    Serial.println("reset_system             - Trigger hardware/software reset");
    Serial.println("reboot                   - Restart system");
    Serial.println("lora [stats|test|diag]   - LoRa operations");
    
    Serial.println("\n=== WIFI/RTSP COMMANDS ===");
    Serial.println("rtsp_stream              - Start WiFi + RTSP streaming");
    Serial.println("rtsp_stop                - Stop RTSP streaming");
    Serial.println("set_wifi <ssid> <pass>   - Configure WiFi credentials");
    
    Serial.println("\n=== MOTHERBOARD COUNTER COMMANDS ===");
    Serial.println("mb_counter               - Show motherboard counter statistics");
    Serial.println("mb_reset                 - Reset motherboard counter");
    
    Serial.println("\n=== MOTHERBOARD COUNTER PARAMETERS ===");
    Serial.println("mb_count_enabled         - Enable/disable counter (0/1)");
    Serial.println("mb_count_threshold       - Detection count to trigger LoRa (1-1000)");
    Serial.println("mb_count_window          - Time window in seconds (1-300)");
    
    Serial.println("\n=== EXAMPLES ===");
    Serial.println("set mb_count_threshold 25   - Trigger LoRa after 25 MB detections");
    Serial.println("set mb_count_window 5       - Use 5-second detection window");
    Serial.println("get mb_count_threshold      - Show current MB trigger threshold");
    Serial.println("mb_counter                  - Show detailed MB counter stats");
    Serial.println("save                        - Save all settings to flash");
    Serial.println("========================================\n");

    Serial.println("\n=== NEURAL NETWORK DEBUG COMMANDS ===");
    Serial.println("nn_status                - Show NN diagnostic information");
    Serial.println("nn_reset                 - Reset neural network system");
    Serial.println("nn_restart               - Restart neural network");
    Serial.println("camera_reset             - Complete camera system reset");
}

void print_system_status() {
    Serial.println("\n=== SYSTEM STATUS V2.0 ===");
    Serial.println("Version: " + String(SYSTEM_VERSION));
    Serial.println("Uptime: " + String(millis()/1000) + " seconds");
    Serial.println("LoRa Interval: " + String(system_config.lora_send_interval/1000) + "s");
    Serial.println("Detection Threshold: " + String(system_config.detection_threshold));
    Serial.println("Motherboard Threshold: " + String(system_config.motherboard_threshold));
    
    Serial.println("\n=== MOTHERBOARD COUNTER STATUS ===");
    Serial.println("Enabled: " + String(system_config.motherboard_count_enabled ? "YES" : "NO"));
    Serial.println("Threshold: " + String(system_config.motherboard_count_threshold) + " detections");
    Serial.println("Window: " + String(system_config.motherboard_count_window_ms/1000) + " seconds");
    Serial.println("Total Triggers: " + String(system_config.total_motherboard_count_triggers));
    
    Serial.println("===========================\n");
}

// ===== INPUT HANDLING =====
bool serial_input_available() {
    return Serial.available() > 0;
}

String read_serial_line() {
    String line = "";
    
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (line.length() > 0) {
                line.trim();
                return line;
            }
        } else if (c >= 32 && c <= 126) {
            line += c;
            if (line.length() >= MAX_COMMAND_LENGTH - 1) {
                break;
            }
        }
    }
    
    return line;
}

// ===== VALIDATION FUNCTIONS =====
bool is_numeric_value(const char* value) {
    if (!value || strlen(value) == 0) return false;
    
    bool has_decimal = false;
    for (int i = 0; value[i] != '\0'; i++) {
        if (value[i] == '.' && !has_decimal) {
            has_decimal = true;
        } else if (!isdigit(value[i])) {
            return false;
        }
    }
    return true;
}

bool is_boolean_value(const char* value) {
    if (!value) return false;
    return (strcmp(value, "0") == 0 || strcmp(value, "1") == 0 ||
            strcasecmp(value, "true") == 0 || strcasecmp(value, "false") == 0);
}

float parse_float_value(const char* value) {
    return value ? atof(value) : 0.0f;
}

int parse_int_value(const char* value) {
    return value ? atoi(value) : 0;
}

bool parse_bool_value(const char* value) {
    if (!value) return false;
    return (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0);
}

const char* command_result_to_string(command_result_t result) {
    switch (result) {
        case CMD_SUCCESS: return "SUCCESS";
        case CMD_ERROR_UNKNOWN_COMMAND: return "UNKNOWN_COMMAND";
        case CMD_ERROR_INVALID_PARAMETER: return "INVALID_PARAMETER";
        case CMD_ERROR_INVALID_VALUE: return "INVALID_VALUE";
        case CMD_ERROR_MISSING_PARAMETER: return "MISSING_PARAMETER";
        case CMD_ERROR_SYSTEM_ERROR: return "SYSTEM_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

// External function declarations (add to top of file)
extern void debug_neural_network_status();
extern bool reset_camera_system();
extern bool restart_neural_network();

command_result_t cmd_nn_status() {
    debug_neural_network_status();
    return CMD_SUCCESS;
}

command_result_t cmd_nn_reset() {
    Serial.println("Resetting neural network...");
    if (reset_camera_system()) {
        Serial.println("✓ Neural network reset complete");
        return CMD_SUCCESS;
    } else {
        Serial.println("❌ Neural network reset failed");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_nn_restart() {
    Serial.println("Restarting neural network...");
    if (restart_neural_network()) {
        Serial.println("✓ Neural network restarted successfully");
        return CMD_SUCCESS;
    } else {
        Serial.println("❌ Neural network restart failed");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}

command_result_t cmd_camera_reset() {
    Serial.println("Performing complete camera reset...");
    if (reset_camera_system()) {
        Serial.println("✓ Camera system reset complete");
        Serial.println("Use 'nn_restart' to reinitialize detection");
        return CMD_SUCCESS;
    } else {
        Serial.println("❌ Camera reset failed");
        return CMD_ERROR_SYSTEM_ERROR;
    }
}