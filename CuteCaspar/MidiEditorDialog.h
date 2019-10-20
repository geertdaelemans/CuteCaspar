#ifndef MIDIEDITORDIALOG_H
#define MIDIEDITORDIALOG_H

#include <QDialog>

namespace Ui {
class MidiEditorDialog;
}

class MidiEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MidiEditorDialog(QWidget *parent = nullptr);
    ~MidiEditorDialog();

private:
    Ui::MidiEditorDialog *ui;
};

#endif // MIDIEDITORDIALOG_H
