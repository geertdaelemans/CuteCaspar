#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "Models/DeviceModel.h"
#include "DatabaseManager.h"
#include "DeviceDialog.h"

#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    // Load server information from registry
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    ui->chkAutoConnect->setChecked(settings.value("auto_connect", true).toBool());
    ui->edtHost->setText(settings.value("host", "127.0.0.1").toString());
    ui->edtPort->setText(settings.value("port", "5250").toString());
    settings.endGroup();

    loadDevice();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadDevice()
{
    ui->treeWidgetDevice->clear();
    ui->treeWidgetDevice->headerItem()->setText(1, "");
    ui->treeWidgetDevice->setColumnHidden(0, true);
    ui->treeWidgetDevice->setColumnWidth(1, 25);
    ui->treeWidgetDevice->setColumnWidth(4, 50);

    QList<DeviceModel> models = DatabaseManager::getInstance().getDevice();
    foreach (DeviceModel model, models)
    {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem(ui->treeWidgetDevice);
        treeItem->setText(0, QString("%1").arg(model.getId()));
        treeItem->setIcon(1, QIcon(":/Graphics/Images/ServerSmall.png"));
        treeItem->setText(2, model.getName());
        treeItem->setText(3, model.getAddress());
        treeItem->setText(4, QString("%1").arg(model.getPort()));
        treeItem->setText(5, model.getDescription());
        treeItem->setText(6, model.getUsername());

        QString password = model.getPassword();
        treeItem->setText(7, password.replace(QRegExp("."), "*"));

        treeItem->setText(8, model.getVersion());
        treeItem->setText(9, model.getShadow());

        if (model.getChannels() > 0)
            treeItem->setText(10, QString("%1").arg(model.getChannels()));

        treeItem->setText(11, model.getChannelFormats());

        if (model.getPreviewChannel() > 0)
            treeItem->setText(12, QString("%1").arg(model.getPreviewChannel()));

        if (model.getLockedChannel() > 0)
            treeItem->setText(13, QString("%1").arg(model.getLockedChannel()));
    }

    checkEmptyDeviceList();
}

/**
 * @brief Check if Device List is empty
 * When the device list is empty open the device list pane and highlight the list
 */
void SettingsDialog::checkEmptyDeviceList()
{
    if (ui->treeWidgetDevice->invisibleRootItem()->childCount() == 0)
    {
        ui->tabWidgetSettings->setCurrentIndex(1);
        ui->treeWidgetDevice->setStyleSheet("border-color: firebrick;");
    }
    else
        ui->treeWidgetDevice->setStyleSheet("");
}


void SettingsDialog::showAddDeviceDialog()
{
    DeviceDialog* dialog = new DeviceDialog(this);
    if (dialog->exec() == QDialog::Accepted)
    {
        DatabaseManager::getInstance().insertDevice(DeviceModel(0, dialog->getName(), dialog->getAddress(),
                                                                dialog->getPort().toInt(), dialog->getUsername(),
                                                                dialog->getPassword(), dialog->getDescription(),
                                                                "", dialog->getShadow(), 0, "", dialog->getPreviewChannel(),
                                                                dialog->getLockedChannel()));

        loadDevice();

//        EventManager::getInstance().fireRefreshLibraryEvent(RefreshLibraryEvent());
    }
}

/**
 * @brief SettingsDialog::removeDevice
 * Remove the selected device from the Device database
 */
void SettingsDialog::removeDevice()
{
    if (ui->treeWidgetDevice->selectedItems().count() == 0)
        return;

    DatabaseManager::getInstance().deleteDevice(ui->treeWidgetDevice->currentItem()->text(0).toInt());
    delete ui->treeWidgetDevice->currentItem();

    loadDevice();

//    EventManager::getInstance().fireRefreshLibraryEvent(RefreshLibraryEvent());
}


void SettingsDialog::deviceItemDoubleClicked(QTreeWidgetItem* current, int index)
{
    Q_UNUSED(index);

    DeviceModel model = DatabaseManager::getInstance().getDeviceById(current->text(0).toInt());

    DeviceDialog* dialog = new DeviceDialog(this);
    dialog->setDeviceModel(model);
    if (dialog->exec() == QDialog::Accepted)
    {
        DatabaseManager::getInstance().updateDevice(DeviceModel(model.getId(), dialog->getName(), dialog->getAddress(),
                                                                dialog->getPort().toInt(), dialog->getUsername(),
                                                                dialog->getPassword(), dialog->getDescription(),
                                                                model.getVersion(), dialog->getShadow(),
                                                                model.getChannels(), model.getChannelFormats(),
                                                                dialog->getPreviewChannel(), dialog->getLockedChannel()));

        loadDevice();

//        EventManager::getInstance().fireRefreshLibraryEvent(RefreshLibraryEvent());
    }
}


void SettingsDialog::on_buttonBox_accepted()
{
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    settings.setValue("auto_connect", ui->chkAutoConnect->isChecked());
    settings.setValue("host", ui->edtHost->text());
    settings.setValue("port", ui->edtPort->text());
    settings.endGroup();
}
