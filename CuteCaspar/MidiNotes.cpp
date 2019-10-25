#include "MidiNotes.h"

#include <QtCore>

MidiNotes* MidiNotes::s_inst = nullptr;

MidiNotes::MidiNotes()
{
    loadNotes();
}

MidiNotes* MidiNotes::getInstance()
{
    if (!s_inst) {
        s_inst = new MidiNotes();
    }
    return s_inst;
}

void MidiNotes::loadNotes()
{
    // Load notes prolfile
    QFile profile;
    if(QFileInfo::exists("Notes.csv")) {
        profile.setFileName("Notes.csv");
    } else {
        profile.setFileName(":/Profiles/Notes.csv");
    }
    if (!profile.open(QIODevice::ReadOnly)) {
        qDebug() << profile.errorString();
    }

    // Put all notes information in QList
    int counter = 0;
    while (!profile.atEnd()) {
        QByteArray line = profile.readLine();
        note tempNote;
        tempNote.id = counter;
        tempNote.name = line.split(',').at(0);
        tempNote.pitch = line.split(',').at(1).toUInt();
        tempNote.duration = line.split(',').at(2).toUInt();
        tempNote.next = line.split(',').at(3).toUInt();
        m_notes.append(tempNote);
        counter++;
    }

    // Add RaspberryPI actions
    m_notes.append({counter++, "Button", 129, 0, 0});
    m_notes.append({counter++, "Light" , 130, 0, 0});
    m_notes.append({counter++, "Magnet", 131, 0, 0});
    m_notes.append({counter++, "Motion", 132, 0, 0});
    m_notes.append({counter++, "Smoke",  133, 0, 0});
}

QList<note> MidiNotes::getNotes() const
{
    return m_notes;
}

int MidiNotes::getNumberOfNotes() const
{
    return m_notes.size();
}

unsigned int MidiNotes::getDuration(unsigned int pitch) const
{
    unsigned int duration = 0;
    for (int i = 0; i < m_notes.size(); i++) {
        if (m_notes[i].pitch == pitch) {
            return m_notes[i].duration;
        }
    }
    return duration;
}

unsigned int MidiNotes::getNext(unsigned int pitch) const
{
    unsigned int next = 0;
    for (int i = 0; i < m_notes.size(); i++) {
        if (m_notes[i].pitch == pitch) {
            return m_notes[i].next;
        }
    }
    return next;
}

QString MidiNotes::getNoteNameByPitch(unsigned int pitch) const
{
    QString output;
    for (int i = 0; i < m_notes.size(); i++) {
        if (m_notes[i].pitch == pitch) {
            return m_notes[i].name;
        }
    }
    return output;
}

unsigned int MidiNotes::getNotePitchByName(QString name) const
{
    unsigned int output = 0;
    for (int i = 0; i < m_notes.size(); i++) {
        if (m_notes[i].name == name) {
            return m_notes[i].pitch;
        }
    }
    return output;
}
