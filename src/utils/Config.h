#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <QJsonObject>

class Config
{
public:
    Config();
    ~Config();

    // 配置文件管理
    bool load(const std::string& filename = "config.json");
    bool save(const std::string& filename = "config.json");

    // 模型配置
    std::string getModelPath() const;
    void setModelPath(const std::string& path);

    // 数据库配置
    std::string getDatabasePath() const;
    void setDatabasePath(const std::string& path);

    // 检测参数
    float getConfidenceThreshold() const;
    void setConfidenceThreshold(float threshold);
    float getNMSThreshold() const;
    void setNMSThreshold(float threshold);

    // 保存间隔
    int getSaveInterval() const;
    void setSaveInterval(int interval);

    // 通用配置项
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    void setString(const std::string& key, const std::string& value);

    int getInt(const std::string& key, int defaultValue = 0) const;
    void setInt(const std::string& key, int value);

    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    void setDouble(const std::string& key, double value);

    bool getBool(const std::string& key, bool defaultValue = false) const;
    void setBool(const std::string& key, bool value);

private:
    QJsonObject m_config;
    std::string m_configFile;

    // 默认值
    static constexpr const char* DEFAULT_MODEL_PATH = "models/best_by_v12n_1W.onnx";
    static constexpr const char* DEFAULT_DB_PATH = "detection_results.db";
    static constexpr float DEFAULT_CONF_THRESHOLD = 0.6f;
    static constexpr float DEFAULT_NMS_THRESHOLD = 0.45f;
    static constexpr int DEFAULT_SAVE_INTERVAL = 1000;
};

#endif // CONFIG_H
