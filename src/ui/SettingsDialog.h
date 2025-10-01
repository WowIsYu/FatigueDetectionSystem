#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QLineEdit;
class QGroupBox;
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr,
                            const QString& currentModelPath = QString(),
                            const QString& currentDbPath = QString());
    ~SettingsDialog();

    QString getSelectedModelPath() const { return m_selectedModelPath; }
    QString getDatabasePath() const;

private slots:
    void selectModel();
    void selectDatabase();
    void onAccepted();
    void onRejected();

private:
    void setupUI();

    // UI组件
    QLabel* m_modelPathLabel;
    QPushButton* m_selectModelBtn;
    QLineEdit* m_dbPathInput;
    QPushButton* m_selectDbBtn;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // 数据
    QString m_selectedModelPath;
    QString m_currentModelPath;
    QString m_currentDbPath;
};

#endif // SETTINGSDIALOG_H
