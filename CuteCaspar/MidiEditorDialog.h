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
    void setClipName(QString clipName, bool insert = false);
    void activeClip(ClipInfo activeClip, ClipInfo upcomingClip, bool insert = false);
    void playerStatus(PlayerStatus status, bool recording);

private slots:
    void on_btnDelete_clicked();
    void on_btnAdd_clicked();
    void on_btnSave_clicked();
    void on_btnResume_clicked();

private:
    Ui::MidiEditorDialog *ui;
    QStandardItemModel* m_model = nullptr;
    QMap<QString, message>* m_midiPlaylist = nullptr;
    void addNewNote(QString timecode, bool noteOn, unsigned int pitch);
    int m_currentIndex = 0;
    QString m_clipName;
    PlayerStatus m_playerStatus;
};

#endif // MIDIEDITORDIALOG_H
