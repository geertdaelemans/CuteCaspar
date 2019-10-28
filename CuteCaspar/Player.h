#ifndef PLAYER_H
#define PLAYER_H

#include "CasparDevice.h"

#include "MidiReader.h"
#include "MidiLogger.h"

enum class PlayerStatus
{
    IDLE,
    READY,
    PLAYLIST_PLAYING,
    PLAYLIST_PAUSED,
    PLAYLIST_INSERT,
    CLIP_PLAYING,
    CLIP_PAUSED
};


class Player : public QObject
{

    Q_OBJECT

public:
    Player(CasparDevice* device);
    void loadPlayList();
    void loadClip(QString clipName);
    void playClip(int clipIndex);
    void startPlayList(int clipIndex);
    void pausePlayList();
    void resumePlayList();
    void stopPlayList();
    PlayerStatus getStatus() const;

public slots:
    void loadNextClip();
    void timecode(double time);
    void currentFrame(int frame, int lastFrame);
    void playNote(unsigned int pitch = 128, bool noteOne = true);
    void killNote();
    void setRecording();
    void setRenew(bool value);
    void insertPlaylist(QString clipName = "random");
    void saveMidiPlayList(QMap<QString, message> midiPlayList);
    void resumeFromFrame(int frames);

private:
    CasparDevice* m_device;
    QList<QString> m_playlistClips;
    int m_currentClipIndex = 0;
    int m_nextClipIndex = 0;
    double m_timecode;
    PlayerStatus m_status;
    MidiReader* midiRead;
    MidiLogger* midiLog;
    QMap<QString, message> midiPlayList;
    QMap<QString, message>::iterator i;
    bool m_singlePlay = false;
    bool m_recording = false;
    bool m_renew = false;
    bool m_interrupted = false;
    QString m_interruptedClipName;
    void setStatus(PlayerStatus status);
    unsigned int previousPitch;
    bool m_insert = false;
    int getNumberOfClips(QString playlist) const;
    void updateRandomClip();
    QString m_randomScare;
    int m_currentFrame;
    int m_lastFrame;

signals:
    void activeClip(int value);
    void activeClipName(QString clipName, bool insert = false);
    void activateButton(unsigned int pitch, bool active = true);
    void playerStatus(PlayerStatus status, bool recording);
    void insertFinished();
    void newMidiPlaylist(QMap<QString, message> midiPlayList);
    void currentNote(QString timecode, bool noteOn, unsigned int pitch);
};

#endif // PLAYER_H
