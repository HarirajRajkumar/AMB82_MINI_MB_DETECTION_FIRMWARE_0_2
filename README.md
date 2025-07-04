# DETECTRA_EXCEL_

An embedded computer vision project for real-time detection of LED states and motherboard-level presence using lightweight models and edge-optimized firmware.

## -------------- Contents -------------- 

- `firmware/` – Codebase for embedded platforms (AMB82-MINI)
- `model_training/` – Trained models and pipelines (YOLOv7-tiny)
    - `datasets/` – Lab Motherboard/LED_ON YOLO datasets
    - `scripts/` – Training and evaluation utilities
    - `docs/` – Diagrams and architecture
- `hardware/` – Schematics and reference sensor, peripherals datasheets

## -------------- Getting Started --------------

1. Flash firmware following instructions in `firmware/build_instructions.md`
2. Load trained model into `/models/trained/`
3. Run `scripts/evaluate.py` for testing

## -------------- Detection Focus --------------

- LED_ON state detection
- Motherboard component presence detection

## -------------- Built With --------------

- YOLOV7 | AMB82 | ARDUINO