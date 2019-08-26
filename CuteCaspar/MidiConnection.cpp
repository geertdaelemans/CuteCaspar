#include "MidiConnection.h"

#include <QDebug>
#include <QSettings>
#include <QPushButton>

MidiConnection* MidiConnection::s_inst = nullptr;

MidiConnection::MidiConnection()
{
    // Prepare MIDI connection
    midiIn = new QMidiIn();
    midiOut = new QMidiOut();

    // Load server information from registry
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    qDebug() << settings.value("midiin", "").toString();
    QString midiOutName = settings.value("midiout", "").toString();
    settings.endGroup();

    if (midiOutName != "") {
        QStringList outputs = getAvailableOutputPorts();
        if (outputs.size() > 0) {
            int index = outputs.indexOf(midiOutName);
            index = (index == -1 ? 0 : index);
            openOutputPort(index);
        }
    }

    reportAvailableMidiPorts();
}


MidiConnection* MidiConnection::getInstance()
{
    if (!s_inst) {
        s_inst = new MidiConnection();
    }
    return s_inst;
}


QStringList MidiConnection::getAvailableInputPorts() {
    return midiIn->getPorts();
}


QStringList MidiConnection::getAvailableOutputPorts() {
    return midiOut->getPorts();
}


void MidiConnection::openOutputPort(int index) {
    if(midiOut->isPortOpen()) {
        midiOut->closePort();
    }
    midiOut->openPort(static_cast<unsigned int>(index));
    currentOutputPortName = getAvailableOutputPorts()[index];
    currentOutputPortIndex = index;
    qDebug() << "Opened MIDI output" << currentOutputPortName;

    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    settings.setValue("midiout", currentOutputPortName);
    settings.endGroup();
}

int MidiConnection::getOpenOutputPortIndex() const {
    return currentOutputPortIndex;
}


void MidiConnection::playNote(unsigned int pitch)
{
    QMidiMessage *message = new QMidiMessage();
    message->setChannel(1);
    message->setStatus(MIDI_NOTE_ON);
    message->setPitch(pitch);
    message->setVelocity(60);

    qDebug() << "ON: pitch" << message->getPitch() << "velocity" << message->getVelocity();

    midiOut->sendMessage(message);
}


void MidiConnection::killNote(unsigned int pitch)
{
    QMidiMessage *message = new QMidiMessage();
    message->setChannel(1);
    message->setStatus(MIDI_NOTE_OFF);
    message->setPitch(pitch);
    message->setVelocity(0);

    qDebug() << "OFF: pitch" << message->getPitch();

    midiOut->sendMessage(message);
}


void MidiConnection::reportAvailableMidiPorts() {
    qDebug() << "MIDI inputs" << midiIn->getPorts();
    qDebug() << "MIDI outputs" << midiOut->getPorts();
}
