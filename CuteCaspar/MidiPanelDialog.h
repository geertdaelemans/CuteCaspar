#ifndef MIDIPANELDIALOG_H
#define MIDIPANELDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QMap>

#include "MidiNotes.h"

namespace Ui {
class MidiPanelDialog;
}

class MidiPanelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MidiPanelDialog(QWidget *parent = nullptr);
    ~MidiPanelDialog();

public slots:
    void activateButton(unsigned int pitch, bool active = true);

signals:
    void buttonPushed(unsigned int, bool);
    void waitNote(int duration);

private slots:
    void playNote();
    void killNote();
    void automatedSequence();
    void on_chkLive_stateChanged(int check);

private:
    Ui::MidiPanelDialog *ui;
    QList<note> notes;
    QMap<unsigned int, QPushButton*> button;
    unsigned int previousPitch = 0;
    unsigned int m_nextPitch = 0;
    void setButtonColor(QPushButton *button, QColor color);
    bool m_button = false;
    bool m_light = false;
    bool m_magnet = false;
    bool m_motion = false;
    bool m_smoke = false;
    QTimer* m_timer;
    bool m_live = true;
};

#endif // MIDIPANELDIALOG_H
