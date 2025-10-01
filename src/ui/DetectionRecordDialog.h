#ifndef DETECTIONRECORDDIALOG_H
#define DETECTIONRECORDDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
QT_END_NAMESPACE

class DatabaseManager;

class DetectionRecordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DetectionRecordDialog(DatabaseManager* dbManager, QWidget* parent = nullptr);
    ~DetectionRecordDialog();

private slots:
    void loadRecords();
    void exportRecords();
    void clearRecords();

private:
    void setupUI();

    // UI组件
    QTableWidget* m_table;
    QPushButton* m_refreshBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_closeBtn;

    // 数据
    DatabaseManager* m_dbManager;
};

#endif // DETECTIONRECORDDIALOG_H
