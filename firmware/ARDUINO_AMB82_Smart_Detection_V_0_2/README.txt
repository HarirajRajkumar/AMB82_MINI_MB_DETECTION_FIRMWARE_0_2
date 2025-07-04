# AMB82 Smart Detection System V0.2.0

A comprehensive IoT object detection system built on the Realtek AMB82-MINI platform featuring real-time neural network inference, LoRa communication, and advanced GPIO control capabilities.

## Features

### Core Detection System
- **YOLOv7 Neural Network**: Custom-trained YOLOV7TINY model for LED and motherboard detection
- **Real-time Processing**: 576x320 resolution at 10 FPS for optimal performance
- **USB Reconnection Safe**: Automatic system recovery after USB disconnection/reconnection
- **Detection Logging**: Persistent storage of detection events in flash memory

### Communication Systems
- **LoRa Connectivity**: RAK3172 module integration for long-range wireless communication
- **WiFi/RTSP Streaming**: Optional video streaming for remote monitoring
- **Serial Command Interface**: Comprehensive command system for configuration and control

### GPIO Control Systems
- **Intelligent Fan Control**: Automated 3-minute cycle control with statistics tracking
- **Crosshair Laser**: Smart targeting system that responds to detection events
- **Status LED**: Multi-pattern indication system for different operational states
- **System Reset Control**: Hardware-level reset capabilities

### Advanced Features
- **Motherboard Counter**: Configurable detection threshold system with automatic LoRa triggers
- **Flash Memory Management**: Configuration persistence and detection log storage
- **System Health Monitoring**: Comprehensive diagnostics and status reporting
- **Power Management**: Optimized for continuous operation

## Hardware Requirements

### Primary Components
- **Realtek AMB82-MINI Development Board**
- **RAK3172 LoRa Module**
- **Camera Module** (compatible with AMB82-MINI)

### Optional Components
- **Crosshair Laser Module**
- **Fan Control MOSFET**
- **External Status LED**
- **Reset Control Circuit**

### Pin Configuration
```
PIN_FAN                = 10    // Fan control output
PIN_CROSSHAIR_LASER    = 9     // Laser control output
PIN_RESET_CONTROL      = 7     // System reset output
PIN_STATUS_LED         = LED_BUILTIN // Status indication

Serial1_TX             = 21    // LoRa module communication
Serial1_RX             = 22    // LoRa module communication
```

## Installation

### 1. Arduino IDE Setup
```bash
# Install Realtek AMB82 board package
# Board Manager URL: https://github.com/ambiot/ambd_arduino/raw/dev/Arduino_package/package_realtek.com_amebapro2_index.json
# Select Board: AMB82-MINI
```

### 2. Required Libraries
The following libraries are included with the AMB82 board package:
- `VideoStream.h`
- `NNObjectDetection.h`
- `FlashMemory.h`
- `WiFi.h`
- `RTSP.h`

### 3. Model Installation
Place the custom YOLOv7 model files in the appropriate neural network directory following the AMB82 documentation.

### 4. Hardware Connections

#### LoRa Module (RAK3172)
```
RAK3172 TX  -> AMB82 Serial1_RX (Pin 22)
RAK3172 RX  -> AMB82 Serial1_TX (Pin 21)
RAK3172 VCC -> AMB82 3.3V
RAK3172 GND -> AMB82 GND
```

#### GPIO Devices
```
Fan Control    -> Pin 10 (via MOSFET)
Crosshair Laser -> Pin 9
Reset Control  -> Pin 7
Status LED     -> Built-in LED
```

## Configuration

### Default Settings
```cpp
Detection Threshold:      70%
Motherboard Threshold:    60%
Fan Cycle Interval:       3 minutes
Laser Blink Interval:     500ms
LoRa Send Interval:       30 seconds
Motherboard Count Trigger: 50 detections in 10 seconds
```

### Configuration Commands
The system supports real-time configuration via serial commands:

```bash
# Detection Settings
set detection_threshold 0.7          # Set detection confidence threshold
set motherboard_threshold 0.6        # Set motherboard-specific threshold

# GPIO Settings
set fan_enabled 1                    # Enable/disable fan control
set crosshair_enabled 1              # Enable/disable crosshair laser
set fan_cycle_interval 180           # Fan cycle time in seconds

# Motherboard Counter
set mb_count_enabled 1               # Enable motherboard counting
set mb_count_threshold 50            # Detections needed for trigger
set mb_count_window 10               # Time window in seconds

# LoRa Settings
set lora_interval 30                 # LoRa transmission interval

# Save configuration
save                                 # Persist settings to flash memory
```

## Usage

### Basic Operation

1. **Power On**: Connect USB cable to AMB82-MINI
2. **Monitor**: Open Serial Monitor at 115200 baud
3. **Wait for Initialization**: System will automatically initialize all components
4. **Detection Active**: System begins real-time object detection

### Serial Commands

#### System Commands
```bash
help                    # Display all available commands
status                  # Show comprehensive system status
reboot                  # Restart the system
reset_system           # Hardware system reset
```

#### Detection Commands
```bash
detection              # Show detection statistics
nn_status              # Neural network diagnostic information
mb_counter             # Motherboard counter statistics
mb_reset               # Reset motherboard counter
```

#### Communication Commands
```bash
lora                   # LoRa module status
lora stats             # Detailed LoRa statistics
lora test              # Send test LoRa message
lora diag              # Complete LoRa diagnostics
```

#### WiFi/Streaming Commands
```bash
set_wifi <ssid> <password>   # Configure WiFi credentials
rtsp_stream                  # Start WiFi and RTSP streaming
rtsp_stop                    # Stop RTSP streaming
```

#### Data Management Commands
```bash
logs 20                # Display last 20 detection logs
clear_logs             # Clear all detection logs
flash                  # Show flash memory status
save                   # Save current configuration
reset                  # Reset to default configuration
```

### LoRa Communication

#### Message Types
- **Detection Messages**: Real-time detection events
- **Status Updates**: Periodic system health reports
- **Trigger Alerts**: Motherboard detection threshold events
- **Heartbeat**: System alive confirmation

#### Message Format
```
Detection: "D,L,85" (LED detected, 85% confidence)
Detection: "D,M,92" (Motherboard detected, 92% confidence)
Status: "S,3600,150" (3600s uptime, 150 total detections)
Trigger: "MT,52,50,10,3600" (52 detections, threshold 50, 10s window, at 3600s)
```

## System Behavior

### Detection Response
1. **Object Detected**: Confidence above threshold triggers event
2. **Status LED**: Switches to fast blink pattern (heartbeat)
3. **Crosshair Laser**: Turns OFF immediately
4. **LoRa Transmission**: Sends detection data
5. **Flash Logging**: Stores detection record
6. **LED Reset**: Returns to slow blink after 10 seconds of no detection

### Motherboard Counter Trigger
1. **Counting**: Tracks motherboard detections in sliding time window
2. **Threshold Reached**: Sends special LoRa trigger message
3. **Visual Indication**: Status LED shows triple-blink pattern
4. **Cooldown**: 30-second minimum between triggers

### USB Reconnection Handling
1. **Connection Monitoring**: Continuous USB state tracking
2. **Disconnection Detection**: Automatic state preservation
3. **Reconnection Recovery**: System health check and restoration
4. **Serial Recovery**: Command interface restoration

## Troubleshooting

### Common Issues

#### Neural Network Initialization Failure
```bash
# Symptoms: "Neural network failed to initialize"
# Solutions:
1. Check camera module connection
2. Verify model files are properly installed
3. Try: nn_restart command
4. Power cycle the device
```

#### LoRa Communication Problems
```bash
# Symptoms: "RAK3172 not responding"
# Solutions:
1. Check wiring (pins 21, 22, VCC, GND)
2. Verify 3.3V power supply
3. Try: lora diag command
4. Check baud rate (115200)
```

#### Detection Not Working
```bash
# Symptoms: No detection events
# Solutions:
1. Verify detection thresholds: get detection_threshold
2. Check object visibility and lighting
3. Monitor with: nn_status command
4. Adjust threshold: set detection_threshold 0.5
```

#### Flash Memory Issues
```bash
# Symptoms: Configuration not saving
# Solutions:
1. Check flash status: flash command
2. Try: save command multiple times
3. Reset to defaults: reset command
```

### Debug Commands
```bash
nn_status              # Neural network diagnostics
gpio                   # GPIO system status
lora diag              # LoRa comprehensive test
flash                  # Flash memory status
test                   # Basic system test
```

## Technical Specifications

### Performance Metrics
- **Detection Latency**: <100ms per frame
- **Processing Resolution**: 576x320 pixels
- **Detection Accuracy**: >90% for trained objects
- **LoRa Range**: Up to 10km (line of sight)
- **Power Consumption**: <500mA @ 5V typical

### Memory Usage
- **Model Size**: ~12MB (YOLOV7TINY)
- **Flash Storage**: 4KB configuration + detection logs
- **RAM Usage**: ~200KB during operation

### Communication Specifications
- **LoRa Frequency**: Configurable (US915/EU868/AS923)
- **LoRa Data Rate**: Variable (0.3-50 kbps)
- **WiFi Standards**: 802.11 b/g/n
- **RTSP Stream**: H.264, up to 1080p

## Development

### Custom Model Training
The system supports custom YOLOV7 models. Follow the AMB82 neural network documentation for model conversion and deployment.

### Extending Functionality
The modular architecture allows easy extension:
- Additional GPIO controls in `amb82_gpio.cpp`
- New detection classes in `ObjectClassList.h`
- Custom LoRa message types in `lora_rak3172.cpp`
- Enhanced serial commands in `serial_commands.cpp`

## License

This project is released under the MIT License. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## Support

For technical support and questions:
- Create an issue in this repository
- Check the AMB82-MINI documentation
- Review the troubleshooting section above

## Acknowledgments

- Realtek for the AMB82-MINI platform
- RAKWireless for the RAK3172 LoRa module
- The YOLO development team for the detection algorithms

---

**Version**: 2.0.0  
**Last Updated**: 2025  
**Compatibility**: AMB82-MINI with Arduino IDE