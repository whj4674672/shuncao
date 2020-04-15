#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtSerialBus/qtserialbusglobal.h>
#include <QDialog>
#include <QSerialPort>

struct Set_uart {
    QString com;
    int parity          = QSerialPort::EvenParity;
    int baud            = QSerialPort::Baud9600;
    int dataBits        = QSerialPort::Data8;
    int stopBits        = QSerialPort::OneStop;
    int responseTime    = 50;
    int numberOfRetries = 1;
};

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
        int parity   = QSerialPort::EvenParity;
        int baud     = QSerialPort::Baud9600;
        int dataBits = QSerialPort::Data8;
        int stopBits = QSerialPort::OneStop;
        int responseTime = 500;
        int numberOfRetries = 1;
    };

    Settings settings() const;

private:
    Ui::SettingsDialog *ui;
    Settings m_settings;
};

#endif // SETTINGSDIALOG_H
