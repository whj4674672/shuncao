#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "settingsdialog.h"
#include <QTime>
#include <QModbusDataUnit>
#include "mythread.h"

namespace Ui {
class MainWindow;
class SettingsDialog;
class MyThread;
}

class SettingsDialog;
class WriteRegisterModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void read_err(QString str);
private:
    void initActions();
    void Disconut_serial();//断开连接后的槽函数
    QModbusDataUnit writeRequest(quint16 add,quint16 lengh) const;
    void open_cmd(void);
    void write_cmd(quint16 cmd_address,quint16 lengh, quint16* data);
    void Auto_write_data(void);
    void readReady(QModbusDataUnit unit);
    void connect_ok(void);

    quint16 ctrl_address = 30; //实际地址4031
    quint16 cmd_data;//控制位的数据
    quint16 lajia_data;

private slots:
    void onStateChanged(bool sta);
    void on_connectButton_clicked();

    void on_Btn_Leftup_pressed();

    void on_Btn_Leftup_released();

    void on_Btn_Leftdown_pressed();

    void on_Btn_Leftdown_released();

    void on_Btn_Righup_pressed();

    void on_Btn_Righdown_released();

    void on_Btn_Allstart_pressed();

    void on_Btn_Allstop_released();

    void on_Btn_Allstop_pressed();

    void on_Btn_Allstart_released();

    void on_Btn_Qstart_pressed();

    void on_Btn_Qstart_released();

    void on_Btn_Qstop_pressed();

    void on_Btn_Qstop_released();

    void on_Btn_Righup_released();

    void on_Btn_Righdown_pressed();

    void on_Btn_Left_pressed();

    void on_Btn_Left_released();

    void on_Btn_Right_pressed();

    void on_Btn_Right_released();

    void on_Btn_set_pressed();

    void on_Btn_auto_clicked();

    void on_Btn_remote_clicked();

    void on_Btn_jiyi_pressed();

    void on_Btn_jiyi_released();

    void on_Btn_sanjiao_clicked();

    void on_Btn_lajia_pressed();

    void on_Btn_lajia_released();

private:
    Ui::MainWindow *ui;
    SettingsDialog *m_settingsDialog;
    QTimer* read_time;//读取间隔定时器
    QTimer* auto_time;//自动发送控制指令的时间

    MyThread *task;//声明一个线程

    bool auto_flag;
    bool remote_flag = false;
    bool sanjiao_flag = false;

signals:
    void disconnect_modbus();
    void connect_modbus(Set_uart setuart);
    void Write_data(QModbusDataUnit data);
};



#endif // MAINWINDOW_H















