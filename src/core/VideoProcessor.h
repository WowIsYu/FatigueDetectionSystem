#ifndef VIDEOPROCESSOR_H
#define VIDEOPROCESSOR_H

#include <QObject>
#include <QThread>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <memory>

// Forward declaration
class DetectionEngine;
class DatabaseManager;

class VideoProcessorWorker : public QObject
{
    Q_OBJECT

public:
    VideoProcessorWorker();
    ~VideoProcessorWorker();

    void setSource(const std::string& source);
    void setDevice(int deviceId);
    void setDetectionEngine(DetectionEngine* engine);
    void setDatabaseManager(DatabaseManager* dbManager);
    void setDisplaySize(int width, int height);
    void setEnableDetection(bool enable);

signals:
    void frameReady(const cv::Mat& frame);
    void error(const QString& message);
    void opened(bool success);  // 新增：初始化完成信号

public slots:
    void process();
    void start();
    void stop();

private:
    cv::VideoCapture m_capture;
    std::atomic<bool> m_running;
    std::string m_source;
    int m_deviceId;
    bool m_isDevice;

    // 检测相关
    DetectionEngine* m_detectionEngine;
    DatabaseManager* m_dbManager;
    int m_displayWidth;
    int m_displayHeight;
    bool m_enableDetection;

    // 检测节流
    std::map<std::string, double> m_lastDetection;
    std::map<std::string, int> m_consecutiveCount;
    std::map<std::string, qint64> m_lastSaveTime;
    double m_confidenceThreshold;
    int m_saveInterval;

    bool shouldSaveDetection(const std::string& name, double confidence);
};

class VideoProcessor : public QObject
{
    Q_OBJECT

public:
    // VideoProcessor 实现
    VideoProcessor(QObject* parent = nullptr);
    ~VideoProcessor();

    // 视频源管理
    bool openVideo(const std::string& filename);
    bool openCamera(int deviceId = 0);
    bool openIPCamera(const std::string& url);
    void close();

    // 检测配置
    void setDetectionEngine(DetectionEngine* engine);
    void setDatabaseManager(DatabaseManager* dbManager);
    void setDisplaySize(int width, int height);
    void setEnableDetection(bool enable);


public slots:
    // 控制
    void start();
    void stop();
    bool isRunning() const;

    // 获取视频信息
    int getFrameCount() const;
    double getFPS() const;
    cv::Size getFrameSize() const;

signals:
    void frameReady(const cv::Mat& frame);
    void error(const QString& message);
    void finished();
    void sourceOpened(bool success);  // 新增：异步初始化完成信号

private:
    std::unique_ptr<QThread> m_thread;
    std::unique_ptr<VideoProcessorWorker> m_worker;
    bool m_isRunning;

    // 视频信息
    int m_frameCount;
    double m_fps;
    cv::Size m_frameSize;
};

#endif // VIDEOPROCESSOR_H
