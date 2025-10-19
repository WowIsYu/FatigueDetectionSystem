#include "mainwindow.h"
#include "core/DatabaseManager.h"
#include "core/DetectionEngine.h"
#include "core/VideoProcessor.h"
#include "ui/SettingsDialog.h"
#include "ui/DetectionRecordDialog.h"
#include "utils/Config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QCloseEvent>

// mainwindow.cpp
// #include <QTimer>
// #ifdef Q_OS_LINUX
// #include <sys/syscall.h>
// static int cpuNow() { int c; syscall(SYS_getcpu,&c,nullptr,nullptr); return c; }
// #endif
// #ifdef Q_OS_WIN
// #include <windows.h>
// static int cpuNow() { return static_cast<int>(GetCurrentProcessorNumber()); }
// #endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_animationTimer(new QTimer(this))
    , m_rotationAngle(0)
    , m_lastFrameTime(0)
    , m_frameCount(0)
    , m_currentFPS(0.0)
{
    setWindowTitle("守护驶途");
    setMinimumSize(1200, 800);

    // 初始化核心组件
    m_config = std::make_unique<Config>();
    loadConfig();

    m_dbManager = std::make_unique<DatabaseManager>(m_config->getDatabasePath());
    qDebug() << "--------------";
    m_detectionEngine = std::make_unique<DetectionEngine>();
     qDebug() << "--------------";
    m_videoProcessor = std::make_unique<VideoProcessor>();

    // 加载模型
    if (!m_currentModelPath.isEmpty()) {
        m_detectionEngine->loadModel(m_currentModelPath.toStdString());
        qDebug() << "loadModel returned"
                 << m_detectionEngine->loadModel(m_currentModelPath.toStdString());
    }

    // 配置VideoProcessor
    m_videoProcessor->setDetectionEngine(m_detectionEngine.get());
    m_videoProcessor->setDatabaseManager(m_dbManager.get());
    m_videoProcessor->setDisplaySize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    m_videoProcessor->setEnableDetection(true);

    // 设置UI
    setupUI();
    setupConnections();

    // 设置性能监测定时器 (每50ms更新一次动画，20fps)
    m_animationTimer->setInterval(50);
    connect(m_animationTimer, &QTimer::timeout, this, &MainWindow::updatePerformanceIndicator);
    m_animationTimer->start(); // 始终运行

    // 1 秒刷一次
    // auto *timer = new QTimer(this);
    // connect(timer, &QTimer::timeout, [](){
    //     qDebug() << "Main thread now on CPU" << cpuNow();
    // });
    // timer->start(1000);
}

MainWindow::~MainWindow()
{
    stopCamera();
    stopIPCamera();
    stopVideoDetection();
    saveConfig();
}

void MainWindow::setupUI()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建左侧面板
    createLeftPanel();
    m_leftPanel->setFixedWidth(LEFT_PANEL_WIDTH);
    mainLayout->addWidget(m_leftPanel);

    // 创建右侧面板
    createRightPanel();
    mainLayout->addWidget(m_rightPanel);

    // 创建设置按钮
    createSettingsButton();
}

void MainWindow::createLeftPanel()
{
    m_leftPanel = new QWidget(this);
    m_leftPanel->setStyleSheet(R"(
        QWidget {
            background-color: #2C3E50;
            color: white;
        }
        QPushButton {
            background-color: #3498DB;
            border: none;
            padding: 5px;
            min-height: 30px;
            border-radius: 3px;
            margin: 2px 5px;
            color: white;
        }
        QPushButton:hover {
            background-color: #2980B9;
        }
        QGroupBox {
            border: 1px solid #34495E;
            margin-top: 10px;
            margin-left: 5px;
            margin-right: 5px;
            padding-top: 15px;
        }
        QGroupBox::title {
            color: white;
            margin-left: 8px;
        }
        QLabel {
            margin-left: 5px;
            margin-right: 5px;
        }
    )");

    auto* layout = new QVBoxLayout(m_leftPanel);
    layout->setContentsMargins(0, 10, 0, 10);

    // 功能选择标题
    auto* titleLabel = new QLabel("功能选择", m_leftPanel);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // 图片检测组
    auto* imageGroup = new QGroupBox("图片检测", m_leftPanel);
    auto* imageLayout = new QVBoxLayout(imageGroup);
    m_imagepathLabel = new QLabel("选择文件", imageGroup);
    m_imageSelectBtn = new QPushButton("选择文件", imageGroup);
    m_imageStartBtn = new QPushButton("开始检测", imageGroup);
    m_imageStopBtn = new QPushButton("关闭检测", imageGroup);

    imageLayout->addWidget(m_imagepathLabel);
    imageLayout->addWidget(m_imageSelectBtn);
    imageLayout->addWidget(m_imageStartBtn);
    imageLayout->addWidget(m_imageStopBtn);
    layout->addWidget(imageGroup);

    // 视频文件检测组
    auto* videoGroup = new QGroupBox("视频文件检测", m_leftPanel);
    auto* videoLayout = new QVBoxLayout(videoGroup);
    m_videoPathLabel = new QLabel("选择文件", videoGroup);
    m_videoSelectBtn = new QPushButton("选择文件", videoGroup);
    m_videoStartBtn = new QPushButton("开始检测", videoGroup);
    m_videoStopBtn = new QPushButton("关闭检测", videoGroup);

    videoLayout->addWidget(m_videoPathLabel);
    videoLayout->addWidget(m_videoSelectBtn);
    videoLayout->addWidget(m_videoStartBtn);
    videoLayout->addWidget(m_videoStopBtn);
    layout->addWidget(videoGroup);

    // 实时视频检测组
    auto* realtimeGroup = new QGroupBox("实时视频检测", m_leftPanel);
    auto* realtimeLayout = new QVBoxLayout(realtimeGroup);
    m_cameraStartBtn = new QPushButton("开启摄像头", realtimeGroup);
    m_cameraStopBtn = new QPushButton("关闭摄像头", realtimeGroup);

    realtimeLayout->addWidget(m_cameraStartBtn);
    realtimeLayout->addWidget(m_cameraStopBtn);
    layout->addWidget(realtimeGroup);

    // 网络摄像头检测组
    auto* ipcameraGroup = new QGroupBox("网络摄像头检测", m_leftPanel);
    auto* ipcameraLayout = new QVBoxLayout(ipcameraGroup);
    m_ipcameraStartBtn = new QPushButton("连接摄像头", ipcameraGroup);
    m_ipcameraStopBtn = new QPushButton("断开连接", ipcameraGroup);

    ipcameraLayout->addWidget(m_ipcameraStartBtn);
    ipcameraLayout->addWidget(m_ipcameraStopBtn);
    layout->addWidget(ipcameraGroup);

    // 性能监测组
    auto* performanceGroup = new QGroupBox("性能监测", m_leftPanel);
    auto* performanceLayout = new QVBoxLayout(performanceGroup);

    m_performanceLabel = new QLabel("●", performanceGroup);
    m_performanceLabel->setAlignment(Qt::AlignCenter);
    m_performanceLabel->setStyleSheet(R"(
        QLabel {
            font-size: 40px;
            color: #2ECC71;
            margin: 10px;
        }
    )");

    m_fpsLabel = new QLabel("UI FPS: 0.0", performanceGroup);
    m_fpsLabel->setAlignment(Qt::AlignCenter);
    m_fpsLabel->setStyleSheet(R"(
        QLabel {
            font-size: 14px;
            color: #ECF0F1;
            margin: 5px;
        }
    )");

    performanceLayout->addWidget(m_performanceLabel);
    performanceLayout->addWidget(m_fpsLabel);
    layout->addWidget(performanceGroup);

    layout->addStretch();
}

void MainWindow::createRightPanel()
{
    m_rightPanel = new QWidget(this);
    auto* layout = new QVBoxLayout(m_rightPanel);
    layout->setContentsMargins(10, 10, 10, 10);

    // 显示标题
    auto* title = new QLabel("基于YOLOv12的疲劳驾驶监测系统", m_rightPanel);
    title->setStyleSheet(R"(
        font-size: 24px;
        font-weight: bold;
        color: #2C3E50;
        padding: 20px;
    )");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // 图像显示区域
    m_imageLabel = new QLabel(m_rightPanel);
    m_imageLabel->setFixedSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    m_imageLabel->setStyleSheet(R"(
        QLabel {
            border: 2px solid #BDC3C7;
            background-color: #ECF0F1;
        }
    )");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText("等待检测...");

    // 创建容器居中显示图像标签
    auto* imageContainer = new QWidget(m_rightPanel);
    auto* imageContainerLayout = new QHBoxLayout(imageContainer);
    imageContainerLayout->addWidget(m_imageLabel);
    imageContainerLayout->setAlignment(Qt::AlignCenter);
    layout->addWidget(imageContainer);

    // 检测结果表格
    auto* resultsGroup = new QGroupBox("检测结果", m_rightPanel);
    resultsGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 14px;
            border: 1px solid #BDC3C7;
            margin-top: 10px;
        }
        QGroupBox::title {
            color: #2C3E50;
        }
    )");
    auto* resultsLayout = new QVBoxLayout(resultsGroup);

    m_resultLabel = new QLabel("检测结果将在这里显示", resultsGroup);
    m_resultLabel->setAlignment(Qt::AlignCenter);
    resultsLayout->addWidget(m_resultLabel);

    layout->addWidget(resultsGroup);
}

void MainWindow::createSettingsButton()
{
    auto* buttonContainer = new QWidget(this);
    auto* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);

    // 创建设置按钮
    m_settingsBtn = new QPushButton("设置", buttonContainer);
    m_settingsBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498DB;
            color: white;
            border: none;
            padding: 5px 10px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: #2980B9;
        }
    )");

    // 创建检测记录按钮
    m_recordBtn = new QPushButton("检测记录", buttonContainer);
    m_recordBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2ECC71;
            color: white;
            border: none;
            padding: 5px 5px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: #27AE60;
        }
    )");

    m_settingsBtn->setFixedSize(80, 30);
    m_recordBtn->setFixedSize(80, 30);

    buttonLayout->addWidget(m_settingsBtn);
    buttonLayout->addWidget(m_recordBtn);

    buttonContainer->setFixedSize(180, 30);
    buttonContainer->move(width() - 190, 10);
    buttonContainer->setParent(this);
}

void MainWindow::setupConnections()
{
    // 图片检测
    connect(m_imageSelectBtn, &QPushButton::clicked, this, &MainWindow::selectImage);
    connect(m_imageStartBtn, &QPushButton::clicked, this, &MainWindow::startImageDetection);
    connect(m_imageStopBtn, &QPushButton::clicked, this, &MainWindow::stopImageDetection);

    // 视频检测
    connect(m_videoSelectBtn, &QPushButton::clicked, this, &MainWindow::selectVideo);
    connect(m_videoStartBtn, &QPushButton::clicked, this, &MainWindow::startVideoDetection);
    connect(m_videoStopBtn, &QPushButton::clicked, this, &MainWindow::stopVideoDetection);

    // 实时视频
    connect(m_cameraStartBtn, &QPushButton::clicked, this, &MainWindow::startCamera);
    connect(m_cameraStopBtn, &QPushButton::clicked, this, &MainWindow::stopCamera);

    // 网络摄像头
    connect(m_ipcameraStartBtn, &QPushButton::clicked, this, &MainWindow::startIPCamera);
    connect(m_ipcameraStopBtn, &QPushButton::clicked, this, &MainWindow::stopIPCamera);

    // 设置和记录
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettings);
    connect(m_recordBtn, &QPushButton::clicked, this, &MainWindow::showRecords);

    // 视频处理器信号
    connect(m_videoProcessor.get(), &VideoProcessor::frameReady,
            this, &MainWindow::onFrameReady);
    connect(m_videoProcessor.get(), &VideoProcessor::sourceOpened,
            this, &MainWindow::onSourceOpened);
    connect(m_videoProcessor.get(), &VideoProcessor::error,
            this, &MainWindow::onVideoError);
}

void MainWindow::loadConfig()
{
    m_config->load();
    m_currentModelPath = QString::fromStdString(m_config->getModelPath());
}

void MainWindow::saveConfig()
{
    m_config->setModelPath(m_currentModelPath.toStdString());
    m_config->save();
}

void MainWindow::selectImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择图片",
        "",
        "图片文件 (*.png *.jpg *.jpeg *.bmp)"
        );

    if (!fileName.isEmpty()) {
        m_currentImagePath = fileName;
        m_imagepathLabel->setText(QFileInfo(fileName).fileName());

        // 显示选择的图片
        QPixmap pixmap(fileName);
        QPixmap scaled = pixmap.scaled(
            m_imageLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );
        m_imageLabel->setPixmap(scaled);
    }
}

void MainWindow::startImageDetection()
{
    if (m_currentImagePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择图片文件");
        return;
    }

    cv::Mat image = cv::imread(m_currentImagePath.toStdString());
    if (image.empty()) {
        QMessageBox::warning(this, "错误", "无法读取图片文件");
        return;
    }

    // 调整图像大小
    cv::resize(image, image, cv::Size(DISPLAY_WIDTH, DISPLAY_HEIGHT));

    // 执行检测
    auto results = m_detectionEngine->detect(image);

    // 绘制检测结果
    for (const auto& det : results) {
        cv::rectangle(image, det.bbox, cv::Scalar(0, 255, 0), 2);
        std::string label = det.className + " " +
                            std::to_string(int(det.confidence * 100)) + "%";
        cv::putText(image, label,
                    cv::Point(det.bbox.x, det.bbox.y - 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 255, 0), 2);

        // 保存检测结果（图片检测直接保存）
        m_dbManager->saveDetection(det.className, det.confidence);
    }

    // 显示结果
    onFrameReady(image);
    updateDetectionResult(QString("检测完成：发现 %1 个目标").arg(results.size()));
}

void MainWindow::stopImageDetection()
{
    m_imageLabel->clear();
    m_imageLabel->setText("等待检测...");
    updateDetectionResult("检测已停止");
}

void MainWindow::selectVideo()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择视频",
        "",
        "视频文件 (*.mp4 *.avi *.mov)"
        );

    if (!fileName.isEmpty()) {
        m_currentVideoPath = fileName;
        m_videoPathLabel->setText(QFileInfo(fileName).fileName());
    }
}

void MainWindow::startVideoDetection()
{
    if (m_currentVideoPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择视频文件");
        return;
    }

    m_videoProcessor->openVideo(m_currentVideoPath.toStdString());
    m_videoProcessor->start();
    updateDetectionResult("视频检测已开始");
}

void MainWindow::stopVideoDetection()
{
    m_videoProcessor->stop();
    updateDetectionResult("视频检测已停止");
}

void MainWindow::startCamera()
{
    if (m_videoProcessor->isRunning()) {
        stopCamera();
    }

    // 异步打开摄像头，不再阻塞 UI
    m_videoProcessor->openCamera(0);
    m_videoProcessor->start();
    updateDetectionResult("正在打开摄像头...");
}

void MainWindow::stopCamera()
{
    m_videoProcessor->stop();
    updateDetectionResult("摄像头已关闭");
}

void MainWindow::startIPCamera()
{
    bool ok;
    QString address = QInputDialog::getText(
        this,
        "输入IP摄像头地址",
        "请输入IP摄像头地址：\n(格式: rtsp://用户名:密码@IP地址:端口)",
        QLineEdit::Normal,
        "rtsp://admin:admin@192.168.1.1:554",
        &ok
        );

    if (!ok || address.isEmpty()) {
        return;
    }

    if (m_videoProcessor->isRunning()) {
        stopIPCamera();
    }

    m_ipCameraAddress = address;

    // 异步打开 IP 摄像头，不再阻塞 UI
    m_videoProcessor->openIPCamera(address.toStdString());
    m_videoProcessor->start();
    updateDetectionResult("正在连接IP摄像头...");
    m_ipcameraStartBtn->setEnabled(false);
    m_ipcameraStopBtn->setEnabled(true);
}

void MainWindow::stopIPCamera()
{
    m_videoProcessor->stop();
    updateDetectionResult("IP摄像头已断开");
    m_ipcameraStartBtn->setEnabled(true);
    m_ipcameraStopBtn->setEnabled(false);
}

void MainWindow::reconnectIPCamera()
{
    if (!m_ipCameraAddress.isEmpty()) {
        stopIPCamera();
        QTimer::singleShot(1000, [this]() {
            if (m_videoProcessor->openIPCamera(m_ipCameraAddress.toStdString())) {
                m_videoProcessor->start();
                updateDetectionResult("已重新连接到IP摄像头");
            }
        });
    }
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(this, m_currentModelPath, m_config->getDatabasePath().c_str());
    if (dialog.exec() == QDialog::Accepted) {
        QString newModelPath = dialog.getSelectedModelPath();
        QString newDbPath = dialog.getDatabasePath();

        if (!newModelPath.isEmpty() && newModelPath != m_currentModelPath) {
            if (m_detectionEngine->loadModel(newModelPath.toStdString())) {
                qDebug() << "loadModel returned"
                         << m_detectionEngine->loadModel(m_currentModelPath.toStdString());
                m_currentModelPath = newModelPath;
                QMessageBox::information(this, "成功", "模型已更新");
            } else {
                QMessageBox::warning(this, "错误", "加载模型失败");
            }
        }

        if (newDbPath != QString::fromStdString(m_config->getDatabasePath())) {
            m_config->setDatabasePath(newDbPath.toStdString());
            m_dbManager = std::make_unique<DatabaseManager>(newDbPath.toStdString());
        }

        saveConfig();
    }
}

void MainWindow::showRecords()
{
    DetectionRecordDialog dialog(m_dbManager.get(), this);
    dialog.exec();
}

void MainWindow::onFrameReady(const cv::Mat& frame)
{
    if (frame.empty()) {
        return;
    }

    // Worker已经完成了检测和绘制，这里只需要显示
    cv::Mat rgb;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    } else {
        rgb = frame;
    }

    // 创建QImage并复制数据
    QImage qImg(rgb.data, rgb.cols, rgb.rows, rgb.step,
                rgb.channels() == 3 ? QImage::Format_RGB888 : QImage::Format_Grayscale8);
    QImage qImgCopy = qImg.copy();

    // 显示到QLabel
    QPixmap pixmap = QPixmap::fromImage(qImgCopy);
    QPixmap scaledPixmap = pixmap.scaled(m_imageLabel->size(),
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaledPixmap);

    if (!m_imageLabel->text().isEmpty()) {
        m_imageLabel->setText("");
    }
}


void MainWindow::updateDetectionResult(const QString& result)
{
    m_resultLabel->setText(result);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (m_settingsBtn) {
        QWidget* buttonContainer = m_settingsBtn->parentWidget();
        if (buttonContainer) {
            buttonContainer->move(width() - 190, 10);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    stopCamera();
    stopIPCamera();
    stopVideoDetection();
    saveConfig();
    event->accept();
}

void MainWindow::onSourceOpened(bool success)
{
    if (success) {
        updateDetectionResult("视频源已成功打开");
    } else {
        updateDetectionResult("无法打开视频源");
        QMessageBox::warning(this, "错误", "无法打开视频源，请检查设备或地址");
        // 恢复按钮状态
        m_ipcameraStartBtn->setEnabled(true);
        m_ipcameraStopBtn->setEnabled(false);
    }
}

void MainWindow::onVideoError(const QString& message)
{
    updateDetectionResult("错误: " + message);
}

void MainWindow::updatePerformanceIndicator()
{
    // 更新旋转角度
    m_rotationAngle = (m_rotationAngle + 30) % 360;

    // 根据角度选择不同的旋转符号
    QStringList rotationChars = {"◴", "◷", "◶", "◵"};
    int index = (m_rotationAngle / 90) % 4;
    m_performanceLabel->setText(rotationChars[index]);

    // 计算 FPS
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_lastFrameTime > 0) {
        qint64 elapsed = currentTime - m_lastFrameTime;
        if (elapsed > 0) {
            m_currentFPS = 1000.0 / elapsed;
            // 使用简单的移动平均来平滑 FPS 显示
            static double avgFPS = 0.0;
            avgFPS = avgFPS * 0.9 + m_currentFPS * 0.1;
            m_fpsLabel->setText(QString("UI FPS: %1").arg(avgFPS, 0, 'f', 1));
        }
    }
    m_lastFrameTime = currentTime;

    // 根据 FPS 改变颜色
    if (m_currentFPS < 15) {
        m_performanceLabel->setStyleSheet(R"(
            QLabel {
                font-size: 40px;
                color: #E74C3C;
                margin: 10px;
            }
        )");
    } else if (m_currentFPS < 25) {
        m_performanceLabel->setStyleSheet(R"(
            QLabel {
                font-size: 40px;
                color: #F39C12;
                margin: 10px;
            }
        )");
    } else {
        m_performanceLabel->setStyleSheet(R"(
            QLabel {
                font-size: 40px;
                color: #2ECC71;
                margin: 10px;
            }
        )");
    }
}
