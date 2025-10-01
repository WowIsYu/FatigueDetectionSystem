#include "DetectionEngine.h"
#include <algorithm>
#include <numeric>
#include <QDebug>

DetectionEngine::DetectionEngine()
    : m_modelLoaded(false)
    , m_confThreshold(0.5f)
    , m_nmsThreshold(0.45f)
    , m_inputWidth(640)
    , m_inputHeight(640)
{
    // 初始化ONNX Runtime环境
    m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "DetectionEngine");
    m_sessionOptions = std::make_unique<Ort::SessionOptions>();
    m_sessionOptions->SetIntraOpNumThreads(4);
    m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    m_memoryInfo = std::make_unique<Ort::MemoryInfo>(
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

    initClassNames();
}

DetectionEngine::~DetectionEngine() = default;

void DetectionEngine::initClassNames()
{
    // 根据输出维度，只有3个类别
    m_classNames = {
        "dahaqian",    // 闭眼睛
        "biyanjing",     // 打哈欠
        "normal"        // 正常
    };
}

bool DetectionEngine::loadModel(const std::string& modelPath)
{
    try {
        qDebug() << "--------------";
        qDebug() << "Initializing ONNX Runtime environment...";
        qDebug() << "--------------";
        qDebug() << QString::fromStdString(modelPath) << "--------------";

        // 创建ONNX Runtime会话
        // m_session = std::make_unique<Ort::Session>(*m_env, modelPath.c_str(), *m_sessionOptions);
        QString qstr = QString::fromStdString(modelPath); // std::string -> QString
        // 1. 转换为 std::wstring
        std::wstring modelPathW = qstr.toStdWString();
        // 显示模型路径
        qDebug() << modelPath << "--------------";
        qDebug() << modelPathW << "--------------";

        // 2. 使用 std::wstring::c_str()
        m_session = std::make_unique<Ort::Session>(*m_env, modelPathW.c_str(), *m_sessionOptions);

        // 获取输入信息
        Ort::AllocatorWithDefaultOptions allocator;
        size_t numInputNodes = m_session->GetInputCount();

        if (numInputNodes != 1) {
            qDebug() << "Model should have exactly 1 input, but has" << numInputNodes;
            return false;
        }

        // 获取输入维度
        Ort::TypeInfo inputTypeInfo = m_session->GetInputTypeInfo(0);
        auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
        m_inputShape = inputTensorInfo.GetShape();

        if (m_inputShape.size() != 4) {
            qDebug() << "Expected 4D input tensor";
            return false;
        }

        m_inputHeight = static_cast<int>(m_inputShape[2]);
        m_inputWidth = static_cast<int>(m_inputShape[3]);
        m_inputSize = std::accumulate(m_inputShape.begin(), m_inputShape.end(),
                                      1, std::multiplies<int64_t>());

        // 获取输出信息
        size_t numOutputNodes = m_session->GetOutputCount();
        if (numOutputNodes != 1) {
            qDebug() << "Model should have exactly 1 output, but has" << numOutputNodes;
            return false;
        }

        Ort::TypeInfo outputTypeInfo = m_session->GetOutputTypeInfo(0);
        auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
        m_outputShape = outputTensorInfo.GetShape();
        m_outputSize = std::accumulate(m_outputShape.begin(), m_outputShape.end(),
                                       1, std::multiplies<int64_t>());

        m_modelLoaded = true;
        qDebug() << QString::fromStdString(modelPath) << "--------------";
        qDebug() << "Model loaded successfully";
        qDebug() << "Input shape:" << m_inputShape[0] << m_inputShape[1]
                 << m_inputShape[2] << m_inputShape[3];
        qDebug() << "Output shape:" << m_outputShape[0] << m_outputShape[1]
                 << m_outputShape[2];

        return true;
    } catch (const Ort::Exception& e) {
        qDebug() << "Failed to load model:" << e.what();
        m_modelLoaded = false;
        return false;
    }
}

std::vector<Detection> DetectionEngine::detect(const cv::Mat& image)
{
    std::vector<Detection> results;

    if (!m_modelLoaded || image.empty()) {
        return results;
    }

    try {
        // 记录原始图像尺寸
        cv::Size originalSize = image.size();
        qDebug() << "Original Frame Size:" << originalSize.width << "x" << originalSize.height;

        // 预处理
        cv::Mat processedImage = preprocess(image);

        // 准备输入数据
        std::vector<float> inputData(m_inputSize);
        cv::Mat floatImage;
        processedImage.convertTo(floatImage, CV_32F, 1.0 / 255.0);

        // HWC to CHW
        std::vector<cv::Mat> channels(3);
        cv::split(floatImage, channels);
        for (int c = 0; c < 3; ++c) {
            std::memcpy(inputData.data() + c * m_inputHeight * m_inputWidth,
                        channels[c].data,
                        m_inputHeight * m_inputWidth * sizeof(float));
        }

        // 创建输入张量
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            *m_memoryInfo, inputData.data(), inputData.size(),
            m_inputShape.data(), m_inputShape.size());

        // 运行推理
        // const char* inputName = m_session->GetInputName(0, Ort::AllocatorWithDefaultOptions());
        // const char* outputName = m_session->GetOutputName(0, Ort::AllocatorWithDefaultOptions());

        // 运行推理
        // 获取默认分配器
        // Ort::AllocatorWithDefaultOptions allocator;

        // 获取输入名称
        Ort::AllocatedStringPtr inputNamePtr = m_session->GetInputNameAllocated(0, Ort::AllocatorWithDefaultOptions());
        const char* inputName = inputNamePtr.get();

        // 获取输出名称
        Ort::AllocatedStringPtr outputNamePtr = m_session->GetOutputNameAllocated(0, Ort::AllocatorWithDefaultOptions());
        const char* outputName = outputNamePtr.get();

        auto outputTensors = m_session->Run(Ort::RunOptions{nullptr},
                                            &inputName, &inputTensor, 1,
                                            &outputName, 1);

        // 获取输出数据
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        std::vector<float> output(outputData, outputData + m_outputSize);

        // 后处理 - 专门处理 [1, 7, 8400] 格式
        results = postprocessCustomFormat(output, originalSize);

    } catch (const Ort::Exception& e) {
        qDebug() << "Detection failed:" << e.what();
    }

    return results;
}

cv::Mat DetectionEngine::preprocess(const cv::Mat& image)
{
    cv::Mat resized;
    letterbox(image, resized, cv::Size(m_inputWidth, m_inputHeight));
    return resized;
}

void DetectionEngine::letterbox(const cv::Mat& src, cv::Mat& dst, const cv::Size& targetSize)
{
    float scale = std::min(static_cast<float>(targetSize.width) / src.cols,
                           static_cast<float>(targetSize.height) / src.rows);

    int newWidth = static_cast<int>(src.cols * scale);
    int newHeight = static_cast<int>(src.rows * scale);

    cv::Mat scaled;
    cv::resize(src, scaled, cv::Size(newWidth, newHeight));

    dst = cv::Mat::zeros(targetSize, src.type());
    dst.setTo(cv::Scalar(114, 114, 114));

    int top = (targetSize.height - newHeight) / 2;
    int left = (targetSize.width - newWidth) / 2;

    scaled.copyTo(dst(cv::Rect(left, top, newWidth, newHeight)));
}

std::vector<Detection> DetectionEngine::postprocessCustomFormat(const std::vector<float>& output,
                                                                const cv::Size& originalSize)
{
    std::vector<Detection> detections;

    // 输出格式: [1, 7, 8400]
    // 7个值: x_center, y_center, width, height, class1_score, class2_score, class3_score
    const int numBoxes = 8400;
    const int numValues = 7;

    float scaleX = static_cast<float>(originalSize.width) / m_inputWidth;
    float scaleY = static_cast<float>(originalSize.height) / m_inputHeight;

    qDebug() << "Display Frame Size:" << originalSize.width << "x" << originalSize.height;

    for (int i = 0; i < numBoxes; ++i) {
        // 获取边界框坐标 (前4个值) - 使用转置索引
        float cx = output[0 * numBoxes + i];  // x_center
        float cy = output[1 * numBoxes + i];  // y_center
        float w = output[2 * numBoxes + i];   // width
        float h = output[3 * numBoxes + i];   // height

        // 获取类别分数 (后3个值)
        float maxScore = 0;
        int classId = -1;
        for (int c = 0; c < 3; ++c) {
            float score = output[(4 + c) * numBoxes + i];
            if (score > maxScore) {
                maxScore = score;
                classId = c;
            }
        }

        // 应用置信度阈值
        if (maxScore < m_confThreshold || classId < 0) {
            continue;
        }

        // 将坐标从模型输出空间转换到原始图像空间
        float x1 = (cx - w / 2.0f) * scaleX;
        float y1 = (cy - h / 2.0f) * scaleY;
        float x2 = (cx + w / 2.0f) * scaleX;
        float y2 = (cy + h / 2.0f) * scaleY;

        // 限制在图像范围内
        x1 = std::max(0.0f, x1);
        y1 = std::max(0.0f, y1);
        x2 = std::min(static_cast<float>(originalSize.width - 1), x2);
        y2 = std::min(static_cast<float>(originalSize.height - 1), y2);

        Detection det;
        det.bbox = cv::Rect(static_cast<int>(x1),
                            static_cast<int>(y1),
                            static_cast<int>(x2 - x1),
                            static_cast<int>(y2 - y1));
        det.confidence = maxScore;
        det.classId = classId;
        det.className = (classId < m_classNames.size()) ? m_classNames[classId] : "unknown";

        detections.push_back(det);

        qDebug() << "Detection Box (x,y,w,h):" << det.bbox.x << det.bbox.y
                 << det.bbox.width << det.bbox.height;
    }

    // 应用NMS
    return nms(detections);
}

std::vector<Detection> DetectionEngine::postprocess(const std::vector<float>& output,
                                                    const cv::Size& originalSize)
{
    // 这个函数现在调用专门的处理函数
    return postprocessCustomFormat(output, originalSize);
}

std::vector<Detection> DetectionEngine::nms(std::vector<Detection>& detections)
{
    if (detections.empty()) {
        return detections;
    }

    std::sort(detections.begin(), detections.end(),
              [](const Detection& a, const Detection& b) {
                  return a.confidence > b.confidence;
              });

    std::vector<Detection> result;
    std::vector<bool> suppressed(detections.size(), false);

    for (size_t i = 0; i < detections.size(); ++i) {
        if (suppressed[i]) {
            continue;
        }

        result.push_back(detections[i]);

        for (size_t j = i + 1; j < detections.size(); ++j) {
            if (suppressed[j]) {
                continue;
            }

            cv::Rect intersection = detections[i].bbox & detections[j].bbox;
            float intersectionArea = intersection.area();
            float unionArea = detections[i].bbox.area() + detections[j].bbox.area() - intersectionArea;

            if (unionArea > 0) {
                float iou = intersectionArea / unionArea;
                if (iou > m_nmsThreshold) {
                    suppressed[j] = true;
                }
            }
        }
    }

    return result;
}
