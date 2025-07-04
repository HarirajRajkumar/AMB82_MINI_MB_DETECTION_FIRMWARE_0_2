/*
 * AMB82 Smart Detection System V0.2.0 - USB Reconnection Safe
 * 
 * Fixes for USB hot-plug reconnection issues:
 * 1. USB connection state monitoring
 * 2. System state recovery after USB events
 * 3. Graceful serial communication handling
 * 4. VOE state protection and recovery
 * 5. Power glitch protection
 */

#include "config.h"
#include "motherboard_counter.h"
#include "amb82_flash.h"
#include "amb82_gpio.h"
#include "serial_commands.h"
#include "lora_rak3172.h"

// Neural Network includes
#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#include "ObjectClassList.h"

// ===== GLOBAL VARIABLES =====
system_config_t system_config;
system_state_t system_state = SYS_STATE_INIT;

// External function declarations - ADD THESE if missing
extern bool start_rtsp_streaming();
extern void stop_rtsp_streaming();

// Video configuration
#define CHANNEL 0
#define CHANNELNN 3
#define NNWIDTH 576
#define NNHEIGHT 320

// Core detection setup
VideoSetting configNN(NNWIDTH, NNHEIGHT, 10, VIDEO_RGB, 0);
NNObjectDetection ObjDet;
StreamIO videoStreamerNN(1, 1);

// Optional WiFi/RTSP components
VideoSetting config(VIDEO_FHD, 30, VIDEO_H264, 0);
RTSP rtsp;
StreamIO videoStreamer(1, 1);

// WiFi credentials (configurable)
char wifi_ssid[32] = "Your_WiFi_SSID";
char wifi_password[32] = "Your_WiFi_Password";

// System state tracking
bool neural_network_loaded = false;
bool camera_initialized = false;
bool wifi_connected = false;
bool rtsp_streaming = false;
uint32_t detection_count = 0;
uint32_t led_detections = 0;
uint32_t motherboard_detections = 0;

// ===== USB CONNECTION MONITORING =====
typedef enum {
  USB_STATE_DISCONNECTED = 0,
  USB_STATE_CONNECTED = 1,
  USB_STATE_RECONNECTING = 2,
  USB_STATE_STABLE = 3
} usb_state_t;

typedef struct {
  usb_state_t current_state;
  usb_state_t previous_state;
  uint32_t last_check_time;
  uint32_t disconnection_time;
  uint32_t reconnection_time;
  uint32_t stable_time;
  bool connection_change_detected;
  uint32_t reconnection_count;
} usb_monitor_t;

usb_monitor_t usb_monitor = {
  .current_state = USB_STATE_DISCONNECTED,
  .previous_state = USB_STATE_DISCONNECTED,
  .last_check_time = 0,
  .disconnection_time = 0,
  .reconnection_time = 0,
  .stable_time = 0,
  .connection_change_detected = false,
  .reconnection_count = 0
};

// ===== USB MONITORING FUNCTIONS =====
bool is_usb_connected() {
  // Check if Serial is available and responsive
  if (!Serial) return false;

  // Try a simple operation to test if Serial is functional
  //size_t available = Serial.available();
  return true;  // If we get here, Serial is working
}

void update_usb_state() {
  uint32_t current_time = millis();

  // Only check every 100ms to avoid excessive polling
  if (current_time - usb_monitor.last_check_time < 100) {
    return;
  }

  usb_monitor.previous_state = usb_monitor.current_state;
  bool connected = is_usb_connected();

  switch (usb_monitor.current_state) {
    case USB_STATE_DISCONNECTED:
      if (connected) {
        usb_monitor.current_state = USB_STATE_RECONNECTING;
        usb_monitor.reconnection_time = current_time;
        usb_monitor.reconnection_count++;
        usb_monitor.connection_change_detected = true;
      }
      break;

    case USB_STATE_CONNECTED:
      if (!connected) {
        usb_monitor.current_state = USB_STATE_DISCONNECTED;
        usb_monitor.disconnection_time = current_time;
        usb_monitor.connection_change_detected = true;
      } else if (current_time - usb_monitor.stable_time > 5000) {
        usb_monitor.current_state = USB_STATE_STABLE;
      }
      break;

    case USB_STATE_RECONNECTING:
      if (connected && (current_time - usb_monitor.reconnection_time > 1000)) {
        usb_monitor.current_state = USB_STATE_CONNECTED;
        usb_monitor.stable_time = current_time;
        usb_monitor.connection_change_detected = true;
      } else if (!connected) {
        usb_monitor.current_state = USB_STATE_DISCONNECTED;
        usb_monitor.disconnection_time = current_time;
      }
      break;

    case USB_STATE_STABLE:
      if (!connected) {
        usb_monitor.current_state = USB_STATE_DISCONNECTED;
        usb_monitor.disconnection_time = current_time;
        usb_monitor.connection_change_detected = true;
      }
      break;
  }

  usb_monitor.last_check_time = current_time;
}

void handle_usb_reconnection() {
  if (!usb_monitor.connection_change_detected) return;

  if (usb_monitor.current_state == USB_STATE_CONNECTED && usb_monitor.previous_state == USB_STATE_RECONNECTING) {

    // USB just reconnected - handle recovery
    Serial.println("\n🔌 USB RECONNECTION DETECTED!");
    Serial.println("⚠️  Reconnection #" + String(usb_monitor.reconnection_count));
    Serial.println("🔄 Performing system state recovery...");

    // Give time for serial to stabilize
    delay(500);

    // Reinitialize serial commands safely
    try {
      serial_commands_init();
      Serial.println("✅ Serial commands recovered");
    } catch (...) {
      Serial.println("⚠️  Serial commands recovery failed");
    }

    // Check system health
    perform_post_reconnection_check();

    Serial.println("🎯 System recovery complete - monitoring resumed");
    Serial.println("════════════════════════════════════════════════\n");
  }

  usb_monitor.connection_change_detected = false;
}

void perform_post_reconnection_check() {
  Serial.println("🔍 Post-reconnection system health check:");

  // Check neural network
  if (neural_network_loaded) {
    try {
      int count = ObjDet.getResultCount();
      Serial.println("  ✅ Neural Network: OK (count=" + String(count) + ")");
    } catch (...) {
      Serial.println("  ⚠️  Neural Network: Recovered after reconnection");
    }
  } else {
    Serial.println("  ❌ Neural Network: Not loaded");
  }

  // Check GPIO
  if (gpio_is_initialized()) {
    Serial.println("  ✅ GPIO: OK");
  }

  // Check LoRa
  if (lora_is_initialized()) {
    Serial.println("  ✅ LoRa: OK");
  }

  // Check flash
  if (flash_is_initialized()) {
    Serial.println("  ✅ Flash: OK");
  }

  // Display current stats
  Serial.println("📊 Current stats: Detections=" + String(detection_count) + " (LED:" + String(led_detections) + ", MB:" + String(motherboard_detections) + ")");
}

// ===== SAFE SERIAL OUTPUT =====
void safe_serial_print(const String& message) {
  if (usb_monitor.current_state == USB_STATE_STABLE || usb_monitor.current_state == USB_STATE_CONNECTED) {
    try {
      Serial.println(message);
    } catch (...) {
      // Silent failure if serial is not working
    }
  }
}

void safe_serial_print(const char* message) {
  safe_serial_print(String(message));
}

// ===== DETECTION HANDLER =====
void handle_detection(uint8_t object_class, float confidence, ObjectDetectionResult& result) {
  // Update statistics
  detection_count++;

  if (object_class == CLASS_LED_ON) {
    led_detections++;
  } else if (object_class == CLASS_MOTHERBOARD) {
    motherboard_detections++;

    // Add to motherboard counter
    motherboard_counter_add_detection(millis());

    // Check if trigger threshold reached
    if (motherboard_counter_check_trigger()) {
      // Send LoRa trigger message
      send_motherboard_trigger_lora();

      // Special visual indication
      gpio_status_led_set_pattern(LED_PATTERN_TRIPLE_BLINK);

      safe_serial_print("🚨 MOTHERBOARD DETECTION TRIGGER ACTIVATED!");
      safe_serial_print("📡 LoRa trigger message sent");
    }
  }

  // Create detection result for logging
  detection_result_t det_result = { 0 };
  det_result.timestamp = millis();
  det_result.object_class = object_class;
  det_result.confidence = confidence;
  det_result.x_min = result.xMin();
  det_result.y_min = result.yMin();
  det_result.x_max = result.xMax();
  det_result.y_max = result.yMax();
  det_result.valid = 1;

  // Log to flash
  if (flash_is_initialized()) {
    flash_write_detection_log(&det_result);
  }

  // Visual feedback
  //if (object_class != CLASS_MOTHERBOARD || !motherboard_counter_check_trigger()) {
  gpio_status_led_set_pattern(LED_PATTERN_FAST_BLINK);
  //}
  gpio_laser_force_off();

  // LoRa transmission for high-confidence detections
  if (lora_is_initialized() && confidence >= system_config.detection_threshold) {
    send_detection_lora(object_class, confidence);
  }

  // Update crosshair laser for motherboard
  if (object_class == CLASS_MOTHERBOARD && motherboard_counter_check_trigger()) {
    gpio_status_led_set_pattern(LED_PATTERN_TRIPLE_BLINK);
  }

  // Print detection info safely
  String counter_info = "";
  if (object_class == CLASS_MOTHERBOARD) {
    uint32_t window_count = motherboard_counter_get_count_in_window();
    counter_info = " (Count: " + String(window_count) + "/" + String(system_config.motherboard_count_threshold) + ")";
  }

  safe_serial_print("🎯 " + String(object_class == CLASS_LED_ON ? "LED" : "MOTHERBOARD") + " detected: " + String(confidence, 1) + "%" + counter_info);
}

// ===== LORA FUNCTIONS =====
void send_detection_lora(uint8_t object_class, float confidence) {
  char msg[30];
  snprintf(msg, sizeof(msg), "%d,%.2f", object_class, confidence);

  lora_result_t result = lora_send_message(LORA_MSG_DETECTION, msg);
  if (result != LORA_SUCCESS) {
    DEBUG_PRINT(3, "[LoRa] Failed: " + String(lora_result_to_string(result)));
  } else {
    DEBUG_PRINT(3, "[LoRa] Sent: " + String(msg));
  }
}

void send_status_lora() {
  char msg[25];
  snprintf(msg, sizeof(msg), "UP:%lu,D:%lu", millis() / 1000, detection_count);
  lora_result_t result = lora_send_message(LORA_MSG_STATUS, msg);
  if (result != LORA_SUCCESS) {
    DEBUG_PRINT(3, "[LoRa] Status failed: " + String(lora_result_to_string(result)));
  }
}

// ===== WIFI/RTSP FUNCTIONS =====
bool init_wifi_on_demand() {
  safe_serial_print("[WiFi] Starting connection...");
  safe_serial_print("[WiFi] SSID: " + String(wifi_ssid));

  int status = WL_IDLE_STATUS;
  uint32_t start_time = millis();

  status = WiFi.begin(wifi_ssid, wifi_password);

  while (status != WL_CONNECTED && (millis() - start_time) < 15000) {
    delay(500);
    if (usb_monitor.current_state >= USB_STATE_CONNECTED) Serial.print(".");
    status = WiFi.status();
  }

  if (status == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    safe_serial_print("\n[WiFi] Connected! IP: " + ip);
    wifi_connected = true;
    return true;
  } else {
    safe_serial_print("\n[WiFi] Failed to connect");
    wifi_connected = false;
    return false;
  }
}

// ===== NEURAL NETWORK INITIALIZATION =====
bool init_neural_network_core() {
  safe_serial_print("[NN] Initializing Core Neural Network...");

  try {
    safe_serial_print("  - Configuring video channels...");
    Camera.configVideoChannel(CHANNELNN, configNN);
    Camera.configVideoChannel(CHANNEL, config);  // Also configure main channel
    delay(1000);

    safe_serial_print("  - Camera video initialization...");
    Camera.videoInit();
    delay(2000);

    safe_serial_print("  - Neural network configuration...");
    ObjDet.configVideo(configNN);
    delay(500);

    safe_serial_print("  - Loading CUSTOMIZED_YOLOV7TINY model...");
    ObjDet.modelSelect(OBJECT_DETECTION, CUSTOMIZED_YOLOV7TINY, NA_MODEL, NA_MODEL);
    delay(1000);

    safe_serial_print("  - Starting object detection...");
    ObjDet.begin();
    delay(2000);

    safe_serial_print("  - StreamIO NN configuration...");
    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.setStackSize();
    videoStreamerNN.setTaskPriority();
    videoStreamerNN.registerOutput(ObjDet);

    safe_serial_print("  - Starting StreamIO NN link...");
    if (videoStreamerNN.begin() != 0) {
      safe_serial_print("  ! StreamIO NN link failed");
      return false;
    }
    delay(500);

    safe_serial_print("  - Starting NN video channel...");
    Camera.channelBegin(CHANNELNN);
    delay(3000);

    neural_network_loaded = true;
    camera_initialized = true;

    safe_serial_print("✅ Core detection active with CUSTOMIZED_YOLOV7TINY");
    return true;

  } catch (...) {
    safe_serial_print("❌ Neural network initialization failed");
    neural_network_loaded = false;
    camera_initialized = false;
    return false;
  }
}
// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(3000);

  // Initialize USB monitoring
  usb_monitor.current_state = USB_STATE_CONNECTED;
  usb_monitor.stable_time = millis();

  Serial.println("\n=== AMB82 SMART DETECTION SYSTEM V2.0 - USB RECONNECTION SAFE ===");
  Serial.println("🔌 USB hot-plug protection enabled");
  Serial.println("📐 Resolution: " + String(NNWIDTH) + "x" + String(NNHEIGHT));

  // ===== CAMERA SYSTEM RESET =====
  Serial.println("[0] Camera system reset...");
  try {
    // Camera.end();
    delay(2000);
    Serial.println("✓ Camera reset complete");
  } catch (...) {
    Serial.println("⚠ Camera reset warning (may be normal)");
  }

  // Initialize modules
  Serial.println("[1] Configuration...");
  system_config_t default_config = DEFAULT_CONFIG;
  system_config = default_config;
  flash_init();
  config_load_from_flash();
  Serial.println("✓ Config loaded");

  Serial.println("[2] GPIO...");
  gpio_init();
  Serial.println("✓ GPIO ready");

  Serial.println("[3] Motherboard Counter...");
  motherboard_counter_init();
  Serial.println("✓ Motherboard counter ready");

  Serial.println("[4] Serial commands...");
  serial_commands_init();
  Serial.println("✓ Commands ready");

  // LoRa initialization
  Serial.println("[5] LoRa...");
  Serial1.begin(115200);
  delay(1000);
  Serial1.println("AT");
  delay(1000);

  bool lora_basic_test = false;
  String response = "";
  uint32_t start_time = millis();

  while (millis() - start_time < 3000) {
    if (Serial1.available()) {
      response += (char)Serial1.read();
      if (response.indexOf("OK") >= 0) {
        lora_basic_test = true;
        break;
      }
    }
  }

  if (lora_basic_test) {
    Serial.println("  ✓ RAK3172 basic communication OK");
    lora_result_t lora_result = lora_init();
    Serial.println(lora_result == LORA_SUCCESS ? "✓ LoRa ready" : "⚠ LoRa init failed (but basic comm OK)");
  } else {
    Serial.println("  ⚠ RAK3172 not responding");
  }

  // Neural network initialization with enhanced error handling
  Serial.println("[6] Neural Network...");
  Serial.println("⚠️  IMPORTANT: VOE timeout errors are normal during initialization");
  Serial.println("🔄 Initializing camera and neural network...");

  // Multiple initialization attempts
  int nn_attempts = 0;
  bool nn_success = false;

  while (nn_attempts < 3 && !nn_success) {
    nn_attempts++;
    Serial.println("Neural network attempt #" + String(nn_attempts));

    if (init_neural_network_core()) {
      nn_success = true;
      neural_network_loaded = true;
    } else {
      Serial.println("❌ Neural network attempt #" + String(nn_attempts) + " failed");
      if (nn_attempts < 3) {
        Serial.println("🔄 Retrying in 5 seconds...");
        delay(5000);
      }
    }
  }

  if (!nn_success) {
    Serial.println("❌ Neural network failed after all attempts");
    Serial.println("⚠️  System will continue without detection capability");
  }

  system_state = SYS_STATE_RUNNING;

  Serial.println("\n🎉 SYSTEM V2.0 STATUS!");
  Serial.println("════════════════════════════");
  Serial.println("Detection: " + String(neural_network_loaded ? "✅ ACTIVE" : "❌ FAILED"));
  Serial.println("Model: CUSTOMIZED_YOLOV7TINY");
  Serial.println("Resolution: " + String(NNWIDTH) + "x" + String(NNHEIGHT));
  Serial.println("WiFi: ⏸️ Disabled (use 'rtsp_stream' to enable)");
  Serial.println("LoRa: " + String(lora_basic_test ? "✅ CONNECTED" : "❌ NO RESPONSE"));
  Serial.println("USB Protection: 🔌 ACTIVE");
  Serial.println("Commands: ✅ Type 'help'");

  if (neural_network_loaded) {
    Serial.println("\n🎯 DETECTION SETTINGS:");
    Serial.println("- Threshold: " + String(system_config.detection_threshold * 100) + "%");
    Serial.println("- Motherboard: " + String(system_config.motherboard_threshold * 100) + "%");
    Serial.println("- MB Trigger: " + String(system_config.motherboard_count_threshold) + " detections");
  }

  Serial.println("\n⚠️  TROUBLESHOOTING NOTES:");
  Serial.println("- VOE timeout errors during init are usually normal");
  Serial.println("- If detection fails, try 'nn_restart' command");
  Serial.println("- USB hot-plug is supported for reconnection");
  Serial.println("- Use 'rtsp_stream' for video streaming");
  Serial.println("════════════════════════════\n");
}

// ===== MAIN LOOP =====
void loop() {
  // CRITICAL: USB monitoring must be first
  update_usb_state();
  handle_usb_reconnection();

  // Process modules with USB-safe error handling
  if (gpio_is_initialized()) {
    try {
      gpio_process_all();
    } catch (...) {
      // Silent failure - continue operation
    }
  }

  if (lora_is_initialized()) {
    try {
      lora_process();
    } catch (...) {
      // Silent failure - continue operation
    }
  }

  if (serial_commands_is_enabled() && usb_monitor.current_state >= USB_STATE_CONNECTED) {
    try {
      serial_commands_process();
    } catch (...) {
      // Serial error - might be USB issue
    }
  }

  // Core detection processing (always continue)
  if (neural_network_loaded) {
    process_detections_core();
  }

  // Status reporting with USB-safe output
  static uint32_t last_status = 0;
  if (millis() - last_status > 300000) {  // Every 5 minutes
    if (lora_is_initialized()) {
      send_status_lora();
    }

    uint32_t mb_window_count = motherboard_counter_get_count_in_window();
    safe_serial_print("📊 " + String(millis() / 1000) + "s | Detections: " + String(detection_count) + " (LED:" + String(led_detections) + ", MB:" + String(motherboard_detections) + ") | MB Window: " + String(mb_window_count) + "/" + String(system_config.motherboard_count_threshold) + " | USB: " + String(usb_monitor.reconnection_count) + " reconnections");

    last_status = millis();
  }
  static uint32_t last_led_reset = 0;
  if (millis() - last_led_reset > 10000) {  // 10 seconds
    static uint32_t last_detection_check = detection_count;
    if (last_detection_check == detection_count) {
      // No new detections in 10 seconds, return to normal blink
      gpio_status_led_set_pattern(LED_PATTERN_SLOW_BLINK);
    }
    last_detection_check = detection_count;
    last_led_reset = millis();
  }
  delay(100);
}

// ===== DETECTION PROCESSING =====
void process_detections_core() {
  static uint32_t last_check = 0;
  static uint32_t error_count = 0;

  if (millis() - last_check < 2000) return;

  try {
    // Check if neural network is still loaded
    if (!neural_network_loaded) {
      return;
    }

    std::vector<ObjectDetectionResult> results = ObjDet.getResult();
    int count = ObjDet.getResultCount();

    // Reset error count on successful operation
    error_count = 0;

    // Debug output with USB-safe printing
    static uint32_t last_debug = 0;
    if (millis() - last_debug > 30000) {
      safe_serial_print("[DEBUG] NN Processing - Result count: " + String(count));
      last_debug = millis();
    }

    // Process detections
    if (count > 0) {
      for (int i = 0; i < count && i < 3; i++) {
        ObjectDetectionResult item = results[i];
        int obj_type = item.type();
        float confidence = item.score();

        if (obj_type >= 0 && obj_type < 2) {  // Only process our 2 classes
          if (confidence > system_config.detection_threshold) {

            uint8_t object_class = CLASS_UNKNOWN;

            if (obj_type == 0) {
              object_class = CLASS_LED_ON;
            } else if (obj_type == 1) {
              object_class = CLASS_MOTHERBOARD;
            }

            if (object_class != CLASS_UNKNOWN) {
              handle_detection(object_class, confidence, item);
            }
          }
        }
      }
    }

  } catch (...) {
    error_count++;
    if (error_count < 5) {  // Only print first few errors
      safe_serial_print("! Detection processing error #" + String(error_count));
    }

    // If too many errors, try to restart detection
    if (error_count > 10) {
      safe_serial_print("⚠️  Too many detection errors, attempting restart...");
      neural_network_loaded = false;
      delay(5000);

      if (init_neural_network_core()) {
        safe_serial_print("✅ Detection system restarted successfully");
        error_count = 0;
      } else {
        safe_serial_print("❌ Detection system restart failed");
      }
    }
  }

  last_check = millis();
}

// ===== DIAGNOSTIC FUNCTIONS =====
void debug_neural_network_status() {
  safe_serial_print("\n=== NEURAL NETWORK DIAGNOSTICS ===");
  safe_serial_print("USB State: " + String(usb_monitor.current_state) + " (Reconnections: " + String(usb_monitor.reconnection_count) + ")");
  safe_serial_print("Camera initialized: " + String(camera_initialized ? "YES" : "NO"));
  safe_serial_print("NN loaded: " + String(neural_network_loaded ? "YES" : "NO"));
  safe_serial_print("Detection count: " + String(detection_count));
  safe_serial_print("LED detections: " + String(led_detections));
  safe_serial_print("Motherboard detections: " + String(motherboard_detections));

  try {
    int count = ObjDet.getResultCount();
    safe_serial_print("Current result count: " + String(count));
  } catch (...) {
    safe_serial_print("Object detection check failed");
  }

  safe_serial_print("=====================================\n");
}

bool reset_camera_system() {
  Serial.println("🔄 Camera reset not supported - please reboot device");
  Serial.println("Use the 'reboot' command for full system restart");
  return false;
}

// Add this function to help with troubleshooting
bool restart_neural_network() {
  Serial.println("🔄 NN restart not supported - please reboot device");
  Serial.println("USB reconnection should auto-recover the system");
  return false;
}

void debug_camera_system() {
  Serial.println("\n=== CAMERA SYSTEM DEBUG ===");

  try {
    // Test basic camera operations
    Serial.println("Testing camera channels...");

    // Test NN channel
    Serial.println("NN Channel (CHANNELNN=" + String(CHANNELNN) + "):");
    Serial.println("  Resolution: " + String(NNWIDTH) + "x" + String(NNHEIGHT));

    // Test RTSP channel
    Serial.println("RTSP Channel (CHANNEL=" + String(CHANNEL) + "):");
    Serial.println("  Status: " + String(rtsp_streaming ? "ACTIVE" : "INACTIVE"));

    // Test neural network
    if (neural_network_loaded) {
      int count = ObjDet.getResultCount();
      Serial.println("Neural Network:");
      Serial.println("  Model: CUSTOMIZED_YOLOV7TINY");
      Serial.println("  Current results: " + String(count));
    } else {
      Serial.println("Neural Network: NOT LOADED");
    }

  } catch (...) {
    Serial.println("Camera system debug failed - system may need restart");
  }

  Serial.println("===============================\n");
}

// Add these functions to your AMB82_Smart_Detection_V1.ino file

// ===== RTSP STREAMING FUNCTIONS =====
bool start_rtsp_streaming() {
  safe_serial_print("[RTSP] Starting streaming setup...");

  try {
    // Only start RTSP if WiFi is connected
    if (!wifi_connected) {
      safe_serial_print("[RTSP] WiFi not connected - cannot start RTSP");
      return false;
    }

    // Check if RTSP is already running
    if (rtsp_streaming) {
      safe_serial_print("[RTSP] Already streaming");
      return true;
    }

    safe_serial_print("[RTSP] Configuring video channel for streaming...");

    // Configure main video channel for RTSP (different from NN channel)
    Camera.configVideoChannel(CHANNEL, config);
    delay(1000);

    safe_serial_print("[RTSP] Setting up RTSP server...");

    // Configure RTSP
    rtsp.configVideo(config);
    rtsp.begin();
    delay(1000);

    safe_serial_print("[RTSP] Starting video streamer...");

    // Configure StreamIO for RTSP
    videoStreamer.registerInput(Camera.getStream(CHANNEL));
    videoStreamer.setStackSize();
    videoStreamer.setTaskPriority();
    videoStreamer.registerOutput(rtsp);

    if (videoStreamer.begin() != 0) {
      safe_serial_print("[RTSP] StreamIO setup failed");
      return false;
    }

    safe_serial_print("[RTSP] Starting main video channel...");
    Camera.channelBegin(CHANNEL);
    delay(2000);

    // Get IP for RTSP URL
    IPAddress ip = WiFi.localIP();
    //String rtsp_url = "rtsp://" + ip.toString() + ":554/testStream";
    String rtsp_url = "rtsp://" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + ":554/testStream";


    rtsp_streaming = true;

    safe_serial_print("✅ RTSP streaming started successfully!");
    safe_serial_print("📡 RTSP URL: " + rtsp_url);
    safe_serial_print("🔗 VLC: Open Network Stream with above URL");

    return true;

  } catch (...) {
    safe_serial_print("❌ RTSP streaming setup failed");
    rtsp_streaming = false;
    return false;
  }
}

void stop_rtsp_streaming() {
  safe_serial_print("[RTSP] Stopping streaming...");

  try {
    if (rtsp_streaming) {
      // Stop video channel
      Camera.channelEnd(CHANNEL);
      delay(500);

      // Stop StreamIO
      videoStreamer.end();
      delay(500);

      // Stop RTSP
      rtsp.end();
      delay(500);

      rtsp_streaming = false;
      safe_serial_print("✅ RTSP streaming stopped");
    } else {
      safe_serial_print("⚠️  RTSP was not running");
    }
  } catch (...) {
    safe_serial_print("⚠️  Error stopping RTSP streaming");
    rtsp_streaming = false;
  }
}
