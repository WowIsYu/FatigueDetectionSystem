#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QVariant>

DatabaseManager::DatabaseManager(const std::string& dbPath)
    : m_dbPath(dbPath)
{
    initDatabase();
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::initDatabase()
{
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(QString::fromStdString(m_dbPath));

    if (!m_database.open()) {
        qDebug() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }

    return createTables();
}

bool DatabaseManager::createTables()
{
    QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS detection_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME NOT NULL,
            detection_type TEXT NOT NULL,
            confidence REAL NOT NULL
        )
    )";

    return executeQuery(createTableQuery);
}

bool DatabaseManager::executeQuery(const QString& query)
{
    QSqlQuery sqlQuery(m_database);
    if (!sqlQuery.exec(query)) {
        qDebug() << "Query failed:" << sqlQuery.lastError().text();
        qDebug() << "Query was:" << query;
        return false;
    }
    return true;
}

bool DatabaseManager::saveDetection(const std::string& detectionType, double confidence)
{
    // 检查是否在同一秒内已保存
    qint64 currentSecond = QDateTime::currentSecsSinceEpoch();
    QString typeStr = QString::fromStdString(detectionType);

    if (m_lastSaveTime.count(detectionType) > 0) {
        if (currentSecond == m_lastSaveTime[detectionType]) {
            qDebug() << "Already saved" << typeStr << "in this second, skipping";
            return false;
        }
    }

    // 保留3位小数
    double roundedConfidence = std::round(confidence * 1000.0) / 1000.0;
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    QSqlQuery query(m_database);
    query.prepare("INSERT INTO detection_results (timestamp, detection_type, confidence) "
                  "VALUES (:timestamp, :type, :confidence)");
    query.bindValue(":timestamp", timestamp);
    query.bindValue(":type", typeStr);
    query.bindValue(":confidence", roundedConfidence);

    if (!query.exec()) {
        qDebug() << "Failed to save detection:" << query.lastError().text();
        return false;
    }

    // 更新最后保存时间
    m_lastSaveTime[detectionType] = currentSecond;

    qDebug() << "Saved detection:" << typeStr
             << "confidence:" << roundedConfidence
             << "time:" << timestamp;

    return true;
}

std::vector<DetectionRecord> DatabaseManager::getRecentRecords(int limit)
{
    std::vector<DetectionRecord> records;

    QSqlQuery query(m_database);
    query.prepare("SELECT id, timestamp, detection_type, confidence "
                  "FROM detection_results "
                  "ORDER BY timestamp DESC "
                  "LIMIT :limit");
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        qDebug() << "Failed to get records:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        DetectionRecord record;
        record.id = query.value(0).toInt();
        record.timestamp = query.value(1).toString();
        record.detectionType = query.value(2).toString();
        record.confidence = query.value(3).toDouble();
        records.push_back(record);
    }

    return records;
}

std::vector<DetectionRecord> DatabaseManager::getRecordsByTimeRange(
    const QString& startTime, const QString& endTime)
{
    std::vector<DetectionRecord> records;

    QSqlQuery query(m_database);
    query.prepare("SELECT id, timestamp, detection_type, confidence "
                  "FROM detection_results "
                  "WHERE timestamp BETWEEN :start AND :end "
                  "ORDER BY timestamp");
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);

    if (!query.exec()) {
        qDebug() << "Failed to get records by time range:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        DetectionRecord record;
        record.id = query.value(0).toInt();
        record.timestamp = query.value(1).toString();
        record.detectionType = query.value(2).toString();
        record.confidence = query.value(3).toDouble();
        records.push_back(record);
    }

    return records;
}

bool DatabaseManager::clearAllRecords()
{
    return executeQuery("DELETE FROM detection_results");
}

int DatabaseManager::getTotalDetectionCount()
{
    QSqlQuery query(m_database);
    query.exec("SELECT COUNT(*) FROM detection_results");

    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getDetectionCountByType(const std::string& type)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM detection_results WHERE detection_type = :type");
    query.bindValue(":type", QString::fromStdString(type));

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

double DatabaseManager::getAverageConfidence()
{
    QSqlQuery query(m_database);
    query.exec("SELECT AVG(confidence) FROM detection_results");

    if (query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
}

std::vector<std::pair<std::string, int>> DatabaseManager::getDetectionStatistics()
{
    std::vector<std::pair<std::string, int>> stats;

    QSqlQuery query(m_database);
    query.exec("SELECT detection_type, COUNT(*) as count "
               "FROM detection_results "
               "GROUP BY detection_type "
               "ORDER BY count DESC");

    while (query.next()) {
        std::string type = query.value(0).toString().toStdString();
        int count = query.value(1).toInt();
        stats.emplace_back(type, count);
    }

    return stats;
}
