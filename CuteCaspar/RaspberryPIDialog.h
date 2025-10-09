#ifndef RASPBERRYPIDIALOG_H
#define RASPBERRYPIDIALOG_H

#include <QDialog>
#include <QStatusBar>
#include <QTimer>
#include <QElapsedTimer>

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
    void on_btnMotion_clicked();
    void on_btnSmoke_clicked();
    void on_btnReboot_clicked();
    void on_btnShutdown_clicked();
    void on_sldMagnet_valueChanged(int value);

private:
    Ui::RaspberryPIDialog *ui;
    QStatusBar* statusBar;
    void setButtonColor(QPushButton *button, QColor color);
    void setButton(QPushButton *button, bool state);
    
    // Debouncing support
    QElapsedTimer m_lastClickTimer;
    static const int DEBOUNCE_DELAY_MS = 300; // 300ms debounce delay
    bool isClickAllowed(); // Helper function for debouncing
};

#endif // RASPBERRYPIDIALOG_H
