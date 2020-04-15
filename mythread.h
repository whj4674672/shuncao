#ifndef THREAD_H
#define THREAD_H
#include <QThread>
#include <QModbusDataUnit>
#include <QString>
#include "settingsdialog.h"

class QModbusClient;
class QModbusReply;
class WriteRegisterModel;

class MyThread:public QThread
{
    Q_OBJECT
public:
    explicit MyThread(QObject *parent = nullptr);
    ~MyThread();

    int startAddress = 4000;     //顺槽箱的起始地址
    quint16 numberOfEntries = 55;//一次读取的长度
    quint16 ctrl_address = 30;   //实际地址4031


    void run();       //线程入口函数（工作线程的主函数）
    void my_connect(int state);//线程连接的槽函数
    void Read_data(); //定时读取从机数据的槽函数
    void Read_end();
    void Disconnect_modbus();
    void connect_modbus(Set_uart setuart);
    void Write_data(QModbusDataUnit data);

private:
    QModbusReply       *lastRequest;
    QModbusClient      *modbusDevice;
    WriteRegisterModel *writeModel;

    QModbusDataUnit readRequest() const;

signals:
    void th_connect(bool sta);
    void th_read_err(QString str);
    void th_read_end(QModbusDataUnit unit);
    void th_connect_ok();
};

#endif // THREAD_H
