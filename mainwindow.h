#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "settingsdialog.h"
#include <QTime>
#include <QModbusDataUnit>

class QModbusClient;
class QModbusReply;

namespace Ui {
class MainWindow;
class SettingsDialog;
}

class SettingsDialog;
class WriteRegisterModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initActions();
    void Read_data();//定时读取从机数据的槽函数
    void Disconut_serial();//断开连接后的槽函数
    QModbusDataUnit readRequest() const; //主要的作用是打包成modbus发送的格式
    QModbusDataUnit writeRequest(quint16 add,quint16 lengh) const;
    void open_cmd(void);
    void write_cmd(quint16 cmd_address,quint16 lengh, quint16* data);
    void Auto_write_data(void);

    int startAddress = 4000;//顺槽箱的起始地址
    quint16 numberOfEntries = 55;//一次读取的长度
    quint16 ctrl_address = 30; //实际地址4031
    quint16 cmd_data;//控制位的数据
    quint16 lajia_data;

private slots:
    void onStateChanged(int state);
    void on_connectButton_clicked();
    void readReady();

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
    QModbusReply *lastRequest;
    QModbusClient *modbusDevice;
    WriteRegisterModel *writeModel;
    bool connect_flag = false;//连接状态标志位，初始化设置为fasle
    bool auto_flag;
    bool remote_flag = false;
    bool sanjiao_flag = false;
};



#endif // MAINWINDOW_H















