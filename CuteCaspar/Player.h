#ifndef PLAYER_H
#define PLAYER_H

#include "CasparDevice.h"

#include "MidiReader.h"

enum class PlayerStatus
{
    IDLE,
    PLAYLIST_LOADED,
    PLAYLIST_PLAYING,
    PLAYLIST_PAUSED,
    CLIP_PLAYING
};


class Player : public QObject
{

    Q_OBJECT

public:
    Player(CasparDevice* device);
    void loadPlayList();
    void loadClip(QString clipName);
    void playClip(QString clipName);
    void startPlayList();
    void pausePlayList();
    void resumePlayList();
    void stopPlayList();
    PlayerStatus getStatus();

public slots:
    void loadNextClip();
    void timecode(double time);

private:
    CasparDevice* m_device;
    QList<QString> m_playlistClips;
    int m_currentClipIndex = 0;
    double m_timecode;
    PlayerStatus m_status;
    MidiReader* midiRead;
    QMap<QString, message> midiPlayList;
    bool m_singlePlay = false;

signals:
    void activeClip(int value);
    void playNote(unsigned int pitch);
    void killNote(unsigned int pitch);
    void activeClipName(QString clipName);
};

#endif // PLAYER_H
