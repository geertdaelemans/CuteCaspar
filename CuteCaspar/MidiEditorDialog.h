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
    void newMidiPlaylist(QMap<QString, message> midiPlayList, double timecode);
    void currentNote(QString timecode, bool noteOn, unsigned int pitch);
    void setClip(ClipInfo clip);
    void playerStatus(PlayerStatus status, bool recording);

private slots:
    void on_btnDelete_clicked();
    void on_btnAdd_clicked();
    void on_btnSave_clicked();
    void on_btnResume_clicked();
    void timecode(double time, double duration, int videoLayer);

private:
    Ui::MidiEditorDialog *ui;
    QStandardItemModel* m_model = nullptr;
    QMap<QString, message>* m_midiPlaylist = nullptr;
    int m_currentIndex = 0;
    ClipInfo m_activeClip;
    PlayerStatus m_playerStatus;
    QString m_timecode;
    void addNewNote(QString timecode, bool noteOn, unsigned int pitch);
};

#endif // MIDIEDITORDIALOG_H
