#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "ui_SettingsDialog.h"

#include <QtWidgets/QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void on_buttonBox_accepted();
    void showAddDeviceDialog();
    void removeDevice();
    void deviceItemDoubleClicked(QTreeWidgetItem *current, int index);
    void openInputPort(int index);
    void openOutputPort(int index);

private:
    Ui::SettingsDialog *ui;
    void loadDevice();
    void checkEmptyDeviceList();
};

#endif // SETTINGSDIALOG_H
