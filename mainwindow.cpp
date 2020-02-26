#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include <QTimer>
#include <QStatusBar>
#include <QModbusRtuSerialMaster>
#include <QDebug>
#include <QSerialPortInfo>
#include <string.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
    , modbusDevice(nullptr)
{
    ui->setupUi(this);

    m_settingsDialog = new SettingsDialog(this);
    modbusDevice = new QModbusRtuSerialMaster(this);//设置modbus主站

    initActions();//初始化菜单栏
    statusBar()->showMessage("请先连接串口！");

    if (modbusDevice)
    {
        //连接modbusDevice 状态改变信号到  onStateChanged 槽函数
        connect(modbusDevice, &QModbusClient::stateChanged,
                this, &MainWindow::onStateChanged);
    }

    read_time = new QTimer;
    read_time->setInterval(200);//读取间隔是200ms
    //定时器定时读取顺槽箱的数据
    connect(read_time,&QTimer::timeout,this,&MainWindow::Read_data);

    auto_time = new QTimer;
    auto_time->setInterval(500);//读取间隔是500ms
    connect(auto_time,&QTimer::timeout,this,&MainWindow::Auto_write_data);

    QList<QSerialPortInfo> serialportinfo = QSerialPortInfo::availablePorts();
    int count = serialportinfo.count();
    for (int i =0 ; i < count ; i++) {
        ui->comBox->addItem(serialportinfo.at(i).portName());
    }
}

MainWindow::~MainWindow()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;

    delete ui;
}

void MainWindow::initActions()
{
    ui->actionConnect->setEnabled(true); //菜单栏的连接按钮设置为true
    ui->actionDisconnect->setEnabled(false); //菜单栏的 DIS连接按钮设置为false
    ui->actionExit->setEnabled(true); //菜单栏的退出按钮设置为 true

    auto_flag = false;
    cmd_data = 0;
    //连接到打开串口的槽函数
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::on_connectButton_clicked);

    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);//exit 连接到 close
    connect(ui->actionConnect, &QAction::triggered, m_settingsDialog, &QDialog::show);//连接到提示框的show
}



//定时器超时之后读取从机的超时槽函数
void MainWindow::Read_data()
{
    if (!modbusDevice) //如果为空 则直接返回
        return;

    //从站地址设置为1
    if (auto *reply = modbusDevice->sendReadRequest(readRequest(), 1))
    {
        //如果请求完成，那么返回ture 取反之后就是false
        if (!reply->isFinished())
            //连接modbus的结束信号，连接到主界面的读准备信号
            connect(reply, &QModbusReply::finished, this, &MainWindow::readReady);
        else //如果是广播信号删除reply
            delete reply; // broadcast replies return immediately
    } else //如果读取数据失败
    {
        //显示读取错误的信息
        statusBar()->showMessage(tr("Read error: ") + modbusDevice->errorString(), 5000);
    }
}

void MainWindow::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    //如果没有错误
    if (reply->error() == QModbusDevice::NoError)
    {
        // uint 是读取到的数据
        const QModbusDataUnit unit = reply->result();

        cmd_data = unit.value(ctrl_address);
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
    //如果是协议错误
    else if (reply->error() == QModbusDevice::ProtocolError) {
        statusBar()->showMessage(tr("Read response error: %1 (Mobus exception: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->rawResult().exceptionCode(), -1, 16), 5000);
    }
    //如果是其他错误
    else {
        statusBar()->showMessage(tr("Read response error: %1 (code: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->error(), -1, 16), 5000);
    }

    //删除reply
    reply->deleteLater();
}

QModbusDataUnit MainWindow::readRequest() const
{
    return QModbusDataUnit(QModbusDataUnit::HoldingRegisters, startAddress, numberOfEntries);
}

//串口断开连接的槽函数
void MainWindow::Disconut_serial()
{
    modbusDevice->disconnectDevice(); //断开连接

    ui->actionConnect->setEnabled(true); //连接按钮 设置为可选中
    ui->actionDisconnect->setEnabled(false);//断开按钮设置为不可选中
    //停止定时器
    read_time->stop();
    //设置串口打开的标志位是flase
    connect_flag = false;
}

//状态转换的槽函数
void MainWindow::onStateChanged(int state)
{
    //connected 的值 与state != QModbusDevice::UnconnectedState 比较来得到的
    //如果stat是连接 则 connected 为true
    //如果stat是未连接  connected是 false
    bool connected = (state != QModbusDevice::UnconnectedState);
    //如果stat是连接 则按钮不可选中
    ui->actionConnect->setEnabled(!connected);
    ui->actionDisconnect->setEnabled(connected);

    //如果state是未连接 则按钮显示 打开串口
    if (state == QModbusDevice::UnconnectedState){
        read_time->stop();
        ui->connectButton->setText(tr("打开串口"));
        }
    //如果state是连接 则按钮显示 未连接
    else if (state == QModbusDevice::ConnectedState){
        read_time->start();
        ui->connectButton->setText(tr("关闭串口"));
        }
}


void MainWindow::on_connectButton_clicked()
{
    if (!modbusDevice)
    {
        return;
    }

    statusBar()->clearMessage();

    //如果当前不是连接状态
    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
            //设置连接的端口号
            modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                ui->comBox->currentText());//自动识别的方式

            //设置连接的 校验位 波特率 数据位 停止位
            modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,
                m_settingsDialog->settings().parity);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                m_settingsDialog->settings().baud);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                m_settingsDialog->settings().dataBits);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                m_settingsDialog->settings().stopBits);

            //设置modbus的超时时间
            modbusDevice->setTimeout(m_settingsDialog->settings().responseTime);
            //设置重试次数
            modbusDevice->setNumberOfRetries(m_settingsDialog->settings().numberOfRetries);

            //检查连接状态
            if (!modbusDevice->connectDevice())
            {
                //显示连接失败
                statusBar()->showMessage(tr("Connect failed: ") + modbusDevice->errorString(), 5000);
            }
            else
            {
                qDebug()<<"连接成功";
                //read_time->start();
                ui->actionConnect->setEnabled(false);//连接按钮设置为不可选中
                ui->actionDisconnect->setEnabled(true);//断开按钮设置可选中
                statusBar()->showMessage(tr("连接成功 ") , 5000);

                //连接成功之后立即把远程使能位打开
                //open_cmd();
            }
    }
    else {  //如果连接状态是已经连接
           modbusDevice->disconnectDevice(); //断开连接
           ui->actionConnect->setEnabled(true); //连接按钮 设置为可选中
      }
}

void MainWindow::open_cmd(void)
{
    if (!modbusDevice)
        return;
    quint16 cmd_address = 49;
    QModbusDataUnit writeUnit = writeRequest(cmd_address,1);
    writeUnit.setValue(0,1<<7); //bit7设置为1

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, 1))
    {
        //正常完成返回true 取反之后就是false
        if (!reply->isFinished())
        {
            //lamdba的connect 函数
            //传输结束的信号，接收槽是this 捕获列表是 [this, reply] 既当前窗口和reply
            connect(reply, &QModbusReply::finished, this, [this, reply]()
            {
                //如果是协议错误
                if (reply->error() == QModbusDevice::ProtocolError)
                {
                    //打印错误信息
                    statusBar()->showMessage(tr("Write response error: %1 (Mobus exception: 0x%2)")
                        .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16),
                        5000);
                }
                //如果是其他错误，打印错误信息
                else if (reply->error() != QModbusDevice::NoError)
                {
                    statusBar()->showMessage(tr("Write response error: %1 (code: 0x%2)").
                        arg(reply->errorString()).arg(reply->error(), -1, 16), 5000);
                }
                //释放reply
                reply->deleteLater();
            });
        }
        else
        {
            //释放掉reply
            // broadcast replies return immediately
            reply->deleteLater();
        }
    }
    else //如果读取失败则显示错误信息，显示时间5秒
    {
        reply->deleteLater();
        statusBar()->showMessage(tr("Write error: ") + modbusDevice->errorString(), 5000);
    }
}

QModbusDataUnit MainWindow::writeRequest(quint16 add,quint16 lengh) const
{
    return QModbusDataUnit(QModbusDataUnit::HoldingRegisters, startAddress+add, lengh);
}

//写数据的统一处理函数
void MainWindow::write_cmd(quint16 cmd_address, quint16 lengh, quint16* data)
{
    if (!modbusDevice)
        return;

    QModbusDataUnit writeUnit = writeRequest(cmd_address,lengh);

    quint16 wdata[127] = {0};
    memcpy(wdata,data,2*lengh);
//qDebug()<<"wdata:"<<wdata[0]<<"lengh:"<<lengh;
   //把要写入的数据放入到writeUnit
    for (int i = 0; i< lengh ; i++) {
      writeUnit.setValue(i,wdata[i]);//*(data)
    }
//qDebug()<<"writeUnit:"<<writeUnit.value(0);
    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, 1))
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
                    //打印错误信息
                    statusBar()->showMessage(tr("Write response error: %1 (Mobus exception: 0x%2)")
                        .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16),
                        5000);
                }
                //如果是其他错误，打印错误信息
                else if (reply->error() != QModbusDevice::NoError)
                {
                    statusBar()->showMessage(tr("Write response error: %1 (code: 0x%2)").
                        arg(reply->errorString()).arg(reply->error(), -1, 16), 5000);
                }
                //释放reply
                reply->deleteLater();
            });
        }
        else
        {
            //释放掉reply
            // broadcast replies return immediately
            reply->deleteLater();
        }
    }
    else //如果读取失败则显示错误信息，显示时间5秒
    {
        reply->deleteLater();
        statusBar()->showMessage(tr("Write error: ") + modbusDevice->errorString(), 5000);
    }

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
//qDebug()<<"qstart press:"<< cmd;
    write_cmd(30,1,&cmd);
}

void MainWindow::on_Btn_Qstart_released()
{
    quint16 cmd = cmd_data & ~(1<<8);
//qDebug()<<"qstart released："<< cmd;
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
        ui->Btn_auto->setText(tr("自动"));
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
    if (!modbusDevice)
    {
        return;
    }

    if(auto_flag == false){
        auto_flag = true;
        auto_time->start();
        ui->Btn_auto->setText(tr("停止"));
    }
    else{
        auto_flag = false;
        auto_time->stop();
        ui->Btn_auto->setText(tr("自动"));
    }
}

void MainWindow::on_Btn_remote_clicked()
{
    if (!modbusDevice)
    {
        return;
    }

    if(remote_flag == false){
        remote_flag = true;

        quint16 cmd = 0x80;

        write_cmd(49,1,&cmd);

        ui->Btn_remote->setText(tr("近控"));
    }
    else{
        remote_flag = false;

        quint16 cmd = 0x00;
        write_cmd(49,1,&cmd);
        ui->Btn_remote->setText(tr("远控"));
    }

}

void MainWindow::on_Btn_jiyi_pressed()
{
    if (!modbusDevice)
    {
        return;
    }

    quint16 cmd = 0x08;

    write_cmd(32,1,&cmd);
}

void MainWindow::on_Btn_jiyi_released()
{
    if (!modbusDevice)
    {
        return;
    }

    quint16 cmd = 0x00;

    write_cmd(32,1,&cmd);
}

void MainWindow::on_Btn_sanjiao_clicked()
{
    if (!modbusDevice)
    {
        return;
    }

    if(sanjiao_flag == false){
        sanjiao_flag = true;

        quint16 cmd = 0x01;

        write_cmd(50,1,&cmd);

        ui->Btn_sanjiao->setText(tr("三角煤失能"));
    }
    else{
        sanjiao_flag = false;

        quint16 cmd = 0x00;
        write_cmd(50,1,&cmd);
        ui->Btn_sanjiao->setText(tr("三角煤使能"));
    }
}

void MainWindow::on_Btn_lajia_pressed()
{
    if (!modbusDevice)
    {
        return;
    }

    quint16 cmd = lajia_data | 3;
    write_cmd(49,1,&cmd);
}

void MainWindow::on_Btn_lajia_released()
{
    if (!modbusDevice)
    {
        return;
    }

    quint16 cmd = lajia_data & ~(3) ;
    write_cmd(49,1,&cmd);
}
