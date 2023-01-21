#ifndef PLAYER_H
#define PLAYER_H

#include "CasparDevice.h"
#include "MidiReader.h"
#include "MidiLogger.h"
#include "MidiNotes.h"
#include "Models/ClipInfo.h"

enum class PlayerStatus
{
    IDLE,
    READY,
    PLAYLIST_PLAYING,
    PLAYLIST_PAUSED,
    PLAYLIST_INSERT,
};

enum class VideoLayer
{
    SOUNDSCAPE = 1,
    DEFAULT = 2,
    OVERLAY = 3,
    EDIT = 4
};

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

class Player : public QObject
{

    Q_OBJECT

public:
    const bool TRIGGER_PLAYLIST_AFTER_SCARE = true;
    Player();
    static Player *getInstance();
    void setDevice(CasparDevice *device);
    void setRandom(bool random);
    void loadPlayList();
    void loadClip(QString clipName);
    void playClip(int clipIndex);
    void startPlayList(int clipIndex);
    void pausePlayList();
    void resumePlayList();
    void resumeFromFrame(int frames);
    void stopPlayList();
    PlayerStatus getStatus() const;
    void retrieveMidiPlayList(QString clipName);
    void saveMidiPlayList(QMap<QString, message> midiPlayList);
    void playClip(QString clipName);
    void nextClip();
    void setTriggersActive(bool value);
    VideoLayer getActiveVideoLayer() {return m_activeVideoLayer;};
    void updateRandomClip();

    // SoundScape Calls
    void startSoundScape();
    void pauseSoundScape();
    void resumeSoundScape();
    void stopSoundScape();
    void toggleSoundScape();

    void stopOverlay();
    void delayedLoadNextClip(int timeout);
public slots:
    void loadNextClip();
    void timecode(double time, double duration, int videoLayer);
    void currentFrame(int frame, int lastFrame);
    void playNote(unsigned int pitch = 128, bool noteOne = true);
    void killNote();
    void setRecording();
    void insertPlaylist(QString clipName = "random");
    void onTimer_LoadNextClip();

private:
    CasparDevice* m_device;
    QList<ClipInfo> m_playlistClips;
    VideoLayer m_activeVideoLayer = VideoLayer::DEFAULT;
    ClipInfo m_currentClip;
    ClipInfo m_nextClip;
    ClipInfo m_randomClip;
    ClipInfo m_soundScapeClip;
    ClipInfo m_interruptClip;
    double m_timecode;
    double m_timecodeOverlayLayer;
    double m_timecodeSoundScapeLayer;
    PlayerStatus m_status;
    MidiReader* midiRead;
    MidiLogger* midiLog;
    QMap<QString, message> midiPlayList;
    QMap<QString, message> midiSoundScape;
    QMap<QString, message>::iterator midiPlayListIterator;
    QMap<QString, message>::iterator midiSoundScapeIterator;
    bool m_singlePlay = false;
    bool m_recording = false;
    bool m_triggersActive = true;
    void setStatus(PlayerStatus status);
    unsigned int previousPitch;
    bool m_insertedClip = false;
    bool m_endOfClipDetected = false;
    int m_currentFrame;
    int m_lastFrame;
    int getClipIndexByName(QString ClipName);
    bool m_soundScapeActive = false;
    bool m_soundScapePlaying = false;
    void retrieveMidiSoundScape(QString clipName);
    bool m_random = true;
    MidiNotes* m_midiNotes = MidiNotes::getInstance();
    int m_stopLength = 0;

signals:
    void newActiveClip(ClipInfo activeClip = ClipInfo(), ClipInfo upcomingClip = ClipInfo(), bool insert = false);
    void activateButton(unsigned int pitch, bool active = true);
    void playerStatus(PlayerStatus status, bool recording);
    void insertFinished();
    void newMidiPlaylist(QMap<QString, message> midiPlayList);
    void newRandomClip(ClipInfo randomClip);
    void currentNote(QString timecode, bool noteOn, unsigned int pitch);
    void refreshPlayList();
    void soundScapeActive(bool active);
};

#endif // PLAYER_H
