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
#include "RaspberryPI.h"

#include "SettingsDialog.h"
#include "MidiPanelDialog.h"
#include "RaspberryPIDialog.h"
#include "ControlDialog.h"

#include "MidiLogger.h"
#include "MidiReader.h"
#include "Player.h"
#include "MidiConnection.h"

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
    void setCurrentClip(int index);
    void setTimeCode(double time);
    void activeClipName(QString clipName, bool insert = false);
    void playerStatus(PlayerStatus status, bool isRecording);

private slots:
    void disconnectServer();
    void on_actionExit_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void connectionStateChanged(CasparDevice &);
    void mediaChanged(const QList<CasparMedia> &mediaItems, CasparDevice &device);
    void refreshMediaList();
    void on_tableView_clicked(const QModelIndex &index);
    void on_actionSettings_triggered();
    void on_actionPreview_toggled(bool visible);
    void on_actionPlaylist_triggered();
    void on_btnStartPlaylist_clicked();
    void on_btnStopPlaylist_clicked();
    void on_btnRecording_clicked();
    void on_renewCheckBox_stateChanged(int arg1);
    void on_btnPlayClip_clicked();
    void on_actionMidi_Panel_triggered();

    void on_pushButton_clicked();

    void on_actionRaspberryPI_triggered();

    void on_actionControl_Panel_triggered();

signals:
    void nextClip();
    void currentTime(double time);
    void setRecording();
    void setRenew(bool value);

private:
    Ui::MainWindow *ui;
    QTcpSocket tcp;
    QUdpSocket udp;
    int playCurFrame, playLastFrame, playCurVFrame, playLastVFrame, playFps;
    void log(QString message);
    CasparOscListener listener;
    CasparDevice* device = nullptr;
    QDateTime lastOsc;
    MidiPanelDialog * m_midiPanelDialog = nullptr;
    RaspberryPIDialog * m_raspberryPIDialog = nullptr;
    ControlDialog* m_controlDialog = nullptr;
    RaspberryPI* m_raspberryPI = nullptr;
    QString currentClip;
    QString timecode;
    int currentClipIndex = 0;
    Player* player = nullptr;
    MidiConnection* midiCon = nullptr;
    void setButtonColor(QPushButton *button, QColor color);
};

#endif // MAINWINDOW_H
