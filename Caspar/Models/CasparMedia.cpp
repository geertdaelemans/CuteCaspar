#include "CasparMedia.h"

CasparMedia::CasparMedia(const QString& name, const QString& type, const QString& timecode, const double fps)
    : name(name), type(type), timecode(timecode), fps(fps)
{
}

const QString& CasparMedia::getName() const
{
    return this->name;
}

const QString& CasparMedia::getType() const
{
    return this->type;
}

const QString& CasparMedia::getTimecode() const
{
    return this->timecode;
}

double CasparMedia::getFPS() const
{
    return this->fps;
}
