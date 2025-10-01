#include "VideoProcessor.h"
#include <QDebug>
#include <QTimer>

// VideoProcessorWorker 实现
VideoProcessorWorker::VideoProcessorWorker()
    : m_running(false)
    , m_deviceId(0)
    , m_isDevice(false)
{
}

VideoProcessorWorker::~VideoProcessorWorker()
{
    stop();
    if (m_capture.isOpened()) {
        m_capture.release();
    }
}

void VideoProcessorWorker::setSource(const std::string& source)
{
    m_source = source;
    m_isDevice = false;
}

void VideoProcessorWorker::setDevice(int deviceId)
{
    m_deviceId = deviceId;
    m_isDevice = true;
}

void VideoProcessorWorker::start()
{
    if (m_running) {
        return;
    }

    // 打开视频源
    if (m_isDevice) {
        m_capture.open(m_deviceId);
    } else {
        m_capture.open(m_source);
    }

    if (!m_capture.isOpened()) {
        emit error("无法打开视频源");
        return;
    }

    m_running = true;
    QTimer::singleShot(0, this, &VideoProcessorWorker::process);
}

void VideoProcessorWorker::stop()
{
    m_running = false;
}

void VideoProcessorWorker::process()
{
    if (!m_running || !m_capture.isOpened()) {
        return;
    }

    cv::Mat frame;
    if (m_capture.read(frame)) {
        if (!frame.empty()) {
            emit frameReady(frame);
        }
    } else {
        // 视频结束或读取错误
        if (!m_isDevice) {
            // 对于视频文件，可能已到达结尾
            stop();
            return;
        }
        // 对于摄像头，尝试重新连接
        emit error("帧读取失败");
    }

    // 继续处理下一帧
    if (m_running) {
        // 控制帧率，约30fps
        QTimer::singleShot(33, this, &VideoProcessorWorker::process);
    }
}

// VideoProcessor 实现
VideoProcessor::VideoProcessor(QObject* parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_frameCount(0)
    , m_fps(30.0)
{
    m_thread = std::make_unique<QThread>();
    m_worker = std::make_unique<VideoProcessorWorker>();

    // 将worker移动到线程
    m_worker->moveToThread(m_thread.get());

    // 连接信号
    connect(m_worker.get(), &VideoProcessorWorker::frameReady,
            this, &VideoProcessor::frameReady);
    connect(m_worker.get(), &VideoProcessorWorker::error,
            this, &VideoProcessor::error);

    m_thread->start();
}

VideoProcessor::~VideoProcessor()
{
    stop();
    m_thread->quit();
    m_thread->wait();
}

bool VideoProcessor::openVideo(const std::string& filename)
{
    if (m_isRunning) {
        stop();
    }

    m_worker->setSource(filename);

    // 获取视频信息
    cv::VideoCapture cap(filename);
    if (cap.isOpened()) {
        m_frameCount = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        m_fps = cap.get(cv::CAP_PROP_FPS);
        int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        m_frameSize = cv::Size(width, height);
        cap.release();
        return true;
    }

    return false;
}

bool VideoProcessor::openCamera(int deviceId)
{
    if (m_isRunning) {
        stop();
    }

    m_worker->setDevice(deviceId);

    // 测试摄像头
    cv::VideoCapture cap(deviceId);
    if (cap.isOpened()) {
        m_fps = 30.0; // 默认FPS
        int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        m_frameSize = cv::Size(width, height);
        cap.release();
        return true;
    }

    return false;
}

bool VideoProcessor::openIPCamera(const std::string& url)
{
    if (m_isRunning) {
        stop();
    }

    m_worker->setSource(url);

    // 测试连接
    cv::VideoCapture cap(url);
    if (cap.isOpened()) {
        m_fps = cap.get(cv::CAP_PROP_FPS);
        if (m_fps <= 0) {
            m_fps = 25.0; // 默认FPS
        }
        int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        m_frameSize = cv::Size(width, height);
        cap.release();
        return true;
    }

    return false;
}

void VideoProcessor::close()
{
    stop();
}

void VideoProcessor::start()
{
    if (m_isRunning) {
        return;
    }

    m_isRunning = true;
    QMetaObject::invokeMethod(m_worker.get(), "start", Qt::QueuedConnection);
}

void VideoProcessor::stop()
{
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;
    m_worker->stop();
    emit finished();
}

bool VideoProcessor::isRunning() const
{
    return m_isRunning;
}

int VideoProcessor::getFrameCount() const
{
    return m_frameCount;
}

double VideoProcessor::getFPS() const
{
    return m_fps;
}

cv::Size VideoProcessor::getFrameSize() const
{
    return m_frameSize;
}
