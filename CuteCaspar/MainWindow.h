#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QSettings>
#include <QListWidgetItem>
#include <QDateTime>
#include <QSqlQueryModel>

#include "AmcpDevice.h"
#include "CasparOSCListener.h"
#include "CasparDevice.h"
#include "RaspberryPI.h"
#include "DatabaseManager.h"

#include "SettingsDialog.h"
#include "MidiEditorDialog.h"
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
    void connectServer();    

public slots:
    void onTcpStateChanged(QAbstractSocket::SocketState socketState);
    void readyRead();
    void processOsc(QStringList address, QStringList values);
    void listMedia();
    void setTimeCode(double time, double duration, int videoLayer);
    void reportActiveClip(ClipInfo clipName, ClipInfo upcoming, bool insert = false);
    void playerStatus(PlayerStatus status, bool isRecording);
    void libraryContextMenu(QPoint pos);
    void playlistContextMenu(QPoint pos);
    void copyToList();
    void removeClipFromList();
    void databaseUpdated(QString table);

private slots:
    void disconnectServer();
    void on_actionExit_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void connectionStateChanged(CasparDevice &);
    void mediaChanged(const QList<CasparMedia> &mediaItems, CasparDevice &device);
    void refreshPlayList();
    void refreshLibraryList();
    void soundScapeActive(bool active);
    void on_tableView_clicked(const QModelIndex &index);
    void on_actionSettings_triggered();
    void on_actionPreview_toggled(bool visible);
    void on_actionPlaylist_triggered();
    void on_btnStartPlaylist_clicked();
    void on_btnStopPlaylist_clicked();
    void on_btnRecording_clicked();
    void on_chkRandom_stateChanged(int random);
    void on_chkTriggers_stateChanged(int triggers);
    void on_btnPlayClip_clicked();
    void on_actionMidi_Panel_triggered();
    void on_btnInterrupt_clicked();
    void on_actionRaspberryPI_triggered();
    void on_actionControl_Panel_triggered();
    void on_actionMIDI_Editor_triggered();
    void newRandomClip(ClipInfo randomClip);
    void on_btnNext_clicked();
    void on_btnReloadLibrary_clicked();

    void on_btnNewInterrupt_clicked();

    void on_btnSoundScape_clicked();

signals:
    void nextClip();
    void currentTime(double time, double duration, int videoLayer);
    void currentFrame(int frame, int lastFrame);
    void setRecording();
//    void setRenew(bool value);
    void parseMessage(QString msg);
    void clipNameSelected(QString clipName);

private:
    Ui::MainWindow *ui;
    QTcpSocket tcp;
    QUdpSocket udp;
    int playCurVFrame, playLastVFrame, playFps;
    void log(QString message);
    CasparOscListener listener;
    CasparDevice* m_device = nullptr;
    MidiEditorDialog* m_midiEditorDialog = nullptr;
    MidiPanelDialog* m_midiPanelDialog = nullptr;
    RaspberryPIDialog* m_raspberryPIDialog = nullptr;
    ControlDialog* m_controlDialog = nullptr;
    RaspberryPI* m_raspberryPI = nullptr;
    ClipInfo m_currentClip;
    QString timecode;
    Player* m_player = nullptr;
    MidiConnection* m_midiCon = nullptr;
    void setButtonColor(QPushButton *button, QColor color);
};

#endif // MAINWINDOW_H
