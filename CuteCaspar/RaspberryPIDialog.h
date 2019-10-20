#ifndef RASPBERRYPIDIALOG_H
#define RASPBERRYPIDIALOG_H

#include <QDialog>

#include "RaspberryPI.h"

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
    void refreshUpdate(status stat);

private slots:
    void on_btnConnect_clicked();
    void on_btnMagnet_clicked();
    void on_btnButton_clicked();
    void on_btnLight_clicked();

private:
    Ui::RaspberryPIDialog *ui;
    void setButtonColor(QPushButton *button, QColor color);
};

#endif // RASPBERRYPIDIALOG_H
