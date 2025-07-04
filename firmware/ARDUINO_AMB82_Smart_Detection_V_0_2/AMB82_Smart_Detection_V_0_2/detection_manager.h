// detection_manager.h - Object Detection Integration
#ifndef DETECTION_MANAGER_H
#define DETECTION_MANAGER_H

#include "config.h"
#include "NNObjectDetection.h"
#include <vector>

// Don't include ObjectClassList.h here to avoid multiple definitions
// Use helper functions instead of direct itemList access

// ===== DETECTION OPERATION RESULTS =====
typedef enum {
    DETECTION_SUCCESS = 0,
    DETECTION_ERROR_INIT,
    DETECTION_ERROR_CAMERA,
    DETECTION_ERROR_MODEL,
    DETECTION_ERROR_PROCESSING,
    DETECTION_ERROR_NO_RESULTS
} detection_result_code_t;

// ===== DETECTION STATISTICS =====
typedef struct {
    uint32_t total_frames_processed;
    uint32_t total_detections_found;
    uint32_t led_on_detections;
    uint32_t motherboard_detections;
    uint32_t false_detections;
    uint32_t processing_errors;
    
    float avg_confidence_led;
    float avg_confidence_motherboard;
    float max_confidence_led;
    float max_confidence_motherboard;
    
    uint32_t last_detection_time;
    uint32_t detection_processing_time_ms;
    
    // Performance metrics
    uint32_t avg_processing_time_ms;
    uint32_t min_processing_time_ms;
    uint32_t max_processing_time_ms;
} detection_stats_t;

// ===== DETECTION EVENT CALLBACKS =====
typedef void (*detection_callback_t)(detection_result_t* result);

// ===== DETECTION MANAGER STATE =====
typedef struct {
    bool initialized;
    bool enabled;
    bool camera_active;
    bool model_loaded;
    
    detection_stats_t stats;
    detection_callback_t callback;
    
    uint32_t last_process_time;
    uint32_t process_interval;
    
    // Current detection results
    std::vector<ObjectDetectionResult>* current_results;
    uint8_t current_result_count;
    
    // Filtering and thresholds
    float confidence_threshold;
    float motherboard_threshold;
    
    // WiFi and streaming
    bool wifi_connected;
    bool rtsp_active;
    char wifi_ssid[32];
    char wifi_password[32];
} detection_manager_t;

// ===== DETECTION INITIALIZATION =====
detection_result_code_t detection_init();
detection_result_code_t detection_init_camera();
detection_result_code_t detection_init_model();
detection_result_code_t detection_init_streaming();
bool detection_is_initialized();

// ===== DETECTION PROCESSING =====
void detection_process();
detection_result_code_t detection_process_frame();
detection_result_code_t detection_analyze_results();
void detection_update_statistics(detection_result_t* result);

// ===== DETECTION CONFIGURATION =====
detection_result_code_t detection_set_thresholds(float detection_threshold, float motherboard_threshold);
detection_result_code_t detection_enable(bool enable);
detection_result_code_t detection_set_process_interval(uint32_t interval_ms);
void detection_set_callback(detection_callback_t callback);

// ===== DETECTION RESULTS HANDLING =====
uint8_t detection_get_result_count();
detection_result_code_t detection_get_result(uint8_t index, detection_result_t* result);
bool detection_has_motherboard_detected();
float detection_get_last_motherboard_confidence();
uint32_t detection_get_last_detection_time();

// ===== DETECTION STATISTICS =====
void detection_get_statistics(detection_stats_t* stats);
void detection_reset_statistics();
void detection_print_statistics();
void detection_print_current_results();

// ===== WIFI AND STREAMING =====
detection_result_code_t detection_setup_wifi(const char* ssid, const char* password);
bool detection_is_wifi_connected();
const char* detection_get_rtsp_url();
void detection_print_network_info();

// ===== DETECTION UTILITIES =====
const char* detection_result_code_to_string(detection_result_code_t code);
const char* detection_class_to_string(uint8_t class_id);
uint8_t detection_string_to_class(const char* class_name);

// ===== OBJECT CLASS HELPERS =====
// These functions provide access to ObjectClassList without exposing itemList directly
bool detection_is_object_filtered(int obj_type);
const char* detection_get_object_name(int obj_type);
int detection_get_object_count();

// ===== DETECTION CONVERSION HELPERS =====
detection_result_code_t detection_convert_yolo_result(ObjectDetectionResult* yolo_result, detection_result_t* our_result);
void detection_scale_coordinates(detection_result_t* result, uint16_t img_width, uint16_t img_height);

// ===== GLOBAL DETECTION MANAGER =====
extern detection_manager_t detection_manager;

#define CHANNEL   0
#define CHANNELNN 3

// Lower resolution for NN processing
#define NNWIDTH  576
#define NNHEIGHT 320


// ===== DETECTION CONSTANTS =====
#define DETECTION_DEFAULT_INTERVAL      100     // 100ms between processing
#define DETECTION_MAX_RESULTS           10      // Maximum results to process
#define DETECTION_CONFIDENCE_MIN        0.1f    // Minimum confidence to consider
#define DETECTION_CONFIDENCE_MAX        1.0f    // Maximum confidence possible

// ===== WIFI CONSTANTS =====
#define WIFI_CONNECT_TIMEOUT           10000    // 10 seconds
#define WIFI_RETRY_DELAY               2000     // 2 seconds between retries
#define WIFI_MAX_RETRIES               5        // Maximum connection attempts

// ===== RTSP STREAMING CONSTANTS =====
#define RTSP_DEFAULT_PORT              554
#define RTSP_BITRATE                   (2 * 1024 * 1024)  // 2 Mbps

#endif // DETECTION_MANAGER_H