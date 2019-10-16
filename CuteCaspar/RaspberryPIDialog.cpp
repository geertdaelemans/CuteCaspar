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

void RaspberryPIDialog::on_btnQuit_clicked()
{
    emit sendMessage("quit");
}

void RaspberryPIDialog::on_btnLed_clicked()
{
    if (ledOn) {
        emit sendMessage("off");
        ui->btnLed->setText("LED ON");
        ledOn = false;
    } else {
        emit sendMessage("on");
        ui->btnLed->setText("LED OFF");
        ledOn = true;
    }

}

void RaspberryPIDialog::statusButton(QString msg)
{
    if (msg == "High") {
        QPixmap pixmap(":/Images/2) Red button Hover.ico");
        ui->lblButton->setPixmap(pixmap);
        ui->lblButton->show();
    } else {
        QPixmap pixmap(":/Images/3) Red button Pressed.ico");
        ui->lblButton->setPixmap(pixmap);
        ui->lblButton->show();
    }
}
