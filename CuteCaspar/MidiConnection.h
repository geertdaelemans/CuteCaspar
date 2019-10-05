#ifndef MIDICONNECTION_H
#define MIDICONNECTION_H

#include "qmidiin.h"
#include "qmidiout.h"
#include "qmidimessage.h"

class MidiConnection : public QObject
{

    Q_OBJECT

public:
    MidiConnection();
    static MidiConnection *getInstance();
    void reportAvailableMidiPorts();
    QStringList getAvailableInputPorts();
    QStringList getAvailableOutputPorts();
    void openInputPort(int index);
    void openOutputPort(int index);
    void playNote(unsigned int pitch);
    void killNote(unsigned int pitch);
    int getOpenInputPortIndex() const;
    int getOpenOutputPortIndex() const;

public slots:
    void messageReceived(QMidiMessage *msg);

signals:
    void midiMessageReceived(unsigned int pitch, bool onOff);

private:
    static MidiConnection* s_inst;
    QMidiIn* midiIn;
    QMidiOut* midiOut;
    QString currentInputPortName = nullptr;
    QString currentOutputPortName = nullptr;
    int currentInputPortIndex = -1;
    int currentOutputPortIndex = -1;
};

#endif // MIDICONNECTION_H
