#include "RaspberryPIDialog.h"
#include "ui_RaspberryPIDialog.h"

#include "RaspberryPI.h"
#include "SettingsDialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QStatusBar>
#include <QCloseEvent>


RaspberryPIDialog::RaspberryPIDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RaspberryPIDialog)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    // Set status bar
    statusBar = new QStatusBar(this);
    ui->statusBar->addWidget(statusBar);

    // Check connection status
    if (RaspberryPI::getInstance()->isConnected()) {

        // Enable all buttons
        ui->btnConnect->setText("Quit");
        ui->btnButton->setEnabled(true);
        ui->btnLight->setEnabled(true);
        ui->btnMagnet->setEnabled(true);
        ui->btnMotion->setEnabled(true);
        ui->btnSmoke->setEnabled(true);

        // Initialize buttons
        setButton(ui->btnButton, RaspberryPI::getInstance()->isButtonActive());
        setButton(ui->btnLight, RaspberryPI::getInstance()->isLightActive());
        setButton(ui->btnMagnet, RaspberryPI::getInstance()->isMagnetActive());
        setButton(ui->btnMotion, RaspberryPI::getInstance()->isMotionActive());
        setButton(ui->btnSmoke, RaspberryPI::getInstance()->isSmokeActive());

        // Statusbar
        statusBar->showMessage("Raspberry PI connected");

    } else {
        ui->btnConnect->setText("Connect");

        // Disable all buttons
        setButtonColor(ui->btnButton, QColor(53,53,53));
        setButtonColor(ui->btnLight, QColor(53,53,53));
        setButtonColor(ui->btnMagnet, QColor(53,53,53));
        setButtonColor(ui->btnMotion, QColor(53,53,53));
        setButtonColor(ui->btnSmoke, QColor(53,53,53));

        // Statusbar
        statusBar->showMessage("Raspberry PY not connected");
    }

    // Initialise the magnet probability
    ui->sldMagnet->setValue(RaspberryPI::getInstance()->getMagnetProbability());
}

RaspberryPIDialog::~RaspberryPIDialog()
{
    delete ui;
}

void RaspberryPIDialog::statusButton(QString msg)
{
    if (msg == "high") {
        QPixmap pixmap(":/Images/Red button Hover.png");
        ui->lblButton->setPixmap(pixmap);
        ui->lblButton->show();
    } else if (msg == "low") {
        QPixmap pixmap(":/Images/Red button Pressed.png");
        ui->lblButton->setPixmap(pixmap);
        ui->lblButton->show();
    }
}


/**
 * BUTTONS
 */

void RaspberryPIDialog::on_btnConnect_clicked()
{
    if (ui->btnConnect->text() == "Connect") {
        if (RaspberryPI::getInstance()->isConnected()) {
            ui->btnConnect->setText("Quit");
            ui->btnButton->setEnabled(true);
            ui->btnLight->setEnabled(true);
            ui->btnMagnet->setEnabled(true);
            ui->btnMotion->setEnabled(true);
            ui->btnSmoke->setEnabled(true);
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
        ui->btnLight->setEnabled(false);
        ui->btnMagnet->setEnabled(false);
        ui->btnMotion->setEnabled(false);
        ui->btnSmoke->setEnabled(false);
        setButtonColor(ui->btnButton, QColor(53,53,53));
        setButtonColor(ui->btnLight, QColor(53,53,53));
        setButtonColor(ui->btnMagnet, QColor(53,53,53));
        setButtonColor(ui->btnMotion, QColor(53,53,53));
        setButtonColor(ui->btnSmoke, QColor(53,53,53));
    }
}

void RaspberryPIDialog::on_btnReboot_clicked()
{
    RaspberryPI::getInstance()->reboot();
}

void RaspberryPIDialog::on_btnShutdown_clicked()
{
    RaspberryPI::getInstance()->shutdown();
}

void RaspberryPIDialog::on_btnButton_clicked()
{
    if (!RaspberryPI::getInstance()->isButtonActive()) {
        RaspberryPI::getInstance()->setButtonActive(true);
    } else {
        RaspberryPI::getInstance()->setButtonActive(false);
    }
}

void RaspberryPIDialog::on_btnLight_clicked()
{
    if (!RaspberryPI::getInstance()->isLightActive()) {
        RaspberryPI::getInstance()->setLightActive(true);
    } else {
        RaspberryPI::getInstance()->setLightActive(false);
    }
}

void RaspberryPIDialog::on_btnMagnet_clicked()
{
    if (!RaspberryPI::getInstance()->isMagnetActive()) {
        RaspberryPI::getInstance()->setMagnetActive(true);
    } else {
        RaspberryPI::getInstance()->setMagnetActive(false, true);
    }
}

void RaspberryPIDialog::on_btnMotion_clicked()
{
    if (!RaspberryPI::getInstance()->isMotionActive()) {
        RaspberryPI::getInstance()->setMotionActive(true);
    } else {
        RaspberryPI::getInstance()->setMotionActive(false);
    }
}

void RaspberryPIDialog::on_btnSmoke_clicked()
{
    if (!RaspberryPI::getInstance()->isSmokeActive()) {
        RaspberryPI::getInstance()->setSmokeActive(true);
    } else {
        RaspberryPI::getInstance()->setSmokeActive(false);
    }
}

void RaspberryPIDialog::on_sldMagnet_valueChanged(int value)
{
    RaspberryPI::getInstance()->setMagnetProbablilty(value);
}

/**
 * HELPERS
 */

void RaspberryPIDialog::setButtonColor(QPushButton* button, QColor color)
{
    QPalette pal = button->palette();
    pal.setColor(QPalette::Button, color);
    button->setPalette(pal);
    button->update();
}

void RaspberryPIDialog::refreshUpdate(status stat)
{
    qDebug("Status Received");
    statusBar->showMessage(stat.connected ? "Connected" : "Not Connected");
    setButton(ui->btnButton, stat.buttonActive);
    setButton(ui->btnLight, stat.lightActive);
    setButton(ui->btnMagnet, stat.magnetActive);
    setButton(ui->btnMotion, stat.motionActive);
    setButton(ui->btnSmoke, stat.smokeActive);
    ui->sldMagnet->setValue(stat.magnetProbability);
}

void RaspberryPIDialog::setButton(QPushButton* button, bool state)
{
    if (state) {
        setButtonColor(button, Qt::darkGreen);
    } else {
        setButtonColor(button, Qt::darkRed);
    }
}
