#include "LibraryModel.h"

LibraryModel::LibraryModel(int id, const QString& label, const QString& name, const QString& deviceName, const QString& type, int thumbnailId, const QString& timecode, const double fps, const int midi)
    : id(id), label(label), name(name), deviceName(deviceName), type(type), thumbnailId(thumbnailId), timecode(timecode), fps(fps), midi(midi)
{
}

int LibraryModel::getId() const
{
    return this->id;
}

const QString& LibraryModel::getLabel() const
{
    return this->label;
}

const QString& LibraryModel::getName() const
{
    return this->name;
}

const QString& LibraryModel::getDeviceName() const
{
    return this->deviceName;
}

const QString& LibraryModel::getType() const
{
    return this->type;
}

const QString& LibraryModel::getTimecode() const
{
    return this->timecode;
}

double LibraryModel::getFPS() const
{
    return this->fps;
}

int LibraryModel::getMidi() const
{
    return this->midi;
}

void LibraryModel::setLabel(const QString& label)
{
    this->label = label;
}

void LibraryModel::setName(const QString& name)
{
    this->name = name;
}

void LibraryModel::setDeviceName(const QString& deviceName)
{
    this->deviceName = deviceName;
}

void LibraryModel::setTimecode(const QString& timecode)
{
    this->timecode = timecode;
}

void LibraryModel::setMidi(const int midi)
{
    this->midi = midi;
}

int LibraryModel::getThumbnailId() const
{
    return this->thumbnailId;
}
