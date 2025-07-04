// amb82_gpio.h - GPIO Control Module
#ifndef AMB82_GPIO_H
#define AMB82_GPIO_H

#include "config.h"

// ===== GPIO OPERATION RESULTS =====
typedef enum {
    GPIO_SUCCESS = 0,
    GPIO_ERROR_INIT,
    GPIO_ERROR_INVALID_PIN,
    GPIO_ERROR_INVALID_VALUE
} gpio_result_t;

// ===== GPIO PIN STATES =====
typedef enum {
    GPIO_STATE_OFF = 0,
    GPIO_STATE_ON = 1,
    GPIO_STATE_BLINKING = 2,
    GPIO_STATE_PWM = 3
} gpio_state_t;

// ===== FAN CONTROL =====
typedef struct {
    bool enabled;
    gpio_state_t state;
    uint32_t last_toggle_time;
    uint32_t cycle_interval;        // 3 minutes on/off cycle
    bool current_state;             // true = ON, false = OFF
    uint32_t total_on_time;
    uint32_t total_cycles;
} fan_control_t;

// ===== CROSSHAIR LASER CONTROL =====
typedef struct {
    bool enabled;
    gpio_state_t state;
    uint32_t last_blink_time;
    uint32_t blink_interval;
    bool current_state;             // true = ON, false = OFF
    bool should_blink;              // Based on motherboard detection
    float motherboard_confidence;   // Last detected confidence
    uint32_t last_detection_time;
    uint32_t detection_timeout;     // Time to wait before starting blink
} laser_control_t;

// ===== RESET CONTROL =====
typedef struct {
    bool enabled;
    gpio_state_t state;
    bool current_state;
} reset_control_t;

// ===== STATUS LED CONTROL =====
typedef struct {
    bool enabled;
    gpio_state_t state;
    uint32_t last_blink_time;
    uint32_t blink_interval;
    bool current_state;
    uint8_t blink_pattern;          // Different patterns for different states
    uint8_t pattern_count;
    uint8_t pattern_index;
} status_led_control_t;

// ===== GPIO MODULE STATE =====
typedef struct {
    bool initialized;
    fan_control_t fan;
    laser_control_t crosshair_laser;
    reset_control_t reset_control;
    status_led_control_t status_led;
} gpio_module_t;

// ===== GPIO INITIALIZATION =====
gpio_result_t gpio_init();
bool gpio_is_initialized();

// ===== FAN CONTROL =====
gpio_result_t gpio_fan_init();
void gpio_fan_process();
gpio_result_t gpio_fan_enable(bool enable);
gpio_result_t gpio_fan_set_cycle_interval(uint32_t interval_ms);
bool gpio_fan_get_state();
uint32_t gpio_fan_get_on_time();
uint32_t gpio_fan_get_cycles();
void gpio_fan_reset_stats();

// ===== CROSSHAIR LASER CONTROL =====
gpio_result_t gpio_laser_init();
void gpio_laser_process();
gpio_result_t gpio_laser_enable(bool enable);
gpio_result_t gpio_laser_set_blink_interval(uint32_t interval_ms);
gpio_result_t gpio_laser_set_detection_timeout(uint32_t timeout_ms);
void gpio_laser_update_detection(float motherboard_confidence);
void gpio_laser_force_on();
void gpio_laser_force_off();
bool gpio_laser_get_state();
bool gpio_laser_should_blink();

// ===== RESET CONTROL =====
gpio_result_t gpio_reset_control_init();
gpio_result_t gpio_trigger_system_reset();
gpio_result_t gpio_set_reset_pin(bool state);
bool gpio_get_reset_pin_state();

// ===== STATUS LED CONTROL =====
gpio_result_t gpio_status_led_init();
void gpio_status_led_process();
gpio_result_t gpio_status_led_set_pattern(uint8_t pattern);
gpio_result_t gpio_status_led_set_interval(uint32_t interval_ms);
void gpio_status_led_force_on();
void gpio_status_led_force_off();
bool gpio_status_led_get_state();

// ===== GPIO PROCESSING =====
void gpio_process_all();

// ===== GPIO UTILITIES =====
void gpio_print_status();
void gpio_print_fan_stats();
void gpio_print_laser_stats();
const char* gpio_state_to_string(gpio_state_t state);

// ===== GLOBAL GPIO INSTANCE =====
extern gpio_module_t gpio_module;

// ===== GPIO CONSTANTS =====
#define FAN_DEFAULT_CYCLE_INTERVAL      (3 * 60 * 1000)    // 3 minutes
#define LASER_DEFAULT_BLINK_INTERVAL    500                // 500ms
#define LASER_DEFAULT_DETECTION_TIMEOUT (2 * 1000)         // 2 seconds
#define STATUS_LED_DEFAULT_INTERVAL     1000               // 1 second

// ===== STATUS LED PATTERNS =====
#define LED_PATTERN_OFF                 0
#define LED_PATTERN_SLOW_BLINK          1   // Normal operation
#define LED_PATTERN_FAST_BLINK          2   // Error state
#define LED_PATTERN_DOUBLE_BLINK        3   // Detection event
#define LED_PATTERN_TRIPLE_BLINK        4   // LoRa transmission
#define LED_PATTERN_SOLID_ON            5   // Initialization

#endif // AMB82_GPIO_H