#ifndef MIDIEDITORDIALOG_H
#define MIDIEDITORDIALOG_H

#include <QDialog>
#include <QStandardItemModel>

#include "Player.h"

namespace Ui {
class MidiEditorDialog;
}

class MidiEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MidiEditorDialog(QWidget *parent = nullptr);
    ~MidiEditorDialog();

public slots:
    void newMidiPlaylist(QMap<QString, message> midiPlayList);
    void currentNote(QString timecode, bool noteOn, unsigned int pitch);

private:
    Ui::MidiEditorDialog *ui;
    QStandardItemModel* m_model = nullptr;
    QMap<QString, message> m_newMidiPlaylist;
    void addNewNote(QString timecode, bool noteOn, unsigned int pitch);
};

#endif // MIDIEDITORDIALOG_H
