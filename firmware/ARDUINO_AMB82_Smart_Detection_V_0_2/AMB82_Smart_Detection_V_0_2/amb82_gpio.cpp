// amb82_gpio.cpp - GPIO Control Implementation
#include "amb82_gpio.h"

// ===== GLOBAL VARIABLES =====
gpio_module_t gpio_module = {0};

// ===== GPIO INITIALIZATION =====
gpio_result_t gpio_init() {
    INFO_PRINT("Initializing GPIO module...");
    
    // Initialize all GPIO controls
    gpio_result_t result;
    
    result = gpio_fan_init();
    if (result != GPIO_SUCCESS) {
        ERROR_PRINT("Fan GPIO initialization failed");
        return result;
    }
    
    result = gpio_laser_init();
    if (result != GPIO_SUCCESS) {
        ERROR_PRINT("Laser GPIO initialization failed");
        return result;
    }
    
    result = gpio_reset_control_init();
    if (result != GPIO_SUCCESS) {
        ERROR_PRINT("Reset control GPIO initialization failed");
        return result;
    }
    
    result = gpio_status_led_init();
    if (result != GPIO_SUCCESS) {
        ERROR_PRINT("Status LED GPIO initialization failed");
        return result;
    }
    
    gpio_module.initialized = true;
    
    // Set initial status LED pattern
    gpio_status_led_set_pattern(LED_PATTERN_SLOW_BLINK);
    
    INFO_PRINT("GPIO module initialized successfully");
    return GPIO_SUCCESS;
}

bool gpio_is_initialized() {
    return gpio_module.initialized;
}

// ===== FAN CONTROL =====
gpio_result_t gpio_fan_init() {
    INFO_PRINT("Initializing fan control...");
    
    // Configure fan pin as output
    pinMode(PIN_FAN, OUTPUT);
    digitalWrite(PIN_FAN, LOW);
    
    // Initialize fan control structure
    gpio_module.fan.enabled = system_config.fan_enabled;
    gpio_module.fan.state = GPIO_STATE_OFF;
    gpio_module.fan.cycle_interval = system_config.fan_cycle_interval;
    gpio_module.fan.current_state = false;
    gpio_module.fan.last_toggle_time = millis();
    gpio_module.fan.total_on_time = 0;
    gpio_module.fan.total_cycles = 0;
    
    INFO_PRINT("Fan control initialized - Cycle: " + String(gpio_module.fan.cycle_interval/1000) + "s");
    return GPIO_SUCCESS;
}

void gpio_fan_process() {
    if (!gpio_module.fan.enabled) {
        return;
    }
    
    uint32_t current_time = millis();
    uint32_t elapsed_time = current_time - gpio_module.fan.last_toggle_time;
    
    // Check if it's time to toggle the fan state
    if (elapsed_time >= gpio_module.fan.cycle_interval) {
        // Toggle fan state
        gpio_module.fan.current_state = !gpio_module.fan.current_state;
        digitalWrite(PIN_FAN, gpio_module.fan.current_state ? HIGH : LOW);
        
        // Update statistics
        if (gpio_module.fan.current_state) {
            DEBUG_PRINT(3, "Fan turned ON");
            gpio_module.fan.state = GPIO_STATE_ON;
            gpio_module.fan.total_cycles++;
        } else {
            DEBUG_PRINT(3, "Fan turned OFF");
            gpio_module.fan.state = GPIO_STATE_OFF;
            gpio_module.fan.total_on_time += elapsed_time;
        }
        
        gpio_module.fan.last_toggle_time = current_time;
    }
}

gpio_result_t gpio_fan_enable(bool enable) {
    gpio_module.fan.enabled = enable;
    system_config.fan_enabled = enable;
    
    if (!enable) {
        digitalWrite(PIN_FAN, LOW);
        gpio_module.fan.current_state = false;
        gpio_module.fan.state = GPIO_STATE_OFF;
    }
    
    INFO_PRINT("Fan " + String(enable ? "enabled" : "disabled"));
    return GPIO_SUCCESS;
}

gpio_result_t gpio_fan_set_cycle_interval(uint32_t interval_ms) {
    gpio_module.fan.cycle_interval = interval_ms;
    system_config.fan_cycle_interval = interval_ms;
    
    INFO_PRINT("Fan cycle interval set to " + String(interval_ms/1000) + " seconds");
    return GPIO_SUCCESS;
}

bool gpio_fan_get_state() {
    return gpio_module.fan.current_state;
}

uint32_t gpio_fan_get_on_time() {
    uint32_t total_time = gpio_module.fan.total_on_time;
    
    // Add current on-time if fan is currently on
    if (gpio_module.fan.current_state) {
        total_time += (millis() - gpio_module.fan.last_toggle_time);
    }
    
    return total_time;
}

uint32_t gpio_fan_get_cycles() {
    return gpio_module.fan.total_cycles;
}

void gpio_fan_reset_stats() {
    gpio_module.fan.total_on_time = 0;
    gpio_module.fan.total_cycles = 0;
    INFO_PRINT("Fan statistics reset");
}

// ===== CROSSHAIR LASER CONTROL =====
gpio_result_t gpio_laser_init() {
    INFO_PRINT("Initializing crosshair laser control...");
    
    // Configure laser pin as output
    pinMode(PIN_CROSSHAIR_LASER, OUTPUT);
    digitalWrite(PIN_CROSSHAIR_LASER, LOW);
    
    // Initialize laser control structure
    gpio_module.crosshair_laser.enabled = system_config.crosshair_enabled;
    gpio_module.crosshair_laser.state = GPIO_STATE_OFF;
    gpio_module.crosshair_laser.blink_interval = system_config.laser_blink_interval;
    gpio_module.crosshair_laser.current_state = false;
    gpio_module.crosshair_laser.should_blink = false;
    gpio_module.crosshair_laser.motherboard_confidence = 0.0f;
    gpio_module.crosshair_laser.last_detection_time = 0;
    gpio_module.crosshair_laser.detection_timeout = LASER_DEFAULT_DETECTION_TIMEOUT;
    gpio_module.crosshair_laser.last_blink_time = millis();
    
    INFO_PRINT("Crosshair laser initialized - Blink: " + String(gpio_module.crosshair_laser.blink_interval) + "ms");
    return GPIO_SUCCESS;
}

void gpio_laser_process() {
    if (!gpio_module.crosshair_laser.enabled) {
        return;
    }
    
    uint32_t current_time = millis();
    
    // Check if motherboard detection timeout has elapsed
    if (gpio_module.crosshair_laser.last_detection_time > 0) {
        uint32_t time_since_detection = current_time - gpio_module.crosshair_laser.last_detection_time;
        
        if (time_since_detection > gpio_module.crosshair_laser.detection_timeout) {
            // No recent detection or confidence too low - should blink
            gpio_module.crosshair_laser.should_blink = true;
        } else {
            // Recent detection with good confidence - don't blink
            gpio_module.crosshair_laser.should_blink = false;
        }
    } else {
        // No detection yet - should blink
        gpio_module.crosshair_laser.should_blink = true;
    }
    
    // Handle blinking logic
    if (gpio_module.crosshair_laser.should_blink) {
        uint32_t elapsed_blink_time = current_time - gpio_module.crosshair_laser.last_blink_time;
        
        if (elapsed_blink_time >= gpio_module.crosshair_laser.blink_interval) {
            // Toggle laser state
            gpio_module.crosshair_laser.current_state = !gpio_module.crosshair_laser.current_state;
            digitalWrite(PIN_CROSSHAIR_LASER, gpio_module.crosshair_laser.current_state ? HIGH : LOW);
            
            gpio_module.crosshair_laser.state = gpio_module.crosshair_laser.current_state ? 
                                               GPIO_STATE_BLINKING : GPIO_STATE_BLINKING;
            gpio_module.crosshair_laser.last_blink_time = current_time;
            
            DEBUG_PRINT(3, "Crosshair laser " + String(gpio_module.crosshair_laser.current_state ? "ON" : "OFF"));
        }
    } else {
        // Should not blink - turn off laser
        if (gpio_module.crosshair_laser.current_state) {
            digitalWrite(PIN_CROSSHAIR_LASER, LOW);
            gpio_module.crosshair_laser.current_state = false;
            gpio_module.crosshair_laser.state = GPIO_STATE_OFF;
            DEBUG_PRINT(3, "Crosshair laser OFF (good detection)");
        }
    }
}

gpio_result_t gpio_laser_enable(bool enable) {
    gpio_module.crosshair_laser.enabled = enable;
    system_config.crosshair_enabled = enable;
    
    if (!enable) {
        digitalWrite(PIN_CROSSHAIR_LASER, LOW);
        gpio_module.crosshair_laser.current_state = false;
        gpio_module.crosshair_laser.state = GPIO_STATE_OFF;
    }
    
    INFO_PRINT("Crosshair laser " + String(enable ? "enabled" : "disabled"));
    return GPIO_SUCCESS;
}

gpio_result_t gpio_laser_set_blink_interval(uint32_t interval_ms) {
    gpio_module.crosshair_laser.blink_interval = interval_ms;
    system_config.laser_blink_interval = interval_ms;
    
    INFO_PRINT("Laser blink interval set to " + String(interval_ms) + "ms");
    return GPIO_SUCCESS;
}

gpio_result_t gpio_laser_set_detection_timeout(uint32_t timeout_ms) {
    gpio_module.crosshair_laser.detection_timeout = timeout_ms;
    
    INFO_PRINT("Laser detection timeout set to " + String(timeout_ms) + "ms");
    return GPIO_SUCCESS;
}

void gpio_laser_update_detection(float motherboard_confidence) {
    gpio_module.crosshair_laser.motherboard_confidence = motherboard_confidence;
    
    // Check if confidence meets threshold
    if (motherboard_confidence >= system_config.motherboard_threshold) {
        gpio_module.crosshair_laser.last_detection_time = millis();
        DEBUG_PRINT(3, "Motherboard detected - confidence: " + String(motherboard_confidence));
    }
}

void gpio_laser_force_on() {
    digitalWrite(PIN_CROSSHAIR_LASER, HIGH);
    gpio_module.crosshair_laser.current_state = true;
    gpio_module.crosshair_laser.state = GPIO_STATE_ON;
}

void gpio_laser_force_off() {
    digitalWrite(PIN_CROSSHAIR_LASER, LOW);
    gpio_module.crosshair_laser.current_state = false;
    gpio_module.crosshair_laser.state = GPIO_STATE_OFF;
}

bool gpio_laser_get_state() {
    return gpio_module.crosshair_laser.current_state;
}

bool gpio_laser_should_blink() {
    return gpio_module.crosshair_laser.should_blink;
}

// ===== RESET CONTROL =====
gpio_result_t gpio_reset_control_init() {
    INFO_PRINT("Initializing reset control...");
    
    // Configure reset control pin as output
    pinMode(PIN_RESET_CONTROL, OUTPUT);
    digitalWrite(PIN_RESET_CONTROL, LOW); // Default to LOW (no reset)
    
    // Initialize reset control structure
    gpio_module.reset_control.enabled = true;
    gpio_module.reset_control.current_state = false;
    gpio_module.reset_control.state = GPIO_STATE_OFF;
    
    INFO_PRINT("Reset control initialized - State: OFF");
    return GPIO_SUCCESS;
}

gpio_result_t gpio_trigger_system_reset() {
    INFO_PRINT("Triggering system reset via GPIO pin...");
    
    // Set reset pin HIGH to trigger reset
    digitalWrite(PIN_RESET_CONTROL, HIGH);
    gpio_module.reset_control.current_state = true;
    gpio_module.reset_control.state = GPIO_STATE_ON;
    
    // Keep the pin high for a short duration
    delay(100);
    
    // Note: The system should reset before reaching this point
    // But if it doesn't, we'll reset the pin state
    digitalWrite(PIN_RESET_CONTROL, LOW);
    gpio_module.reset_control.current_state = false;
    gpio_module.reset_control.state = GPIO_STATE_OFF;
    
    return GPIO_SUCCESS;
}

gpio_result_t gpio_set_reset_pin(bool state) {
    digitalWrite(PIN_RESET_CONTROL, state ? HIGH : LOW);
    gpio_module.reset_control.current_state = state;
    gpio_module.reset_control.state = state ? GPIO_STATE_ON : GPIO_STATE_OFF;
    
    DEBUG_PRINT(3, "Reset pin set to " + String(state ? "HIGH" : "LOW"));
    return GPIO_SUCCESS;
}

bool gpio_get_reset_pin_state() {
    return gpio_module.reset_control.current_state;
}

// ===== STATUS LED CONTROL =====
gpio_result_t gpio_status_led_init() {
    INFO_PRINT("Initializing status LED control...");
    
    // Configure status LED pin as output
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
    
    // Initialize status LED control structure
    gpio_module.status_led.enabled = true;
    gpio_module.status_led.state = GPIO_STATE_OFF;
    gpio_module.status_led.blink_interval = STATUS_LED_DEFAULT_INTERVAL;
    gpio_module.status_led.current_state = false;
    gpio_module.status_led.blink_pattern = LED_PATTERN_SLOW_BLINK;
    gpio_module.status_led.pattern_count = 0;
    gpio_module.status_led.pattern_index = 0;
    gpio_module.status_led.last_blink_time = millis();
    
    INFO_PRINT("Status LED initialized");
    return GPIO_SUCCESS;
}

void gpio_status_led_process() {
    if (!gpio_module.status_led.enabled) {
        return;
    }
    
    uint32_t current_time = millis();
    uint32_t elapsed_time = current_time - gpio_module.status_led.last_blink_time;
    
    switch (gpio_module.status_led.blink_pattern) {
        case LED_PATTERN_OFF:
            digitalWrite(PIN_STATUS_LED, LOW);
            gpio_module.status_led.current_state = false;
            break;
            
        case LED_PATTERN_SOLID_ON:
            digitalWrite(PIN_STATUS_LED, HIGH);
            gpio_module.status_led.current_state = true;
            break;
            
        case LED_PATTERN_SLOW_BLINK:
            if (elapsed_time >= gpio_module.status_led.blink_interval) {
                gpio_module.status_led.current_state = !gpio_module.status_led.current_state;
                digitalWrite(PIN_STATUS_LED, gpio_module.status_led.current_state ? HIGH : LOW);
                gpio_module.status_led.last_blink_time = current_time;
            }
            break;
            
        case LED_PATTERN_FAST_BLINK:
            if (elapsed_time >= (gpio_module.status_led.blink_interval / 4)) {
                gpio_module.status_led.current_state = !gpio_module.status_led.current_state;
                digitalWrite(PIN_STATUS_LED, gpio_module.status_led.current_state ? HIGH : LOW);
                gpio_module.status_led.last_blink_time = current_time;
            }
            break;
            
        case LED_PATTERN_DOUBLE_BLINK:
        case LED_PATTERN_TRIPLE_BLINK:
            // Implement pattern-based blinking
            if (elapsed_time >= (gpio_module.status_led.blink_interval / 8)) {
                uint8_t max_blinks = (gpio_module.status_led.blink_pattern == LED_PATTERN_DOUBLE_BLINK) ? 4 : 6;
                
                if (gpio_module.status_led.pattern_index < max_blinks) {
                    gpio_module.status_led.current_state = !gpio_module.status_led.current_state;
                    digitalWrite(PIN_STATUS_LED, gpio_module.status_led.current_state ? HIGH : LOW);
                    gpio_module.status_led.pattern_index++;
                } else {
                    // Pattern complete, wait before repeating
                    if (elapsed_time >= gpio_module.status_led.blink_interval) {
                        gpio_module.status_led.pattern_index = 0;
                        gpio_module.status_led.last_blink_time = current_time;
                    }
                }
            }
            break;
    }
}

gpio_result_t gpio_status_led_set_pattern(uint8_t pattern) {
    if (pattern > LED_PATTERN_SOLID_ON) {
        return GPIO_ERROR_INVALID_VALUE;
    }
    
    gpio_module.status_led.blink_pattern = pattern;
    gpio_module.status_led.pattern_index = 0;
    gpio_module.status_led.last_blink_time = millis();
    
    DEBUG_PRINT(3, "Status LED pattern set to " + String(pattern));
    return GPIO_SUCCESS;
}

gpio_result_t gpio_status_led_set_interval(uint32_t interval_ms) {
    gpio_module.status_led.blink_interval = interval_ms;
    
    DEBUG_PRINT(3, "Status LED interval set to " + String(interval_ms) + "ms");
    return GPIO_SUCCESS;
}

void gpio_status_led_force_on() {
    digitalWrite(PIN_STATUS_LED, HIGH);
    gpio_module.status_led.current_state = true;
    gpio_module.status_led.blink_pattern = LED_PATTERN_SOLID_ON;
}

void gpio_status_led_force_off() {
    digitalWrite(PIN_STATUS_LED, LOW);
    gpio_module.status_led.current_state = false;
    gpio_module.status_led.blink_pattern = LED_PATTERN_OFF;
}

bool gpio_status_led_get_state() {
    return gpio_module.status_led.current_state;
}

// ===== GPIO PROCESSING =====
void gpio_process_all() {
    gpio_fan_process();
    gpio_laser_process();
    gpio_status_led_process();
}

// ===== GPIO UTILITIES =====
void gpio_print_status() {
    Serial.println("\n=== GPIO STATUS ===");
    Serial.println("Initialized: " + String(gpio_module.initialized ? "YES" : "NO"));
    
    Serial.println("\nFan Control:");
    Serial.println("  Enabled: " + String(gpio_module.fan.enabled ? "YES" : "NO"));
    Serial.println("  State: " + String(gpio_state_to_string(gpio_module.fan.state)));
    Serial.println("  Current: " + String(gpio_module.fan.current_state ? "ON" : "OFF"));
    Serial.println("  Cycle: " + String(gpio_module.fan.cycle_interval/1000) + "s");
    Serial.println("  Total Cycles: " + String(gpio_module.fan.total_cycles));
    
    Serial.println("\nCrosshair Laser:");
    Serial.println("  Enabled: " + String(gpio_module.crosshair_laser.enabled ? "YES" : "NO"));
    Serial.println("  State: " + String(gpio_state_to_string(gpio_module.crosshair_laser.state)));
    Serial.println("  Should Blink: " + String(gpio_module.crosshair_laser.should_blink ? "YES" : "NO"));
    Serial.println("  MB Confidence: " + String(gpio_module.crosshair_laser.motherboard_confidence));
    Serial.println("  Blink Interval: " + String(gpio_module.crosshair_laser.blink_interval) + "ms");
    
    Serial.println("\nReset Control:");
    Serial.println("  State: " + String(gpio_module.reset_control.current_state ? "HIGH" : "LOW"));
    
    Serial.println("\nStatus LED:");
    Serial.println("  Pattern: " + String(gpio_module.status_led.blink_pattern));
    Serial.println("  State: " + String(gpio_module.status_led.current_state ? "ON" : "OFF"));
    Serial.println("==================\n");
}

void gpio_print_fan_stats() {
    Serial.println("\n=== FAN STATISTICS ===");
    Serial.println("Total Cycles: " + String(gpio_fan_get_cycles()));
    Serial.println("Total On Time: " + String(gpio_fan_get_on_time()/1000) + "s");
    Serial.println("Current State: " + String(gpio_fan_get_state() ? "ON" : "OFF"));
    Serial.println("Cycle Interval: " + String(gpio_module.fan.cycle_interval/1000) + "s");
    Serial.println("======================\n");
}

void gpio_print_laser_stats() {
    Serial.println("\n=== LASER STATISTICS ===");
    Serial.println("Should Blink: " + String(gpio_laser_should_blink() ? "YES" : "NO"));
    Serial.println("Current State: " + String(gpio_laser_get_state() ? "ON" : "OFF"));
    Serial.println("MB Confidence: " + String(gpio_module.crosshair_laser.motherboard_confidence));
    Serial.println("Detection Timeout: " + String(gpio_module.crosshair_laser.detection_timeout) + "ms");
    Serial.println("========================\n");
}

const char* gpio_state_to_string(gpio_state_t state) {
    switch (state) {
        case GPIO_STATE_OFF: return "OFF";
        case GPIO_STATE_ON: return "ON";
        case GPIO_STATE_BLINKING: return "BLINKING";
        case GPIO_STATE_PWM: return "PWM";
        default: return "UNKNOWN";
    }
}