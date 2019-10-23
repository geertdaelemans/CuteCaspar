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

private slots:
    void on_btnDelete_clicked();
    void on_btnAdd_clicked();
    void on_btnSave_clicked();

signals:
    void saveMidiPlayList(QMap<QString, message> midiPlayList);

private:
    Ui::MidiEditorDialog *ui;
    QStandardItemModel* m_model = nullptr;
    QMap<QString, message> m_newMidiPlaylist;
    void addNewNote(QString timecode, bool noteOn, unsigned int pitch);
    int m_currentIndex = 0;
};

#endif // MIDIEDITORDIALOG_H
