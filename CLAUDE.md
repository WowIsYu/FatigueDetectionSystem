# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Fatigue Detection System (疲劳驾驶监测系统) - A real-time driver fatigue detection system using YOLOv12 deep learning model, built with C++ and Qt6. Supports multiple video sources (images, video files, USB cameras, IP cameras) and stores detection results in SQLite database.

## Build System

This project uses **CMake 3.16+** with **C++17** standard.

### Dependencies

The build system requires manual path configuration for two key dependencies in `CMakeLists.txt`:

1. **OpenCV 4.12.0**: Set `OPENCV_ROOT_PATH` (currently: `D:/dependency/opencv-4.12.0/opencv/build`)
   - Debug library: `opencv_world4120d.dll`
   - Release library: `opencv_world4120.dll`

2. **ONNX Runtime 1.23.0**: Set `ONNXRUNTIME_ROOT_PATH` (currently: `D:/dependency/onnxruntime-win-x64-1.23.0`)

3. **Qt6** (Widgets, Sql, Multimedia, Network modules)

### Build Commands

**Debug build:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

**Release build:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Windows with Visual Studio:**
```bash
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Post-build

CMake automatically copies required runtime files to the executable directory:
- `models/` directory (contains ONNX model files)
- `onnxruntime.dll`
- OpenCV DLLs (debug/release versions)

## Architecture

### Core Component Structure

The system follows a modular architecture with clear separation of concerns:

**MainWindow** (`src/mainwindow.h/cpp`)
- Central orchestrator managing all UI interactions and component lifecycle
- Owns instances of all core components (DatabaseManager, DetectionEngine, VideoProcessor, Config)
- Handles four detection modes: image, video file, USB camera, IP camera
- Uses QTimer for frame-by-frame processing in video modes
- Implements detection throttling: saves results only when confidence exceeds threshold and respects save intervals

**DetectionEngine** (`src/core/DetectionEngine.h/cpp`)
- Wraps ONNX Runtime for YOLOv12 model inference
- Manages model loading, preprocessing (letterbox resizing), and postprocessing (NMS)
- Configurable confidence and NMS thresholds
- Class names: Eyes Closed, Yawning, Phone Usage, Distracted, Normal

**VideoProcessor** (`src/core/VideoProcessor.h/cpp`)
- **Threading model**: Uses QThread with worker object (VideoProcessorWorker)
- Worker runs in separate thread, emits `frameReady(cv::Mat)` signals to main thread
- Supports three video sources via cv::VideoCapture:
  - File path (video files)
  - Device ID (USB cameras)
  - URL string (IP cameras via RTSP/HTTP)
- Thread-safe start/stop operations

**DatabaseManager** (`src/core/DatabaseManager.h/cpp`)
- Qt SQL wrapper for SQLite database operations
- Stores detection records: timestamp, detection type, confidence
- Provides statistics and time-range queries
- Database schema created on first initialization

**Config** (`src/utils/Config.h/cpp`)
- JSON-based configuration persistence
- Default model path: `models/best_by_v12n_1W.onnx`
- Stores model path, database path, thresholds, and save intervals

### Threading and Concurrency

- **Main thread**: Qt UI event loop, frame display, detection result updates
- **VideoProcessor worker thread**: Video frame capture and emission
- **Synchronization**: Qt signals/slots (queued connections between threads)
- Detection inference runs on main thread when processing frames

### Data Flow

1. Video source → VideoProcessor worker thread → `frameReady()` signal
2. MainWindow receives frame → DetectionEngine.detect() → Results
3. If detection confidence/interval conditions met → DatabaseManager saves
4. Frame + detection boxes rendered → QLabel display

## Model Files

ONNX model expected in `models/` directory (copied to build tree post-build):
- Default: `best_by_v12n_1W.onnx`
- Model must be YOLO format with compatible input/output shapes
- Model path configurable via Settings dialog

## Detection Classes

The system detects 5 fatigue-related states:
0. Eyes Closed
1. Yawning
2. Phone Usage
3. Distracted
4. Normal

## Key Implementation Details

**Detection throttling** (MainWindow):
- `m_lastDetection`: Tracks last detection confidence per class
- `m_consecutiveCount`: Counts consecutive detections for stability
- `m_lastSaveTime`: Prevents duplicate saves within interval (default: 1000ms)
- Only saves when `confidence > m_confidenceThreshold` (default: 0.6)

**IP Camera reconnection**:
- Stores address in `m_ipCameraAddress`
- `reconnectIPCamera()` closes and reopens connection

**Platform-specific code**:
- Windows: `currCpu()` uses `GetCurrentProcessorNumber()` for debugging
- Desktop OpenGL forced via `Qt::AA_UseDesktopOpenGL`

## Configuration Files

- `config.json`: Runtime configuration (created on first run)
- `detection_results.db`: SQLite database for detection records

## Common Modifications

**To add new detection classes**:
1. Update model training with new classes
2. Modify `DetectionEngine::initClassNames()` to include new class names
3. Update UI result labels if needed

**To change model**:
1. Place ONNX file in `models/` directory
2. Use Settings dialog or modify `Config::DEFAULT_MODEL_PATH`

**To adjust detection sensitivity**:
- Confidence threshold: `Config::setConfidenceThreshold()`
- NMS threshold: `Config::setNMSThreshold()`
- Save interval: `Config::setSaveInterval()`

## Important Notes

- All file paths in CMakeLists.txt use Windows-style paths - adjust for Linux/macOS
- OpenCV VideoCapture device IDs start at 0 (typically default camera)
- RTSP URL format: `rtsp://username:password@ip:port/stream`
- Model input size determined from ONNX model metadata during loading
