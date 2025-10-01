# 疲劳驾驶监测系统 (C++ Qt版本)

## 项目简介

基于YOLOv12深度学习模型的疲劳驾驶实时监测系统，使用C++和Qt框架开发，支持多种视频输入源，实时检测驾驶员疲劳状态。

## 功能特性

- ✅ 图片检测：支持静态图片的疲劳状态检测
- ✅ 视频文件检测：支持MP4、AVI等格式视频文件
- ✅ 实时摄像头检测：支持本地USB摄像头
- ✅ 网络摄像头：支持RTSP/HTTP协议的IP摄像头
- ✅ 检测记录：SQLite数据库存储检测结果
- ✅ 数据分析：历史检测数据查看与统计
- ✅ 模型配置：支持动态切换ONNX模型

## 系统要求

- C++17或更高版本
- Qt 6.0+
- OpenCV 4.5+
- ONNXRuntime 1.12+
- SQLite3
- CMake 3.16+

## 依赖安装

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    qt6-base-dev \
    qt6-multimedia-dev \
    libopencv-dev \
    libsqlite3-dev \
    cmake \
    build-essential

# 安装ONNXRuntime
wget https://github.com/microsoft/onnxruntime/releases/download/v1.12.0/onnxruntime-linux-x64-1.12.0.tgz
tar -xzf onnxruntime-linux-x64-1.12.0.tgz
sudo cp -r onnxruntime-linux-x64-1.12.0/* /usr/local/
```

### Windows (使用vcpkg)

```powershell
vcpkg install qt6:x64-windows
vcpkg install opencv4:x64-windows
vcpkg install sqlite3:x64-windows
vcpkg install onnxruntime:x64-windows
```

### macOS

```bash
brew install qt6
brew install opencv
brew install sqlite
brew install onnxruntime
```

## 编译构建

### Linux/macOS

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows (Visual Studio)

```powershell
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## 使用说明

### 启动程序

```bash
./FatigueDrivingMonitor
```

### 功能使用

#### 1. 图片检测

- 点击"选择文件"按钮选择图片
- 点击"开始检测"进行检测
- 检测结果会显示在右侧

#### 2. 视频文件检测

- 选择视频文件
- 点击"开始检测"开始实时分析
- 点击"关闭检测"停止

#### 3. 实时摄像头

- 点击"开启摄像头"启动本地摄像头
- 系统自动进行实时检测
- 点击"关闭摄像头"停止

#### 4. 网络摄像头

- 点击"连接摄像头"
- 输入IP摄像头地址（格式：rtsp://用户名:密码@IP:端口）
- 系统自动连接并检测

#### 5. 查看检测记录

- 点击右上角"检测记录"按钮
- 查看历史检测数据
- 支持数据导出

### 配置说明

#### 模型配置

- 点击"设置"按钮
- 选择ONNX模型文件
- 支持YOLOv5/v7/v8/v12模型

#### 数据库配置

- 默认使用SQLite数据库
- 数据库文件：detection_results.db
- 可在设置中更改路径

## 检测类别

系统可检测以下疲劳驾驶相关状态：

- 闭眼 (Eyes Closed)
- 打哈欠 (Yawning)
- 使用手机 (Phone Usage)
- 分心驾驶 (Distracted)
- 正常驾驶 (Normal)

## 性能优化

### 推理优化

- 使用ONNXRuntime加速推理
- 支持GPU加速（需CUDA支持）
- 多线程视频处理

### 内存优化

- 智能帧缓存
- 动态内存管理
- 资源自动释放

## 故障排除

### 摄像头无法打开

- 检查摄像头权限
- 确认摄像头未被其他程序占用
- 尝试重新插拔USB摄像头

### 模型加载失败

- 确认模型文件为ONNX格式
- 检查模型文件路径是否正确
- 验证ONNXRuntime版本兼容性

### 网络摄像头连接失败

- 检查网络连接
- 确认IP地址和端口正确
- 验证用户名密码

## 开发说明

### 项目结构

```
src/
├── core/           # 核心功能模块
├── ui/             # 界面相关
└── utils/          # 工具类
```

### 主要类说明

- `MainWindow`: 主窗口类
- `DetectionEngine`: YOLO检测引擎
- `VideoProcessor`: 视频处理器
- `DatabaseManager`: 数据库管理
- `SettingsDialog`: 设置对话框

## 许可证

MIT License

## 贡献指南

欢迎提交Issue和Pull Request

## 联系方式

如有问题，请提交Issue或联系开发者

## 更新日志

### v1.0.0 (2024-01)

- 初始版本发布
- 实现基础检测功能
- 支持多种视频源
- 数据库记录功能