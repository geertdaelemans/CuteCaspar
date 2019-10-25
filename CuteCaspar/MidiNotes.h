#ifndef MIDINOTES_H
#define MIDINOTES_H

#include <QObject>

struct note {
    int id;
    QString name;
    unsigned int pitch;
    unsigned int duration;
    unsigned int next;
};

class MidiNotes : public QObject
{
    Q_OBJECT

public:
    MidiNotes();
    static MidiNotes* getInstance();
    QList<note> getNotes() const;
    int getNumberOfNotes() const;
    QString getNoteNameByPitch(unsigned int pitch) const;
    unsigned int getNotePitchByName(QString name) const;
    unsigned int getDuration(unsigned int) const;
    unsigned int getNext(unsigned int pitch) const;

private:
    static MidiNotes* s_inst;
    void loadNotes();
    QList<note> m_notes;
};

#endif // MIDINOTES_H
