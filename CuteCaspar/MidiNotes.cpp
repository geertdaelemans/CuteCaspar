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
        m_notes.append(tempNote);
        counter++;
    }

}

QList<note> MidiNotes::getNotes() const
{
    return m_notes;
}

int MidiNotes::getNumberOfNotes() const
{
    return m_notes.size();
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
