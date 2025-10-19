#include "VideoProcessor.h"
#include "DetectionEngine.h"
#include "DatabaseManager.h"
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <Windows.h>
#include <qcoreapplication.h>

// VideoProcessorWorker 实现
VideoProcessorWorker::VideoProcessorWorker()
    : m_running(false)
    , m_deviceId(0)
    , m_isDevice(false)
    , m_detectionEngine(nullptr)
    , m_dbManager(nullptr)
    , m_displayWidth(960)
    , m_displayHeight(540)
    , m_enableDetection(true)
    , m_confidenceThreshold(0.6)
    , m_saveInterval(1000)
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

void VideoProcessorWorker::setDetectionEngine(DetectionEngine* engine)
{
    m_detectionEngine = engine;
}

void VideoProcessorWorker::setDatabaseManager(DatabaseManager* dbManager)
{
    m_dbManager = dbManager;
}

void VideoProcessorWorker::setDisplaySize(int width, int height)
{
    m_displayWidth = width;
    m_displayHeight = height;
}

void VideoProcessorWorker::setEnableDetection(bool enable)
{
    m_enableDetection = enable;
}

void VideoProcessorWorker::start()
{
    if (m_running) {
        return;
    }

    // 打开视频源（在 Worker 线程中执行，不会阻塞 UI）
    bool success = false;
    if (m_isDevice) {
        success = m_capture.open(m_deviceId);
    } else {
        success = m_capture.open(m_source);
    }

    // 发送初始化结果信号
    emit opened(success);

    if (!success) {
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
    if (!m_capture.read(frame)) {
        // 视频结束或读取错误
        if (!m_isDevice) {
            // 对于视频文件，可能已到达结尾
            stop();
            return;
        }
        // 对于摄像头，尝试重新连接
        emit error("帧读取失败");

        // 继续尝试
        if (m_running) {
            QTimer::singleShot(33, this, &VideoProcessorWorker::process);
        }
        return;
    }

    if (frame.empty()) {
        if (m_running) {
            QTimer::singleShot(33, this, &VideoProcessorWorker::process);
        }
        return;
    }

    // 在Worker线程中处理：Resize + 检测 + 绘制
    cv::Mat displayFrame;
    cv::resize(frame, displayFrame, cv::Size(m_displayWidth, m_displayHeight));

    // 如果启用检测且引擎可用
    if (m_enableDetection && m_detectionEngine) {
        auto results = m_detectionEngine->detect(displayFrame);

        // 绘制检测结果
        for (const auto& det : results) {
            cv::rectangle(displayFrame, det.bbox, cv::Scalar(0, 255, 0), 2);

            std::string label = det.className + " " +
                                std::to_string(int(det.confidence * 100)) + "%";
            cv::putText(displayFrame, label,
                        cv::Point(det.bbox.x, det.bbox.y - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0, 255, 0), 2);

            // 保存检测结果到数据库
            if (m_dbManager && shouldSaveDetection(det.className, det.confidence)) {
                m_dbManager->saveDetection(det.className, det.confidence);
            }
        }
    }

    // 发送处理好的帧到主线程显示
    emit frameReady(displayFrame);

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
    qDebug() << "Worker thread:" << m_worker->thread();
    qDebug() << "VideoProcessor thread:" << this->thread();
    connect(m_worker.get(), &VideoProcessorWorker::error,
            this, &VideoProcessor::error);
    connect(m_worker.get(), &VideoProcessorWorker::opened,
            this, &VideoProcessor::sourceOpened);

    // connect(m_thread.get(), &QThread::started, [this]() {
    //     HANDLE h = reinterpret_cast<HANDLE>(m_thread->currentThreadId());
    //     SetThreadAffinityMask(h, 1ULL << 3);   // 核 3
    // });

    // connect(m_thread.get(), &QThread::started, [this]() {
    //     HANDLE h = GetCurrentThread();   // 当前线程句柄（worker）

    //     /* 1. 打印前 */
    //     DWORD procBefore = GetCurrentProcessorNumber();
    //     qDebug() << "worker thread"
    //              << Qt::hex << reinterpret_cast<quintptr>(h)
    //              << "currently on CPU" << procBefore;

    //     /* 3. 绑核 */
    //     SetThreadAffinityMask(h, 1ULL << 3);

    //     /* 4. 打印后 */
    //     DWORD procAfter = GetCurrentProcessorNumber();
    //     qDebug() << "worker thread now bound to CPU" << procAfter;
    // });

    connect(m_thread.get(), &QThread::started, [this]() {
        // ========== 验证1：当前线程ID ==========
        qDebug() << "Lambda executing in thread:"
                 << QThread::currentThread();
        qDebug() << "Main thread:"
                 << QCoreApplication::instance()->thread();
        qDebug() << "Worker thread (m_thread):"
                 << m_thread.get();

        // 1. 当前线程“伪句柄” → 真实句柄
        HANDLE pseudo = GetCurrentThread();   // 0xFFFFFFFE
        HANDLE real   = nullptr;
        DuplicateHandle(
            GetCurrentProcess(),      // 源进程
            pseudo,                   // 源句柄（伪）
            GetCurrentProcess(),      // 目标进程
            &real,                    // 输出：真实句柄
            0, FALSE, DUPLICATE_SAME_ACCESS);


        // ========== 验证2：GetCurrentThread() ==========
        DWORD threadId = GetCurrentThreadId();
        qDebug() << "Current Windows thread ID:" << threadId;
        // 这个ID不是主线程的ID，而是子线程的ID ✅

        // 2. 当前核
        DWORD procBefore = GetCurrentProcessorNumber();
        qDebug() << "worker real handle" << real
                 << "currently on CPU" << procBefore;

        // 3. 绑核
        SetThreadAffinityMask(real, 1ULL << 3);

        // 4. 立即再看
        DWORD procAfter = GetCurrentProcessorNumber();
        qDebug() << "worker now bound to CPU" << procAfter;

        // 5. 用完关闭
        CloseHandle(real);

    });


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

    // 不再在主线程测试打开，改为直接返回 true
    // 实际的打开操作将在 Worker 线程的 start() 中异步执行
    return true;
}

bool VideoProcessor::openCamera(int deviceId)
{
    if (m_isRunning) {
        stop();
    }

    m_worker->setDevice(deviceId);

    // 不再在主线程测试打开，改为直接返回 true
    // 实际的打开操作将在 Worker 线程的 start() 中异步执行
    return true;
}

bool VideoProcessor::openIPCamera(const std::string& url)
{
    if (m_isRunning) {
        stop();
    }

    m_worker->setSource(url);

    // 不再在主线程测试打开，改为直接返回 true
    // 实际的打开操作将在 Worker 线程的 start() 中异步执行
    return true;
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

void VideoProcessor::setDetectionEngine(DetectionEngine* engine)
{
    m_worker->setDetectionEngine(engine);
}

void VideoProcessor::setDatabaseManager(DatabaseManager* dbManager)
{
    m_worker->setDatabaseManager(dbManager);
}

void VideoProcessor::setDisplaySize(int width, int height)
{
    m_worker->setDisplaySize(width, height);
}

void VideoProcessor::setEnableDetection(bool enable)
{
    m_worker->setEnableDetection(enable);
}

bool VideoProcessorWorker::shouldSaveDetection(const std::string& name, double confidence)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 检查置信度阈值
    if (confidence < m_confidenceThreshold) {
        return false;
    }

    // 检查时间间隔
    if (m_lastSaveTime.count(name) > 0) {
        if (currentTime - m_lastSaveTime[name] < m_saveInterval) {
            return false;
        }
    }

    // 检查连续检测
    if (m_lastDetection.count(name) > 0) {
        double lastConf = m_lastDetection[name];
        if (std::abs(confidence - lastConf) > 0.3) {
            m_consecutiveCount[name] = 0;
            return false;
        }

        m_consecutiveCount[name]++;
        if (m_consecutiveCount[name] < 2) {
            return false;
        }
    } else {
        m_consecutiveCount[name] = 1;
        m_lastDetection[name] = confidence;
        return false;
    }

    // 更新记录
    m_lastDetection[name] = confidence;
    m_lastSaveTime[name] = currentTime;
    return true;
}
