#include "RaspberryPIDialog.h"
#include "ui_RaspberryPIDialog.h"

#include "RaspberryPI.h"
#include "SettingsDialog.h"

#include <QDebug>
#include <QMessageBox>


RaspberryPIDialog::RaspberryPIDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RaspberryPIDialog)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    // Check connection status
    if (RaspberryPI::getInstance()->isConnected()) {
        ui->btnConnect->setText("Quit");
        ui->btnButton->setEnabled(true);
        ui->btnMagnet->setEnabled(true);

        // Initialize button status
        if (RaspberryPI::getInstance()->isButtonActive()) {
            setButtonColor(ui->btnButton, Qt::darkGreen);
        } else {
            setButtonColor(ui->btnButton, Qt::darkRed);
        }

        // Initialize magnet status
        if (RaspberryPI::getInstance()->isMagnetActive()) {
            setButtonColor(ui->btnMagnet, Qt::darkGreen);
        } else {
            setButtonColor(ui->btnMagnet, Qt::darkRed);
        }
    } else {
        ui->btnConnect->setText("Connect");
        setButtonColor(ui->btnButton, QColor(53,53,53));
        setButtonColor(ui->btnMagnet, QColor(53,53,53));
    }
}

RaspberryPIDialog::~RaspberryPIDialog()
{
    delete ui;
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
    if (ui->btnConnect->text() == "Connect") {
        if (RaspberryPI::getInstance()->isConnected()) {
            ui->btnConnect->setText("Quit");
            ui->btnButton->setEnabled(true);
            ui->btnMagnet->setEnabled(true);
            // Initialize button status
            RaspberryPI::getInstance()->setButtonActive(true);
        } else {
            QMessageBox msgBox;
            msgBox.setText("Cannot connect to RaspberryPI.");
            msgBox.setWindowTitle("Warning");
            msgBox.setWindowFlag(Qt::FramelessWindowHint, false);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.addButton(QMessageBox::Ok);
            QPushButton* settingsButton = msgBox.addButton(tr("Settings..."), QMessageBox::ActionRole);
            msgBox.exec();
            if (msgBox.clickedButton() == settingsButton) {
                SettingsDialog::getInstance()->exec();
            }
        }
    } else {
        RaspberryPI::getInstance()->stopConnection();
        ui->btnConnect->setText("Connect");
        ui->btnButton->setEnabled(false);
        ui->btnMagnet->setEnabled(false);
        setButtonColor(ui->btnButton, QColor(53,53,53));
        setButtonColor(ui->btnMagnet, QColor(53,53,53));
    }
}

void RaspberryPIDialog::on_btnMagnet_clicked()
{
    if (!RaspberryPI::getInstance()->isMagnetActive()) {
        RaspberryPI::getInstance()->setMagnetActive(true);
    } else {
        RaspberryPI::getInstance()->setMagnetActive(false);
    }
}

void RaspberryPIDialog::setButtonColor(QPushButton* button, QColor color)
{
    QPalette pal = button->palette();
    pal.setColor(QPalette::Button, color);
    button->setPalette(pal);
    button->update();
}


void RaspberryPIDialog::on_btnButton_clicked()
{
    if (!RaspberryPI::getInstance()->isButtonActive()) {
        RaspberryPI::getInstance()->setButtonActive(true);
    } else {
        RaspberryPI::getInstance()->setButtonActive(false);
    }
}

void RaspberryPIDialog::refreshUpdate(status stat)
{
    qDebug() << "Connected" << stat.connected;
    if (stat.buttonActive) {
        setButtonColor(ui->btnButton, Qt::darkGreen);
    } else {
        setButtonColor(ui->btnButton, Qt::darkRed);
    }
    if (stat.magnetActive) {
        setButtonColor(ui->btnMagnet, Qt::darkGreen);
    } else {
        setButtonColor(ui->btnMagnet, Qt::darkRed);
    }
}
