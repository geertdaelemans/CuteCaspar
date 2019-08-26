#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QSettings>
#include <QListWidgetItem>
#include <QDateTime>

#include "AmcpDevice.h"
#include "CasparOSCListener.h"
#include "CasparDevice.h"

#include "SettingsDialog.h"

#include "qmidiout.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    Q_SIGNAL void mediaListUpdated();

    void connectServer();

public slots:
    void onTcpStateChanged(QAbstractSocket::SocketState socketState);
    void readyRead();
    void processOsc(QStringList address, QStringList values);
    void listMedia();

private slots:
    void disconnectServer();
    void on_btnPlay_clicked();
    void on_btnStop_clicked();
    void on_actionExit_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void connectionStateChanged(CasparDevice &);
    void mediaChanged(const QList<CasparMedia> &mediaItems, CasparDevice &device);
    void refreshMediaList();
    void on_tableView_clicked(const QModelIndex &index);
    void on_actionSettings_triggered();
    void on_actionPreview_toggled(bool visible);
    void on_outPortComboBox_currentIndexChanged(int index);
    void playNote(unsigned int pitch = 128);
    void killNote(unsigned int pitch = 128);

private:
    Ui::MainWindow *ui;
    QTcpSocket tcp;
    QUdpSocket udp;
    bool playPlaying = false;
    bool newClipLoaded = false;
    int playCurFrame, playLastFrame, playCurVFrame, playLastVFrame, playFps;
    void log(QString message);
    CasparOscListener listener;
    CasparDevice* device;
    QDateTime lastOsc;
    SettingsDialog * m_settingsDialog;
    QMidiOut* midiOut;
    QMap<QString, unsigned int> notes;
};

#endif // MAINWINDOW_H