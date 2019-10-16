#include "MidiPanelDialog.h"
#include "ui_MidiPanelDialog.h"

#include <QtCore>

const bool SINGLE_BUTTON_MODUS = true;

MidiPanelDialog::MidiPanelDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MidiPanelDialog)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    // Prepare notes panel
    QFile profile;
    if(QFileInfo::exists("Notes.csv"))
    {
        profile.setFileName("Notes.csv");
    }
    else
    {
        profile.setFileName(":/Profiles/Notes.csv");
    }

    if (!profile.open(QIODevice::ReadOnly)) {
        qDebug() << profile.errorString();
    }
    int counter = 0;
    while (!profile.atEnd()) {
        QByteArray line = profile.readLine();
        note tempNote;
        tempNote.id = counter;
        tempNote.name = line.split(',').at(0);
        tempNote.pitch = line.split(',').at(1).toUInt();
        notes.append(tempNote);
        counter++;
    }

    // Magically calculate the number of rows
    int rows = qCeil(qSqrt(static_cast<qreal>(counter)));
    counter = 0;
    for (int i = 0; i < notes.length(); i++) {
        QPushButton* newButton = new QPushButton(notes[i].name);
        newButton->setFocusPolicy(Qt::NoFocus);
        newButton->setProperty("pitch", notes[i].pitch);
        newButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(newButton, SIGNAL(pressed()),
                this, SLOT(playNote()));
        connect(newButton, SIGNAL(released()),
                this, SLOT(killNote()));
        ui->theGrid->addWidget(newButton, counter / rows, counter % rows);
        button[notes[i].pitch] = newButton;
        counter++;
    }
}

MidiPanelDialog::~MidiPanelDialog()
{
    delete ui;
}

void MidiPanelDialog::playNote()
{
    unsigned int pitch = 60;
    auto button = qobject_cast<QPushButton *>(sender());
    if (button && button->property("pitch").isValid()) {
        pitch = button->property("pitch").toUInt();
    }
    emit buttonPushed(pitch, true);
}

void MidiPanelDialog::killNote()
{
    unsigned int pitch = 60;
    auto button = qobject_cast<QPushButton *>(sender());
    button->setDown(true);
    if (button && button->property("pitch").isValid()) {
        pitch = button->property("pitch").toUInt();
    }
    emit buttonPushed(pitch, false);
}

void MidiPanelDialog::activateButton(unsigned int pitch)
{
    if (button.contains(pitch)) {
        button[pitch]->setDown(true);
        setButtonColor(button[pitch], Qt::darkGreen);
        if (SINGLE_BUTTON_MODUS && pitch != previousPitch) {
            button[previousPitch]->setDown(false);
            setButtonColor(button[previousPitch], QColor(53,53,53));
        }
        previousPitch = pitch;
    }
}

void MidiPanelDialog::setButtonColor(QPushButton* button, QColor color)
{
    QPalette pal = button->palette();
    pal.setColor(QPalette::Button, color);
    button->setPalette(pal);
    button->update();
}

