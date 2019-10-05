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
    QString midiInName = settings.value("midiin", "").toString();
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

    if (midiInName != "") {
        QStringList inputs = getAvailableInputPorts();
        if (inputs.size() > 0) {
            int index = inputs.indexOf(midiInName);
            index = (index == -1 ? 0 : index);
            openInputPort(index);
        }
    }

    connect(midiIn, SIGNAL(midiMessageReceived(QMidiMessage*)),
            this, SLOT(messageReceived(QMidiMessage*)));
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

void MidiConnection::openInputPort(int index)
{
    if(midiIn->isPortOpen()) {
        midiIn->closePort();
    }
    midiIn->openPort(static_cast<unsigned int>(index));
    currentInputPortName = getAvailableInputPorts()[index];
    currentInputPortIndex = index;
    qDebug() << "Opened MIDI input" << currentInputPortName;

    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    settings.setValue("midiin", currentInputPortName);
    settings.endGroup();
}


void MidiConnection::openOutputPort(int index)
{
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


int MidiConnection::getOpenInputPortIndex() const {
    return currentInputPortIndex;
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

    midiOut->sendMessage(message);
}


void MidiConnection::killNote(unsigned int pitch)
{
    QMidiMessage *message = new QMidiMessage();
    message->setChannel(1);
    message->setStatus(MIDI_NOTE_OFF);
    message->setPitch(pitch);
    message->setVelocity(0);

    midiOut->sendMessage(message);
}


void MidiConnection::reportAvailableMidiPorts() {
    qDebug() << "MIDI inputs" << midiIn->getPorts();
    qDebug() << "MIDI outputs" << midiOut->getPorts();
}

void MidiConnection::messageReceived(QMidiMessage* msg)
{
    switch (msg->getStatus()) {
    case MIDI_NOTE_ON:
        if (msg->getVelocity() == 0) {
            emit midiMessageReceived(msg->getPitch(), false);
        } else {
            // For Yamaha interpretation of MIDI_NOTE_ON with velocity 0
            emit midiMessageReceived(msg->getPitch(), true);
        }
        break;
    case MIDI_NOTE_OFF:
        emit midiMessageReceived(msg->getPitch(), false);
        break;
    default:
        qDebug() << "Not supported" << msg->getRawMessage();
        break;
    }
}
