// serial_commands.h - Serial Command Interface
#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include "config.h"

// ===== COMMAND RESULT TYPES =====
typedef enum {
    CMD_SUCCESS = 0,
    CMD_ERROR_UNKNOWN_COMMAND,
    CMD_ERROR_INVALID_PARAMETER,
    CMD_ERROR_INVALID_VALUE,
    CMD_ERROR_MISSING_PARAMETER,
    CMD_ERROR_SYSTEM_ERROR
} command_result_t;

// ===== COMMAND STRUCTURE =====
typedef struct {
    char command[32];
    char parameter[32];
    char value[64];
    bool has_parameter;
    bool has_value;
} parsed_command_t;

// ===== SERIAL COMMAND INITIALIZATION =====
void serial_commands_init();
bool serial_commands_is_enabled();

// ===== COMMAND PROCESSING =====
void serial_commands_process();
command_result_t serial_commands_parse_input(const char* input, parsed_command_t* cmd);
command_result_t serial_commands_execute(parsed_command_t* cmd);

// ===== BASIC COMMAND HANDLERS =====
command_result_t cmd_help();
command_result_t cmd_status();
command_result_t cmd_set_parameter(const char* parameter, const char* value);
command_result_t cmd_get_parameter(const char* parameter);
command_result_t cmd_save_config();
command_result_t cmd_reset_config();
command_result_t cmd_reboot();
command_result_t cmd_logs(const char* count_str);
command_result_t cmd_clear_logs();
command_result_t cmd_test();
command_result_t cmd_gpio_status();
command_result_t cmd_lora_status();
command_result_t cmd_lora_stats();
command_result_t cmd_lora_test();
command_result_t cmd_flash_status();
command_result_t cmd_detection_stats();
command_result_t cmd_reset_system();

// ===== WIFI/RTSP COMMAND HANDLERS - FIXED RETURN TYPES =====
command_result_t cmd_rtsp_stream();
command_result_t cmd_rtsp_stop();
command_result_t cmd_set_wifi(const char* ssid, const char* password);
command_result_t cmd_lora_diagnostics();

// ===== MOTHERBOARD COUNTER COMMAND HANDLERS =====
command_result_t cmd_motherboard_counter();
command_result_t cmd_motherboard_reset();

// ===== PARAMETER HANDLERS =====
command_result_t set_lora_interval(const char* value);
command_result_t set_detection_threshold(const char* value);
command_result_t set_motherboard_threshold(const char* value);
command_result_t set_fan_cycle_interval(const char* value);
command_result_t set_laser_blink_interval(const char* value);
command_result_t set_debug_level(const char* value);
command_result_t set_fan_enabled(const char* value);
command_result_t set_crosshair_enabled(const char* value);

// ===== MOTHERBOARD COUNTER PARAMETER HANDLERS =====
command_result_t set_motherboard_count_enabled(const char* value);
command_result_t set_motherboard_count_threshold(const char* value);
command_result_t set_motherboard_count_window(const char* value);

// ===== GET PARAMETER HANDLERS =====
command_result_t get_lora_interval();
command_result_t get_detection_threshold();
command_result_t get_motherboard_threshold();
command_result_t get_fan_cycle_interval();
command_result_t get_laser_blink_interval();
command_result_t get_debug_level();
command_result_t get_system_uptime();
command_result_t get_total_detections();

// ===== MOTHERBOARD COUNTER GET PARAMETER HANDLERS =====
command_result_t get_motherboard_count_enabled();
command_result_t get_motherboard_count_threshold();
command_result_t get_motherboard_count_window();

// ===== UTILITY FUNCTIONS =====
void print_welcome_message();
void print_help_message();
void print_system_status();
const char* command_result_to_string(command_result_t result);

// ===== INPUT HANDLING =====
bool serial_input_available();
String read_serial_line();
void clear_serial_buffer();

// ===== COMMAND VALIDATION =====
bool is_numeric_value(const char* value);
bool is_boolean_value(const char* value);
float parse_float_value(const char* value);
int parse_int_value(const char* value);
bool parse_bool_value(const char* value);

command_result_t cmd_nn_status();
command_result_t cmd_nn_reset(); 
command_result_t cmd_nn_restart();
command_result_t cmd_camera_reset();


// ===== COMMAND CONSTANTS =====
#define MAX_COMMAND_LENGTH      256
#define MAX_TOKENS             4
#define COMMAND_TIMEOUT        100

// ===== AVAILABLE COMMANDS =====
#define CMD_HELP               "help"
#define CMD_STATUS             "status"
#define CMD_SET                "set"
#define CMD_GET                "get"
#define CMD_SAVE               "save"
#define CMD_RESET              "reset"
#define CMD_REBOOT             "reboot"
#define CMD_LOGS               "logs"
#define CMD_CLEAR_LOGS         "clear_logs"
#define CMD_TEST               "test"
#define CMD_GPIO               "gpio"
#define CMD_LORA               "lora"
#define CMD_FLASH              "flash"
#define CMD_DETECTION          "detection"

// ===== AVAILABLE PARAMETERS =====
#define PARAM_LORA_INTERVAL           "lora_interval"
#define PARAM_DETECTION_THRESHOLD     "detection_threshold"
#define PARAM_MOTHERBOARD_THRESHOLD   "motherboard_threshold"
#define PARAM_FAN_CYCLE_INTERVAL      "fan_cycle_interval"
#define PARAM_LASER_BLINK_INTERVAL    "laser_blink_interval"
#define PARAM_DEBUG_LEVEL             "debug_level"
#define PARAM_FAN_ENABLED             "fan_enabled"
#define PARAM_CROSSHAIR_ENABLED       "crosshair_enabled"
#define PARAM_SYSTEM_UPTIME           "system_uptime"
#define PARAM_TOTAL_DETECTIONS        "total_detections"

// ===== MOTHERBOARD COUNTER PARAMETERS =====
#define PARAM_MOTHERBOARD_COUNT_ENABLED       "mb_count_enabled"
#define PARAM_MOTHERBOARD_COUNT_THRESHOLD     "mb_count_threshold"
#define PARAM_MOTHERBOARD_COUNT_WINDOW        "mb_count_window"



#endif // SERIAL_COMMANDS_H