#ifndef MIDICONNECTION_H
#define MIDICONNECTION_H

#include "qmidiin.h"
#include "qmidiout.h"
#include "qmidimessage.h"

class MidiConnection
{
public:
    MidiConnection();
    static MidiConnection *getInstance();
    void reportAvailableMidiPorts();
    QStringList getAvailableInputPorts();
    QStringList getAvailableOutputPorts();
    void openOutputPort(int index);
    void playNote(unsigned int pitch);
    void killNote(unsigned int pitch);
    int getOpenOutputPortIndex() const;

private:
    static MidiConnection* s_inst;
    QMidiIn* midiIn;
    QMidiOut* midiOut;
    QString currentOutputPortName = nullptr;
    int currentOutputPortIndex = -1;
};

#endif // MIDICONNECTION_H
