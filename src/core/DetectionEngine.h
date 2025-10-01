#ifndef DETECTIONENGINE_H
#define DETECTIONENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

struct Detection {
    cv::Rect bbox;
    float confidence;
    int classId;
    std::string className;
};

class DetectionEngine
{
public:
    DetectionEngine();
    ~DetectionEngine();

    // 模型管理
    bool loadModel(const std::string& modelPath);
    bool isModelLoaded() const { return m_modelLoaded; }

    // 检测功能
    std::vector<Detection> detect(const cv::Mat& image);

    // 配置
    void setConfidenceThreshold(float threshold) { m_confThreshold = threshold; }
    void setNMSThreshold(float threshold) { m_nmsThreshold = threshold; }
    float getConfidenceThreshold() const { return m_confThreshold; }
    float getNMSThreshold() const { return m_nmsThreshold; }

    // 类别名称
    std::vector<std::string> getClassNames() const { return m_classNames; }

private:
    // ONNX Runtime相关
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::MemoryInfo> m_memoryInfo;

    // 模型信息
    std::vector<int64_t> m_inputShape;
    std::vector<int64_t> m_outputShape;
    size_t m_inputSize;
    size_t m_outputSize;
    bool m_modelLoaded;

    // 检测参数
    float m_confThreshold;
    float m_nmsThreshold;
    int m_inputWidth;
    int m_inputHeight;

    // 类别信息
    std::vector<std::string> m_classNames;

    // 内部处理函数
    cv::Mat preprocess(const cv::Mat& image);
    std::vector<Detection> postprocess(const std::vector<float>& output,
                                       const cv::Size& originalSize);
    std::vector<Detection> postprocessCustomFormat(const std::vector<float>& output,
                                                   const cv::Size& originalSize);
    std::vector<Detection> nms(std::vector<Detection>& detections);
    void initClassNames();
    void letterbox(const cv::Mat& src, cv::Mat& dst, const cv::Size& targetSize);
};

#endif // DETECTIONENGINE_H
