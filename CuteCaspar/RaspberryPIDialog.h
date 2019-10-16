#ifndef RASPBERRYPIDIALOG_H
#define RASPBERRYPIDIALOG_H

#include <QDialog>

namespace Ui {
class RaspberryPIDialog;
}

class RaspberryPIDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RaspberryPIDialog(QWidget *parent = nullptr);
    ~RaspberryPIDialog();

public slots:
    void statusButton(QString msg);

private slots:
    void on_btnQuit_clicked();
    void on_btnLed_clicked();

signals:
    void sendMessage(QString msg);

private:
    Ui::RaspberryPIDialog *ui;
    bool ledOn = false;
};

#endif // RASPBERRYPIDIALOG_H
