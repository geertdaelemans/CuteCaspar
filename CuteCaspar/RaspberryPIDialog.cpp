#include "RaspberryPIDialog.h"
#include "ui_RaspberryPIDialog.h"

#include <QDebug>


RaspberryPIDialog::RaspberryPIDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RaspberryPIDialog)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);
}

RaspberryPIDialog::~RaspberryPIDialog()
{
    delete ui;
}

void RaspberryPIDialog::on_btnLed_clicked()
{
    if (m_ledOn) {
        emit sendMessage("off");
        ui->btnLed->setText("LED ON");
        m_ledOn = false;
    } else {
        emit sendMessage("on");
        ui->btnLed->setText("LED OFF");
        m_ledOn = true;
    }

}

void RaspberryPIDialog::statusButton(QString msg)
{
    if (msg == "high") {
        QPixmap pixmap(":/Images/2) Red button Hover.ico");
        ui->lblButton->setPixmap(pixmap);
        ui->lblButton->show();
    } else if (msg == "low") {
        QPixmap pixmap(":/Images/3) Red button Pressed.ico");
        ui->lblButton->setPixmap(pixmap);
        ui->lblButton->show();
    }
}

void RaspberryPIDialog::on_btnConnect_clicked()
{
    if (!m_isConnected) {
        emit sendMessage("start");
        ui->btnConnect->setText("Disconnect");
        ui->btnLed->setEnabled(true);
        m_isConnected = true;
    } else {
        emit sendMessage("quit");
        ui->btnConnect->setText("Connect");
        ui->btnLed->setEnabled(false);
        m_isConnected = false;
    }
}
