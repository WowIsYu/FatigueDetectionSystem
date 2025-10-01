#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <vector>
#include <memory>
#include <map>

struct DetectionRecord {
    int id;
    QString timestamp;
    QString detectionType;
    double confidence;
};

class DatabaseManager
{
public:
    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();

    // 数据库操作
    bool initDatabase();
    bool saveDetection(const std::string& detectionType, double confidence);
    std::vector<DetectionRecord> getRecentRecords(int limit = 100);
    std::vector<DetectionRecord> getRecordsByTimeRange(const QString& startTime, const QString& endTime);
    bool clearAllRecords();

    // 统计功能
    int getTotalDetectionCount();
    int getDetectionCountByType(const std::string& type);
    double getAverageConfidence();
    std::vector<std::pair<std::string, int>> getDetectionStatistics();

private:
    std::string m_dbPath;
    QSqlDatabase m_database;
    std::map<std::string, qint64> m_lastSaveTime;

    bool createTables();
    bool executeQuery(const QString& query);
};

#endif // DATABASEMANAGER_H
