#include "DeviceManager.h"
#include "DatabaseManager.h"

Q_GLOBAL_STATIC(DeviceManager, deviceManager)

DeviceManager::DeviceManager()
{
}

DeviceManager& DeviceManager::getInstance()
{
    return *deviceManager();
}

void DeviceManager::initialize()
{
    QList<DeviceModel> models = DatabaseManager::getInstance().getDevice();
    foreach (const DeviceModel& model, models)
    {
        QSharedPointer<CasparDevice> device(new CasparDevice(model.getAddress(), model.getPort()));

        this->deviceModels.insert(model.getName(), model);
        this->devices.insert(model.getName(), device);

        emit deviceAdded(*device);

        device->connectDevice();
    }
}
