#ifndef CONTROLDIALOG_H
#define CONTROLDIALOG_H

#include <QDialog>

namespace Ui {
class ControlDialog;
}

struct clip {
    int id;
    QString name;
};

class ControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ControlDialog(QWidget *parent = nullptr);
    ~ControlDialog();
    void setup();

private:
    Ui::ControlDialog *ui;
    QList<clip> clips;

signals:
    void insertPlaylist(QString clipName);

private slots:
    void takeAction();
};

#endif // CONTROLDIALOG_H
