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
    void activateButton(unsigned int pitch, bool active);

signals:
    void buttonPushed(unsigned int, bool);

private slots:
    void playNote();
    void killNote();

private:
    Ui::MidiPanelDialog *ui;
    QList<note> notes;
    QMap<unsigned int, QPushButton*> button;
    unsigned int previousPitch = 0;
    void setButtonColor(QPushButton *button, QColor color);
    bool m_button = false;
    bool m_light = false;
    bool m_magnet = false;
    bool m_motion = false;
    bool m_smoke = false;
};

#endif // MIDIPANELDIALOG_H
