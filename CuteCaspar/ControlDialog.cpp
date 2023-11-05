#include "ControlDialog.h"
#include "ui_ControlDialog.h"

#include <QtSql>
#include <QSqlQuery>
#include <QLabel>

ControlDialog::ControlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControlDialog)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    setup();
}

ControlDialog::~ControlDialog()
{
    delete ui;
}

void ControlDialog::setup()
{
    qDebug() << "Clip setup";
    // Get "Extras" playlist
    QSqlQuery query;
    if (!query.prepare("SELECT Id, Name FROM Extras"))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    int counter = 0;
    clips.clear();
    ClipInfo newClip;
    while (query.next()) {
        newClip.setId(query.value(0).toInt());
        newClip.setName(query.value(1).toString());
        clips.append(newClip);
        counter++;
        qDebug() << newClip.getId() << newClip.getName();
    }
    query.finish();

    // Magically calculate the number of rows
    int rows = qCeil(qSqrt(static_cast<qreal>(counter + 1)));

    // Remove all items
    QLayoutItem* item;
    while ((item = ui->theGrid->layout()->takeAt(0)) != NULL) {
        delete item->widget();
        delete item;
    }

    // Add first 'Random Scare' button
    QPushButton* newButton = new QPushButton("Random Scare");
    newButton->setFocusPolicy(Qt::NoFocus);
    newButton->setProperty("name", "random");
    newButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(newButton, SIGNAL(pressed()),
            this, SLOT(takeAction()));
    ui->theGrid->addWidget(newButton, 0, 0);

    // Next add all clips from the Extras playlist
    counter = 1;
    for (int i = 0; i < clips.length(); i++) {
        QPushButton* newButton = new QPushButton();
        newButton->setFocusPolicy(Qt::NoFocus);
        newButton->setProperty("name", clips[i].getName());
        newButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QLabel* label = new QLabel(clips[i].getName(), newButton);
        label->setWordWrap(true);
        QHBoxLayout* layout = new QHBoxLayout(newButton);
        layout->addWidget(label, 0, Qt::AlignCenter);
        connect(newButton, SIGNAL(pressed()),
                this, SLOT(takeAction()));
        ui->theGrid->addWidget(newButton, counter / rows, counter % rows);
//        button[notes[i].pitch] = newButton;
        counter++;
    }
}

void ControlDialog::takeAction()
{
    auto button = qobject_cast<QPushButton *>(sender());
    if (button && button->property("name").isValid()) {
        emit insertPlaylist(button->property("name").toString(), "extras");
    }
}
