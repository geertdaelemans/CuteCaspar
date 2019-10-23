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
    void activateButton(unsigned int pitch);

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
};

#endif // MIDIPANELDIALOG_H
