#include "DetectionRecordDialog.h"
#include "../core/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>

DetectionRecordDialog::DetectionRecordDialog(DatabaseManager* dbManager, QWidget* parent)
    : QDialog(parent)
    , m_dbManager(dbManager)
{
    setWindowTitle("检测记录");
    setMinimumSize(800, 600);
    setupUI();
    loadRecords();
}

DetectionRecordDialog::~DetectionRecordDialog() = default;

void DetectionRecordDialog::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    // 创建表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(QStringList() << "ID" << "时间" << "检测类型" << "置信度");

    // 设置表格样式
    m_table->setStyleSheet(R"(
        QTableWidget {
            background-color: white;
            alternate-background-color: #f0f0f0;
            border: 1px solid #d0d0d0;
        }
        QHeaderView::section {
            background-color: #2C3E50;
            color: white;
            padding: 5px;
            border: none;
        }
    )");

    // 设置表格列宽
    QHeaderView* header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    // 设置交替行颜色
    m_table->setAlternatingRowColors(true);

    layout->addWidget(m_table);

    // 按钮布局
    auto* buttonLayout = new QHBoxLayout();

    m_refreshBtn = new QPushButton("刷新", this);
    m_exportBtn = new QPushButton("导出", this);
    m_clearBtn = new QPushButton("清空", this);
    m_closeBtn = new QPushButton("关闭", this);

    // 设置按钮样式
    QString buttonStyle = R"(
        QPushButton {
            background-color: #3498DB;
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 3px;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #2980B9;
        }
        QPushButton:pressed {
            background-color: #2574A9;
        }
    )";

    m_refreshBtn->setStyleSheet(buttonStyle);
    m_exportBtn->setStyleSheet(buttonStyle);
    m_clearBtn->setStyleSheet(buttonStyle.replace("#3498DB", "#E74C3C").replace("#2980B9", "#C0392B"));
    m_closeBtn->setStyleSheet(buttonStyle);

    connect(m_refreshBtn, &QPushButton::clicked, this, &DetectionRecordDialog::loadRecords);
    connect(m_exportBtn, &QPushButton::clicked, this, &DetectionRecordDialog::exportRecords);
    connect(m_clearBtn, &QPushButton::clicked, this, &DetectionRecordDialog::clearRecords);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);

    buttonLayout->addWidget(m_refreshBtn);
    buttonLayout->addWidget(m_exportBtn);
    buttonLayout->addWidget(m_clearBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeBtn);

    layout->addLayout(buttonLayout);
}

void DetectionRecordDialog::loadRecords()
{
    auto records = m_dbManager->getRecentRecords(100);

    m_table->setRowCount(records.size());
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& record = records[i];

        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(record.id)));
        m_table->setItem(i, 1, new QTableWidgetItem(record.timestamp));
        m_table->setItem(i, 2, new QTableWidgetItem(record.detectionType));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(record.confidence, 'f', 3)));

        // 居中对齐
        for (int j = 0; j < 4; ++j) {
            m_table->item(i, j)->setTextAlignment(Qt::AlignCenter);
        }
    }
}

void DetectionRecordDialog::exportRecords()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出检测记录",
        "",
        "CSV文件 (*.csv);;文本文件 (*.txt)"
        );

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建导出文件");
        return;
    }

    QTextStream stream(&file);

    // 写入表头
    stream << "ID,时间,检测类型,置信度\n";

    // 写入数据
    auto records = m_dbManager->getRecentRecords(0); // 获取所有记录
    for (const auto& record : records) {
        stream << record.id << ","
               << record.timestamp << ","
               << record.detectionType << ","
               << QString::number(record.confidence, 'f', 3) << "\n";
    }

    file.close();
    QMessageBox::information(this, "成功", "检测记录已导出");
}

void DetectionRecordDialog::clearRecords()
{
    int ret = QMessageBox::question(
        this,
        "确认清空",
        "确定要清空所有检测记录吗？此操作不可恢复！",
        QMessageBox::Yes | QMessageBox::No
        );

    if (ret == QMessageBox::Yes) {
        if (m_dbManager->clearAllRecords()) {
            loadRecords();
            QMessageBox::information(this, "成功", "检测记录已清空");
        } else {
            QMessageBox::warning(this, "错误", "清空记录失败");
        }
    }
}
