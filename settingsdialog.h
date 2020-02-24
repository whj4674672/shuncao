#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtSerialBus/qtserialbusglobal.h>
#include <QDialog>
#include <QSerialPort>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    struct Settings {
        int parity = QSerialPort::NoParity;
        int baud = QSerialPort::Baud9600;
        int dataBits = QSerialPort::Data8;
        int stopBits = QSerialPort::OneStop;
        int responseTime = 1000;
        int numberOfRetries = 3;
    };

    Settings settings() const;

private:
    Ui::SettingsDialog *ui;
    Settings m_settings;
};

#endif // SETTINGSDIALOG_H
