#include "qmidimapper.h"

QMidiMapper::QMidiMapper(QObject *parent) : QObject(parent)
{

}

void QMidiMapper::setMappingState(bool value)
{
    Q_UNUSED(value);
}

void QMidiMapper::setWidget(QWidget *widget)
{
    Q_UNUSED(widget);
}

void QMidiMapper::onMidiMessageReceive(QMidiMessage *message)
{
    Q_UNUSED(message);
}

