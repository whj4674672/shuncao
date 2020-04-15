#pragma execution_character_set("utf-8")
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include <QTimer>
#include <QStatusBar>
#include <QModbusRtuSerialMaster>
#include <QDebug>
#include <QSerialPortInfo>
#include <string.h>
//#include "mythread.h"

static bool connect_flag = false;
MyThread::MyThread(QObject *parent)
        :QThread(parent)
{
    modbusDevice = new QModbusRtuSerialMaster(this);//设置modbus主站

    if (modbusDevice)
    {
        connect(modbusDevice, &QModbusClient::stateChanged,this, &MyThread::my_connect);
    }
}

MyThread::~MyThread()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();

    delete modbusDevice;

    terminate();
}

void MyThread::run()
{
    exec();
}

void MyThread::my_connect(int state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);

    emit th_connect(connected);//给主线程发送一个modbus连接上的信号
}

void MyThread::Disconnect_modbus()
{
    modbusDevice->disconnectDevice(); //断开连接
}

void MyThread::connect_modbus(Set_uart setuart)
{
    Set_uart uart = setuart;
    //如果当前不是连接状态
    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        //设置连接的端口号
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,uart.com);//自动识别的方式
        //设置连接的 校验位 波特率 数据位 停止位
        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,uart.parity);  //这里强制设置成偶校验
            /*这里应该考虑如果没有进行串口配置的情况下是没有办法正确读取到奇偶校验的的值，所以会导致发送的报文不对，下一步这个地方需要进行优化*/
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,uart.baud);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,uart.dataBits);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,uart.stopBits);

        //设置modbus的超时时间
        modbusDevice->setTimeout(uart.responseTime);
        //设置重试次数
        modbusDevice->setNumberOfRetries(uart.numberOfRetries);

        qDebug()<<"modbus state:"<<modbusDevice->connectDevice();
        //检查连接状态
        if (modbusDevice->connectDevice()==false)
        {
            QString str;
            //显示连接失败
            str = modbusDevice->errorString();
            //connect_flag = false;
        }
        else if(modbusDevice->connectDevice()==true)
        {
           qDebug()<<"lianjie succ";
            connect_flag = true;
           // emit th_connect_ok();
        }
    }
}

void MyThread::Write_data(QModbusDataUnit data)
{
    if (!modbusDevice)
        return;

    if (auto *reply = modbusDevice->sendWriteRequest(data, 1))
    {
        //正常完成返回true 取反之后就是false
         if (!reply->isFinished())
         {
            //qDebug()<<"使能命令";
             //lamdba的connect 函数
             //传输结束的信号，接收槽是this 捕获列表是 [this, reply] 既当前窗口和reply
             connect(reply, &QModbusReply::finished, this, [this, reply]()
             {
                 //如果是协议错误
                 if (reply->error() == QModbusDevice::ProtocolError)
                 {
                     QString str =  reply->errorString();
                     emit th_read_err(str);
                 }
                 //如果是其他错误，打印错误信息
                 else if (reply->error() != QModbusDevice::NoError)
                 {
                     QString str =  reply->errorString();
                     emit th_read_err(str);
                 }
                 //释放reply
                 reply->deleteLater();
             });
         }
    }

}

void MyThread::Read_end()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    if (reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit unit = reply->result();

        emit th_read_end(unit);
    }
    else {
        QString str = modbusDevice->errorString();
        emit th_read_err(str);
    }
    reply->deleteLater();
}

QModbusDataUnit MyThread::readRequest() const
{
    return QModbusDataUnit(QModbusDataUnit::HoldingRegisters, startAddress, numberOfEntries);
}

//定时器超时之后读取从机的超时槽函数
void MyThread::Read_data()
{
        if (!modbusDevice) //如果为空 则直接返回
            return;

        //从站地址设置为1
        if (auto *reply = modbusDevice->sendReadRequest(readRequest(), 1))
        {
            //如果请求完成，那么返回ture 取反之后就是false
            if (!reply->isFinished())
                {
                //连接modbus的结束信号，连接到主界面的读准备信号
                connect(reply, &QModbusReply::finished, this,&MyThread::Read_end);
                }
            else //如果是广播信号删除reply
                delete reply; // broadcast replies return immediately
        }
        else //如果读取数据失败
        {
            QString str = modbusDevice->errorString();
            emit th_read_err(str);
        }
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_settingsDialog = new SettingsDialog(this);
    task = new MyThread(this);//new一个线程

    initActions();//初始化菜单栏
    statusBar()->showMessage("请先连接串口");

    //查询现在连接到电脑上的串口
    QList<QSerialPortInfo> serialportinfo = QSerialPortInfo::availablePorts();
    int count = serialportinfo.count();
    for (int i =0 ; i < count ; i++) {
        ui->comBox->addItem(serialportinfo.at(i).portName());
    }

    connect(task,&MyThread::th_connect,this,&MainWindow::onStateChanged);//子线程 告诉UI线程连接状态
    connect(task,&MyThread::th_read_err,this,&MainWindow::read_err);

    read_time = new QTimer;
    read_time->setInterval(200);//读取间隔是200ms
    //定时器定时读取顺槽箱的数据
    connect(read_time,&QTimer::timeout,task,&MyThread::Read_data);

    connect(task,&MyThread::th_read_end,this,&MainWindow::readReady);
    connect(this,&MainWindow::disconnect_modbus,task,&MyThread::Disconnect_modbus);
    connect(this,&MainWindow::connect_modbus,task,&MyThread::connect_modbus);
    connect(task,&MyThread::th_connect_ok,this,&MainWindow::connect_ok);
    connect(this,&MainWindow::Write_data,task,&MyThread::Write_data);

    auto_time = new QTimer;
    auto_time->setInterval(500);//读取间隔是500ms
    connect(auto_time,&QTimer::timeout,this,&MainWindow::Auto_write_data);

    task->start();
}

MainWindow::~MainWindow()
{
    delete task;
    delete ui;
}

void MainWindow::initActions()
{
    ui->actionConnect->setEnabled(true);     //菜单栏的连接按钮设置为true
    ui->actionDisconnect->setEnabled(false); //菜单栏的 DIS连接按钮设置为false
    ui->actionExit->setEnabled(true);        //菜单栏的退出按钮设置为 true

    auto_flag = false;
    //cmd_data = 0;
    //连接到打开串口的槽函数
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::on_connectButton_clicked);
    connect(ui->actionExit,       &QAction::triggered, this, &QMainWindow::close);//exit 连接到 close
    connect(ui->actionConnect,    &QAction::triggered, m_settingsDialog, &QDialog::show);//连接到提示框的show
}

void MainWindow::connect_ok()
{
    ui->actionConnect->setEnabled(false);//连接按钮设置为不可选中
    ui->actionDisconnect->setEnabled(true);//断开按钮设置可选中
    statusBar()->showMessage("连接成功" , 5000);
}

void MainWindow::read_err(QString str)
{
     statusBar()->showMessage(tr("Read error: ") + str, 5000);
}

void MainWindow::readReady(QModbusDataUnit unit)
{
        cmd_data =   unit.value(ctrl_address);
        lajia_data = unit.value(49);//4050的状态位

        ui->Lcd_setspeed->display(static_cast<int>(unit.value(0)));//采煤机给定速度
        ui->Lcd_leftqelec->display(static_cast<int>(unit.value(1)));//左牵引电流
        ui->Lcd_righqelec->display(static_cast<int>(unit.value(2)));//右牵引电流
        ui->Lcd_leftelec->display(static_cast<int>(unit.value(3)));//左截割电流
        ui->Lcd_righelec->display(static_cast<int>(unit.value(4)));//右截割电流
        ui->Lcd_lefttemp->display(static_cast<int>(unit.value(5)));//左截割温度
        ui->Lcd_rightemp->display(static_cast<int>(unit.value(6)));//右截割温度
        ui->Lcd_oilelec->display(static_cast<int>(unit.value(7)));//油泵电机电流

        ui->Lcd_nowplace->display(static_cast<int>(unit.value(13)));//采煤机当前位置

        ui->Lcd_readspeed->display(static_cast<int>(unit.value(14)));//采煤机实测速度

        ui->Lcd_leftup->display(static_cast<int>(unit.value(15)));//左采高
        ui->Lcd_righup->display(static_cast<int>(unit.value(16)));//右采高

        ui->Lcd_nowrate->display(static_cast<int>(unit.value(19)));//当前限速
        ui->Lcd_nowjianum->display(static_cast<int>(unit.value(20)));//当前架号
        ui->Lcd_worknum->display(static_cast<int>(unit.value(21)));//三角煤工艺号
        ui->Lcd_qingjiao->display(static_cast<int>(unit.value(22)));//倾角
        ui->Lcd_fujiao->display(static_cast<int>(unit.value(22)));//俯角
}

//串口断开连接的槽函数
void MainWindow::Disconut_serial()
{
    emit disconnect_modbus();

    ui->actionConnect->setEnabled(true); //连接按钮 设置为可选中
    ui->actionDisconnect->setEnabled(false);//断开按钮设置为不可选中
    //停止定时器
    read_time->stop();
    //设置串口打开的标志位是flase
    connect_flag = false;
}

//状态转换的槽函数
void MainWindow::onStateChanged(bool sta)
{
    if(sta)
    {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        //如果state是连接 则按钮显示 未连接
        read_time->start();
        ui->connectButton->setText("关闭串口");
        connect_ok();
    }
    else
    {
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        //如果state是未连接 则按钮显示 打开串口
        read_time->stop();
        ui->connectButton->setText("打开串口");
        Disconut_serial();
    }
}


void MainWindow::on_connectButton_clicked()
{
    statusBar()->clearMessage();
    qDebug() << connect_flag;
    if(!connect_flag)
    {
        Set_uart setuart;

        setuart.com             = ui->comBox->currentText();
        setuart.baud            = m_settingsDialog->settings().baud;
        setuart.parity          = m_settingsDialog->settings().parity;
        setuart.dataBits        = m_settingsDialog->settings().dataBits;
        setuart.stopBits        = m_settingsDialog->settings().stopBits;
        setuart.responseTime    = m_settingsDialog->settings().responseTime;
        setuart.numberOfRetries = m_settingsDialog->settings().numberOfRetries;
        connect_flag = true;
        emit connect_modbus(setuart);
    }
    else
    {
        onStateChanged(false);
    }
}

QModbusDataUnit MainWindow::writeRequest(quint16 add,quint16 lengh) const
{
    int startAdd = 4000;
    return QModbusDataUnit(QModbusDataUnit::HoldingRegisters, startAdd+add, lengh);
}

//写数据的统一处理函数
void MainWindow::write_cmd(quint16 cmd_address, quint16 lengh, quint16* data)
{
    QModbusDataUnit writeUnit = writeRequest(cmd_address,lengh);

    quint16 wdata[127] = {0};
    memcpy(wdata,data,2*lengh);
   //把要写入的数据放入到writeUnit
    for (int i = 0; i< lengh ; i++)
    {
      writeUnit.setValue(i,wdata[i]);
    }

    emit Write_data(writeUnit);
}

void MainWindow::on_Btn_Leftup_pressed()
{
   //按下的时候组合数据
    quint16 cmd = cmd_data | (1<<0);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Leftup_released()
{
    quint16 cmd = cmd_data & ~(1<<0);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Leftdown_pressed()
{
    quint16 cmd = cmd_data | (1<<1);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Leftdown_released()
{
    quint16 cmd = cmd_data & ~(1<<1);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Righup_pressed()
{
    quint16 cmd = cmd_data | (1<<2);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Righup_released()
{
    quint16 cmd = cmd_data & ~(1<<2);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Righdown_pressed()
{
    quint16 cmd = cmd_data | (1<<3);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Righdown_released()
{
    quint16 cmd = cmd_data & ~(1<<3);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Allstart_pressed()
{
    quint16 cmd = cmd_data | (1<<4);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Allstart_released()
{
    quint16 cmd = cmd_data & ~(1<<4);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Allstop_pressed()
{
    quint16 cmd = cmd_data | (1<<5);

    write_cmd(30,1,&cmd);
}
void MainWindow::on_Btn_Allstop_released()
{
    quint16 cmd = cmd_data & ~(1<<5);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Qstart_pressed()
{
    quint16 cmd = cmd_data | (1<<8);
    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Qstart_released()
{
    quint16 cmd = cmd_data & ~(1<<8);
    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Qstop_pressed()
{
    quint16 cmd = cmd_data | (1<<9);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Qstop_released()
{
    quint16 cmd = cmd_data & ~(1<<9);

    write_cmd(30,1,&cmd);
}


void MainWindow::on_Btn_Left_pressed()
{
    quint16 cmd = cmd_data | (1<<12);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Left_released()
{
    quint16 cmd = cmd_data & ~(1<<12);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Right_pressed()
{
    quint16 cmd = cmd_data | (1<<13);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Right_released()
{
    quint16 cmd = cmd_data & ~(1<<13);

    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_set_pressed()
{
    if(auto_flag)
    {
        auto_flag = false;
        auto_time->stop();
        ui->Btn_auto->setText("自动");
    }

    quint16 host_data[7] = {0};
    host_data[0] = static_cast<quint16>(ui->Box_Setspeed->value()); //32
    host_data[1] = lajia_data;//33
    host_data[2]= static_cast<quint16>(ui->Box_Setnum->value());   //34
    host_data[3]   = static_cast<quint16>(ui->Box_Setgas->value());   //35
    //36 三机运行情况
    host_data[5]  = static_cast<quint16>(ui->Box_Sethead->value());  //37
    host_data[6]   = static_cast<quint16>(ui->Box_Setend->value());   //38

    write_cmd(31,7,host_data);
}


//自动超时发送
void MainWindow::Auto_write_data()
{
    quint16 host_data[7] = {0};
    host_data[0] = static_cast<quint16>(ui->Box_Setspeed->value()); //32
    //33 模式变化 暂时没有这个功能
    host_data[2]= static_cast<quint16>(ui->Box_Setnum->value());   //34
    host_data[3]   = static_cast<quint16>(ui->Box_Setgas->value());   //35
    //36 三机运行情况
    host_data[5]  = static_cast<quint16>(ui->Box_Sethead->value());  //37
    host_data[6]   = static_cast<quint16>(ui->Box_Setend->value());   //38

    write_cmd(31,7,host_data);
}

void MainWindow::on_Btn_auto_clicked()
{
    if(auto_flag == false){
        auto_flag = true;
        auto_time->start();
        ui->Btn_auto->setText("停止");
    }
    else{
        auto_flag = false;
        auto_time->stop();
        ui->Btn_auto->setText("自动");
    }
}

void MainWindow::on_Btn_remote_clicked()
{
    if(remote_flag == false){
        remote_flag = true;

        quint16 cmd = 0x80;

        write_cmd(49,1,&cmd);

        ui->Btn_remote->setText("近控");
    }
    else{
        remote_flag = false;

        quint16 cmd = 0x00;
        write_cmd(49,1,&cmd);
        ui->Btn_remote->setText("远控");
    }

}

void MainWindow::on_Btn_jiyi_pressed()
{
    quint16 cmd = 0x08;

    write_cmd(32,1,&cmd);
}

void MainWindow::on_Btn_jiyi_released()
{
    quint16 cmd = 0x00;

    write_cmd(32,1,&cmd);
}

void MainWindow::on_Btn_sanjiao_clicked()
{
    if(sanjiao_flag == false){
        sanjiao_flag = true;

        quint16 cmd = 0x01;

        write_cmd(50,1,&cmd);

        ui->Btn_sanjiao->setText("三角煤失能");
    }
    else{
        sanjiao_flag = false;

        quint16 cmd = 0x00;
        write_cmd(50,1,&cmd);
        ui->Btn_sanjiao->setText("三角煤使能");
    }
}

void MainWindow::on_Btn_lajia_pressed()
{
    quint16 cmd = lajia_data | 3;
    write_cmd(49,1,&cmd);
}

void MainWindow::on_Btn_lajia_released()
{
    quint16 cmd = lajia_data & ~(3) ;
    write_cmd(49,1,&cmd);
}
