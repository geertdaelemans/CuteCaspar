#ifndef CONTROLDIALOG_H
#define CONTROLDIALOG_H

#include <QDialog>

#include "Models/ClipInfo.h"

namespace Ui {
class ControlDialog;
}

class ControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ControlDialog(QWidget *parent = nullptr);
    ~ControlDialog();
    void setup();

private:
    Ui::ControlDialog *ui;
    QList<ClipInfo> clips;

signals:
    void insertPlaylist(QString clipName, QString database = "scares");

private slots:
    void takeAction();
};

#endif // CONTROLDIALOG_H
