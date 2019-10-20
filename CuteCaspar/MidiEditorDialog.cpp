#include "MidiEditorDialog.h"
#include "ui_MidiEditorDialog.h"

MidiEditorDialog::MidiEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MidiEditorDialog)
{
    ui->setupUi(this);
}

MidiEditorDialog::~MidiEditorDialog()
{
    delete ui;
}
