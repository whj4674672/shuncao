#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QComboBox>


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    //设置默认的校验方式
    ui->parityCombo->setCurrentIndex(0);
    //下面的设置都是枚举类型，实际值就是对应的数字
    ui->baudCombo->setCurrentText(QString::number(m_settings.baud));
    ui->dataBitsCombo->setCurrentText(QString::number(m_settings.dataBits));
    ui->stopBitsCombo->setCurrentText(QString::number(m_settings.stopBits));

    connect(ui->applyButton, &QPushButton::clicked, [this]()
    {
        //设置校验位
        m_settings.parity = ui->parityCombo->currentIndex();
        //如果校验位大于0 就要加1 因为枚举类型里面没有1
        if (m_settings.parity > 0)
            m_settings.parity++;
        //把combox里面的值转换成int类型之后赋值给baud
        m_settings.baud = ui->baudCombo->currentText().toInt();
        //赋值给数据位
        m_settings.dataBits = ui->dataBitsCombo->currentText().toInt();
        //赋值给停止位
        m_settings.stopBits = ui->stopBitsCombo->currentText().toInt();

        //隐藏  这里为什么是隐藏 而不是删除,自己不能删除自己，只能在主任务delete
        hide();
    });
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

SettingsDialog::Settings SettingsDialog::settings() const
{
    //直接返回settting值
    return m_settings;
}


