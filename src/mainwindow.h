#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <memory>
#include <opencv2/opencv.hpp>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
QT_END_NAMESPACE

// Forward declarations
class DatabaseManager;
class DetectionEngine;
class VideoProcessor;
class Config;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    // 图片检测
    void selectImage();
    void startImageDetection();
    void stopImageDetection();

    // 视频检测
    void selectVideo();
    void startVideoDetection();
    void stopVideoDetection();

    // 实时摄像头
    void startCamera();
    void stopCamera();

    // 网络摄像头
    void startIPCamera();
    void stopIPCamera();
    void reconnectIPCamera();

    // 设置和记录
    void showSettings();
    void showRecords();

    // 帧处理
    void processFrame();
    void onFrameReady(const cv::Mat& frame);

    // 性能监测
    void updatePerformanceIndicator();

private:
    void setupUI();
    void createLeftPanel();
    void createRightPanel();
    void createSettingsButton();
    void setupConnections();
    void loadConfig();
    void saveConfig();

    // 检测相关
    bool shouldSaveDetection(const QString& name, double confidence);
    void updateDetectionResult(const QString& result);

private:
    // UI组件 - 左侧面板
    QWidget* m_leftPanel;
    QLabel* m_imagepathLabel;
    QPushButton* m_imageSelectBtn;
    QPushButton* m_imageStartBtn;
    QPushButton* m_imageStopBtn;

    QLabel* m_videoPathLabel;
    QPushButton* m_videoSelectBtn;
    QPushButton* m_videoStartBtn;
    QPushButton* m_videoStopBtn;

    QPushButton* m_cameraStartBtn;
    QPushButton* m_cameraStopBtn;

    QPushButton* m_ipcameraStartBtn;
    QPushButton* m_ipcameraStopBtn;

    // UI组件 - 右侧面板
    QWidget* m_rightPanel;
    QLabel* m_imageLabel;
    QLabel* m_resultLabel;

    // 设置按钮
    QPushButton* m_settingsBtn;
    QPushButton* m_recordBtn;

    // 核心组件
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<DetectionEngine> m_detectionEngine;
    std::unique_ptr<VideoProcessor> m_videoProcessor;
    std::unique_ptr<Config> m_config;

    // OpenCV相关
    cv::VideoCapture m_capture;
    QTimer* m_frameTimer;

    // 状态变量
    QString m_currentModelPath;
    QString m_currentImagePath;
    QString m_currentVideoPath;
    QString m_ipCameraAddress;

    // 检测控制变量
    std::map<QString, double> m_lastDetection;
    std::map<QString, int> m_consecutiveCount;
    std::map<QString, qint64> m_lastSaveTime;
    double m_confidenceThreshold = 0.6;
    int m_saveInterval = 1000; // milliseconds

    // 性能监测
    QLabel* m_performanceLabel;
    QLabel* m_fpsLabel;
    QTimer* m_animationTimer;
    int m_rotationAngle;
    qint64 m_lastFrameTime;
    int m_frameCount;
    double m_currentFPS;

    // 常量
    static constexpr int DISPLAY_WIDTH = 960;
    static constexpr int DISPLAY_HEIGHT = 540;
    static constexpr int LEFT_PANEL_WIDTH = 200;

};

#endif // MAINWINDOW_H
