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

#include "MidiLogger.h"
#include "MidiReader.h"
#include "Player.h"

#include "qmidiout.h"

namespace Ui {
class MainWindow;
}

struct note {
    int id;
    QString name;
    unsigned int pitch;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    Q_SIGNAL void mediaListUpdated();

    void connectServer();    
    void activateButton(unsigned int);
    void deactivateButton(unsigned int);

public slots:
    void onTcpStateChanged(QAbstractSocket::SocketState socketState);
    void readyRead();
    void processOsc(QStringList address, QStringList values);
    void listMedia();
    void setCurrentClip(int index);
    void setTimeCode(double time);
    void playNote(unsigned int pitch = 128);
    void killNote(unsigned int pitch = 128);
    void activeClipName(QString clipName);

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
    void on_stopPushButton_clicked();
    void on_startPushButton_clicked();
    void on_actionPlaylist_triggered();
    void on_btnStartPlaylist_clicked();

    void on_btnStopPlaylist_clicked();

signals:
    void nextClip();
    void currentTime(double time);

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
    QList<note> notes;
    QString currentClip;
    QString timecode;
    bool recording = false;
    MidiLogger* midiLog;
    MidiReader* midiRead;
    QMap<QString, message> midiPlayList;
    QMap<unsigned int, QPushButton*> button;
    QList<QString> playlistClips;
    int currentClipIndex = 0;
    Player* player;
};

#endif // MAINWINDOW_H
