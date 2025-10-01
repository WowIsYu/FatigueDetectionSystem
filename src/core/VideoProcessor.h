#ifndef VIDEOPROCESSOR_H
#define VIDEOPROCESSOR_H

#include <QObject>
#include <QThread>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <memory>

class VideoProcessorWorker : public QObject
{
    Q_OBJECT

public:
    VideoProcessorWorker();
    ~VideoProcessorWorker();

    void setSource(const std::string& source);
    void setDevice(int deviceId);

signals:
    void frameReady(const cv::Mat& frame);
    void error(const QString& message);

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
};

class VideoProcessor : public QObject
{
    Q_OBJECT

public:
    VideoProcessor(QObject* parent = nullptr);
    ~VideoProcessor();

    // 视频源管理
    bool openVideo(const std::string& filename);
    bool openCamera(int deviceId = 0);
    bool openIPCamera(const std::string& url);
    void close();


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
