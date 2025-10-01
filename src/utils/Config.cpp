#include "Config.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

Config::Config()
    : m_configFile("config.json")
{
    // 设置默认值
    m_config["model_path"] = DEFAULT_MODEL_PATH;
    m_config["db_path"] = DEFAULT_DB_PATH;
    m_config["confidence_threshold"] = DEFAULT_CONF_THRESHOLD;
    m_config["nms_threshold"] = DEFAULT_NMS_THRESHOLD;
    m_config["save_interval"] = DEFAULT_SAVE_INTERVAL;
}

Config::~Config() = default;

bool Config::load(const std::string& filename)
{
    m_configFile = filename;
    QFile file(QString::fromStdString(filename));

    if (!file.exists()) {
        qDebug() << "Config file does not exist, using defaults";
        return save(filename); // 创建默认配置文件
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open config file for reading";
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse config file:" << error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qDebug() << "Config file does not contain a JSON object";
        return false;
    }

    // 合并配置，保留默认值中存在但文件中不存在的项
    QJsonObject fileConfig = doc.object();
    for (auto it = fileConfig.begin(); it != fileConfig.end(); ++it) {
        m_config[it.key()] = it.value();
    }

    return true;
}

bool Config::save(const std::string& filename)
{
    QFile file(QString::fromStdString(filename));

    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open config file for writing";
        return false;
    }

    QJsonDocument doc(m_config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

std::string Config::getModelPath() const
{
    return getString("model_path", DEFAULT_MODEL_PATH);
}

void Config::setModelPath(const std::string& path)
{
    setString("model_path", path);
}

std::string Config::getDatabasePath() const
{
    return getString("db_path", DEFAULT_DB_PATH);
}

void Config::setDatabasePath(const std::string& path)
{
    setString("db_path", path);
}

float Config::getConfidenceThreshold() const
{
    return static_cast<float>(getDouble("confidence_threshold", DEFAULT_CONF_THRESHOLD));
}

void Config::setConfidenceThreshold(float threshold)
{
    setDouble("confidence_threshold", threshold);
}

float Config::getNMSThreshold() const
{
    return static_cast<float>(getDouble("nms_threshold", DEFAULT_NMS_THRESHOLD));
}

void Config::setNMSThreshold(float threshold)
{
    setDouble("nms_threshold", threshold);
}

int Config::getSaveInterval() const
{
    return getInt("save_interval", DEFAULT_SAVE_INTERVAL);
}

void Config::setSaveInterval(int interval)
{
    setInt("save_interval", interval);
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const
{
    QString qKey = QString::fromStdString(key);
    if (m_config.contains(qKey)) {
        return m_config[qKey].toString().toStdString();
    }
    return defaultValue;
}

void Config::setString(const std::string& key, const std::string& value)
{
    m_config[QString::fromStdString(key)] = QString::fromStdString(value);
}

int Config::getInt(const std::string& key, int defaultValue) const
{
    QString qKey = QString::fromStdString(key);
    if (m_config.contains(qKey)) {
        return m_config[qKey].toInt();
    }
    return defaultValue;
}

void Config::setInt(const std::string& key, int value)
{
    m_config[QString::fromStdString(key)] = value;
}

double Config::getDouble(const std::string& key, double defaultValue) const
{
    QString qKey = QString::fromStdString(key);
    if (m_config.contains(qKey)) {
        return m_config[qKey].toDouble();
    }
    return defaultValue;
}

void Config::setDouble(const std::string& key, double value)
{
    m_config[QString::fromStdString(key)] = value;
}

bool Config::getBool(const std::string& key, bool defaultValue) const
{
    QString qKey = QString::fromStdString(key);
    if (m_config.contains(qKey)) {
        return m_config[qKey].toBool();
    }
    return defaultValue;
}

void Config::setBool(const std::string& key, bool value)
{
    m_config[QString::fromStdString(key)] = value;
}
