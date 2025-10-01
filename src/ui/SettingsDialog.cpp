#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>

SettingsDialog::SettingsDialog(QWidget* parent,
                               const QString& currentModelPath,
                               const QString& currentDbPath)
    : QDialog(parent)
    , m_currentModelPath(currentModelPath)
    , m_currentDbPath(currentDbPath)
    , m_selectedModelPath(currentModelPath)
{
    setWindowTitle("设置");
    setMinimumWidth(400);
    setupUI();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    // 模型设置组
    auto* modelGroup = new QGroupBox("模型设置", this);
    auto* modelLayout = new QVBoxLayout(modelGroup);

    // 当前模型路径显示
    m_modelPathLabel = new QLabel(m_currentModelPath.isEmpty() ?
                                      "未选择模型" : m_currentModelPath, modelGroup);
    m_modelPathLabel->setWordWrap(true);

    // 选择模型按钮
    m_selectModelBtn = new QPushButton("选择模型文件", modelGroup);
    connect(m_selectModelBtn, &QPushButton::clicked, this, &SettingsDialog::selectModel);

    modelLayout->addWidget(new QLabel("当前模型:", modelGroup));
    modelLayout->addWidget(m_modelPathLabel);
    modelLayout->addWidget(m_selectModelBtn);

    // 数据库设置组
    auto* dbGroup = new QGroupBox("数据库设置", this);
    auto* dbLayout = new QFormLayout(dbGroup);

    // 数据库路径输入
    m_dbPathInput = new QLineEdit(m_currentDbPath, dbGroup);
    dbLayout->addRow("数据库路径:", m_dbPathInput);

    // 选择数据库按钮
    m_selectDbBtn = new QPushButton("选择数据库文件", dbGroup);
    connect(m_selectDbBtn, &QPushButton::clicked, this, &SettingsDialog::selectDatabase);
    dbLayout->addRow("", m_selectDbBtn);

    // 确定和取消按钮
    auto* buttonsLayout = new QHBoxLayout();
    m_okButton = new QPushButton("确定", this);
    m_cancelButton = new QPushButton("取消", this);

    connect(m_okButton, &QPushButton::clicked, this, &SettingsDialog::onAccepted);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onRejected);

    buttonsLayout->addWidget(m_okButton);
    buttonsLayout->addWidget(m_cancelButton);

    // 添加到主布局
    layout->addWidget(modelGroup);
    layout->addWidget(dbGroup);
    layout->addLayout(buttonsLayout);
}

void SettingsDialog::selectModel()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择模型文件",
        "",
        "ONNX模型 (*.onnx);;PyTorch模型 (*.pt)"
        );

    if (!fileName.isEmpty()) {
        m_modelPathLabel->setText(fileName);
        m_selectedModelPath = fileName;
    }
}

void SettingsDialog::selectDatabase()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "选择数据库文件",
        "",
        "数据库文件 (*.db)"
        );

    if (!fileName.isEmpty()) {
        m_dbPathInput->setText(fileName);
    }
}

QString SettingsDialog::getDatabasePath() const
{
    return m_dbPathInput->text();
}

void SettingsDialog::onAccepted()
{
    accept();
}

void SettingsDialog::onRejected()
{
    reject();
}
