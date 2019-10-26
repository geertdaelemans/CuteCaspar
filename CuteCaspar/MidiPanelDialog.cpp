#include "MidiPanelDialog.h"
#include "ui_MidiPanelDialog.h"

#include <QtCore>

MidiPanelDialog::MidiPanelDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MidiPanelDialog)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    // Retrieve notes definitions
    notes = MidiNotes::getInstance()->getNotes();

    // Magically calculate the number of rows
    int rows = qCeil(qSqrt(static_cast<qreal>(MidiNotes::getInstance()->getNumberOfNotes())));
    int counter = 0;
    for (int i = 0; i < notes.length(); i++) {
        QPushButton* newButton = new QPushButton(notes[i].name);
        newButton->setFocusPolicy(Qt::NoFocus);
        newButton->setProperty("pitch", notes[i].pitch);
        newButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(newButton, SIGNAL(pressed()),
                this, SLOT(playNote()));
        connect(newButton, SIGNAL(released()),
                this, SLOT(killNote()));
        if (notes[i].pitch < 128) {
            ui->theGrid->addWidget(newButton, counter / rows, counter % rows);
            counter++;
        } else {
            newButton->setBaseSize(100, 50);
            newButton->setSizePolicy(QSizePolicy ::Expanding , QSizePolicy ::Maximum);
            newButton->setStyleSheet("height: 40px;");
            ui->theActions->addWidget(newButton);
        }
        button[notes[i].pitch] = newButton;
    }

    // Timer sequence for notes with a duration
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(this, SIGNAL(waitNote(int)),
            m_timer, SLOT(start(int)));
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(automatedSequence()));
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
    if (pitch > 128) {
        switch (pitch) {
        case 129:
            m_button = !m_button;
            emit buttonPushed(pitch, m_button);
            break;
        case 130:
            m_light = !m_light;
            emit buttonPushed(pitch, m_light);
            break;
        case 131:
            m_magnet = !m_magnet;
            emit buttonPushed(pitch, m_magnet);
            break;
        case 132:
            m_motion = !m_motion;
            emit buttonPushed(pitch, m_motion);
            break;
        case 133:
            m_smoke = !m_smoke;
            emit buttonPushed(pitch, m_smoke);
            break;
        }
    } else {
        emit buttonPushed(pitch, true);
    }
}

void MidiPanelDialog::killNote()
{
    auto button = qobject_cast<QPushButton *>(sender());
    button->setDown(true);
}

void MidiPanelDialog::activateButton(unsigned int pitch, bool active)
{
    if (button.contains(pitch)) {
        button[pitch]->setDown(active);
        setButtonColor(button[pitch], (active ? Qt::darkGreen : QColor(53,53,53)));
        if (pitch != previousPitch && pitch < 128) {
            button[previousPitch]->setDown(false);
            setButtonColor(button[previousPitch], QColor(53,53,53));
        }
        if (pitch < 128) {
            previousPitch = pitch;
        }
    }
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    if (MidiNotes::getInstance()->getDuration(pitch) > 0 && m_live) {
        m_nextPitch = MidiNotes::getInstance()->getNext(pitch);
        emit waitNote(static_cast<int>(MidiNotes::getInstance()->getDuration(pitch)));
    }
}

void MidiPanelDialog::setButtonColor(QPushButton* button, QColor color)
{
    QPalette pal = button->palette();
    pal.setColor(QPalette::Button, color);
    button->setPalette(pal);
    button->update();
}

void MidiPanelDialog::automatedSequence()
{
    activateButton(m_nextPitch);
}

void MidiPanelDialog::on_chkLive_stateChanged(int check)
{
    m_live = check;
}
