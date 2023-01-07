#include "Player.h"

#include <QSqlQuery>
#include <QtSql>
#include <QPushButton>

#include "MidiConnection.h"
#include "RaspberryPI.h"
#include "DatabaseManager.h"
#include "Timecode.h"

Q_GLOBAL_STATIC(Player, s_player)

Player::Player()
{
    m_device = nullptr;
    m_status = PlayerStatus::IDLE;

    midiRead = new MidiReader();
    midiLog = new MidiLogger();
}

Player* Player::getInstance()
{
    return s_player;
}

void Player::setDevice(CasparDevice* device)
{
    m_device = device;
}

void Player::setRandom(bool random)
{
    m_random = random;
}


/**
 * @brief Player::loadPlayList
 */
void Player::loadPlayList()
{
    QSqlQuery query;
    if (!query.prepare("SELECT Id, Name, Fps, Midi FROM Playlist ORDER BY DisplayOrder"))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    m_playlistClips.clear();
    ClipInfo newClip;
    int playListOrder = 0;
    while (query.next()) {
        newClip.setId(query.value(0).toInt());
        newClip.setDisplayOrder(playListOrder);
        newClip.setName(query.value(1).toString());
        newClip.setFps(query.value(2).toDouble());
        newClip.setMidi(query.value(3).toInt());
        m_playlistClips.append(newClip);
        playListOrder++;
    }
    query.finish();
    setStatus(PlayerStatus::READY);
}


void Player::updateRandomClip()
{
    int amount = DatabaseManager::getInstance()->getNumberOfClips("Scares");
    qDebug("Random scares: %d", amount);
    if (amount > 0) {
        int id = QRandomGenerator::global()->bounded(amount) + 1;
        QSqlQuery query;
        if (!query.prepare(QString("SELECT Id, Name, Fps, Midi FROM Scares WHERE Id = %1").arg(id)))
            qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
        query.exec();
        while (query.next()) {
            m_randomClip.setId(query.value(0).toInt());
            m_randomClip.setDisplayOrder(0);
            m_randomClip.setName(query.value(1).toString());
            m_randomClip.setFps(query.value(2).toDouble());
            m_randomClip.setMidi(query.value(3).toInt());
        }
        query.finish();
        emit newRandomClip(m_randomClip);
    }
}


/**
 * @brief Player::startPlayList
 */
void Player::startPlayList(int clipIndex)
{
    if (m_singlePlay) {
        m_device->stop(1, m_defaultLayer);
    }
    if (m_random) {
        m_currentClip = m_playlistClips[QRandomGenerator::global()->bounded(m_playlistClips.size())];
    } else {
        m_currentClip = m_playlistClips[clipIndex];
    }
    m_nextClip = m_currentClip;
    m_timecode = 100;  // By faking the present timecode, the start of the clip (follows) will trigger loadNextClip()
    m_device->playMovie(1, m_defaultLayer, m_currentClip.getName(), "", 0, "", "", 0, 0, false, false);
    m_singlePlay = false;
    emit activeClip(m_currentClip.getId());
    setStatus(PlayerStatus::PLAYLIST_PLAYING);

    // TODO: SoundScape cLip name should not be hardcoded
    ClipInfo soundScapeClip;
    soundScapeClip.setName("EXTRAS/SOUNDSCAPE");
    soundScapeClip.setFps(29.97);
    m_soundScapeClip = soundScapeClip;
    startSoundScape();
}

/**
 * @brief Player::pausePlayList
 */
void Player::pausePlayList()
{
    m_device->pause(1, m_defaultLayer);
    pauseSoundScape();
    setStatus(PlayerStatus::PLAYLIST_PAUSED);
}


/**
 * @brief Player::resumePlayList
 */
void Player::resumePlayList()
{
    emit activeClipName(m_currentClip.getName(), m_nextClip.getName(), false);
    m_device->resume(1, m_defaultLayer);
    setStatus(PlayerStatus::PLAYLIST_PLAYING);
    resumeSoundScape();
}


/**
 * @brief Player::resumeFromFrame
 * @param frames
 */
void Player::resumeFromFrame(int frames)
{
    m_device->callSeek(1, m_defaultLayer, frames);
    m_device->resume(1, m_defaultLayer);
    setStatus(PlayerStatus::PLAYLIST_PLAYING);
}


/**
 * @brief Player::stopPlayList
 */
void Player::stopPlayList()
{
    m_device->stop(1, m_defaultLayer);
    midiLog->closeMidiLog();
    setStatus(PlayerStatus::READY);
    emit activeClipName("", "");
    stopSoundScape();
    stopOverlay();
}


void Player::nextClip()
{
    if (m_activeVideoLayer != m_defaultLayer) {
        stopOverlay();
        resumePlayList();
    } else {
        if (m_nextClip.getName() != "") {
            loadClip(m_nextClip.getName());
            m_device->play(1, m_defaultLayer);
        } else {
            stopPlayList();
        }
    }
}

void Player::insertPlaylist(QString clipName)
{
    qDebug() << "insertPlaylist" << clipName;
    QString interruptedClipName;
    if (TRIGGER_PLAYLIST_AFTER_SCARE && getStatus() == PlayerStatus::READY) {
        startPlayList(m_currentClip.getPlaylistOrder());
    }
    if (clipName == "random") {
        if (m_randomClip.getName() != "") {
            interruptedClipName = m_randomClip.getName();
        } else {
            qDebug("No insert clip available");
            return;
        }
    } else {
        interruptedClipName = clipName;
    }
    pauseSoundScape();
    m_device-> pause(1, m_defaultLayer);
    m_activeVideoLayer = m_overlayLayer;
    m_device->playMovie(1, m_overlayLayer, interruptedClipName, "", 0, "", "", 0, 0, false, false);
    emit activeClipName(interruptedClipName, m_currentClip.getName(), true);
    qDebug() << "Playing interrupt clip:" << interruptedClipName;
    retrieveMidiPlayList(interruptedClipName);
    if (midiRead->isReady()) {
        qDebug("MIDI file found...");
    } else {
        qDebug("No MIDI file found...");
    }
    if (m_recording) {
        midiLog->openMidiLog(interruptedClipName);
    }
    setStatus(PlayerStatus::PLAYLIST_INSERT);
    if (clipName == "random") {
        updateRandomClip();
    }
}

void Player::saveMidiPlayList(QMap<QString, message> playList)
{
    midiPlayList = playList;
    if (midiLog->isReady()) {
        qDebug() << "Cannot write";
    } else {
        qDebug() << "Writing" << m_currentClip.getName();
        midiLog->openMidiLog(m_currentClip.getName());
        foreach(auto it, midiPlayList) {
            midiLog->writeNote(QString("%1,%2,%3").arg(it.timeCode).arg(it.type).arg(it.pitch));
        }
        midiLog->closeMidiLog();
    }
    emit refreshPlayList();
}


/**
 * @brief Player::playClip
 * Insert clip in playlist and start playing
 * @param clipIndex - ID of clip to be played
 */
void Player::playClip(int clipIndex)
{
    // Find the clip with given ID
    for (int i = 0; i < m_playlistClips.length(); i++) {
        if (m_playlistClips[i].getId() == clipIndex) {
            m_currentClip = m_playlistClips[i];
            break;
        }
    }

    // Play clip once
    m_device->loadMovie(1, m_defaultLayer, m_currentClip.getName(), "", 0, "", "", 0, 0, false, false, true);
    m_insertedClip = true;
    setStatus(PlayerStatus::PLAYLIST_PLAYING);
    emit activeClipName(m_currentClip.getName(), m_nextClip.getName());
    m_device->play(1, m_defaultLayer);
}

/**
 * @brief Player::playClip
 * Insert clip in playlist and start playing
 * @param clipName - name of the clip to be played
 */
void Player::playClip(QString clipName)
{
    playClip(getClipIndexByName(clipName));
}

/**
 * @brief Player::getClipIndexByName
 * Look-up the ID of a clip based upon its name
 * @param clipName - the name of the clip
 * @return the index of the clip
 */
int Player::getClipIndexByName(QString clipName)
{
    for (int i = 0; i < m_playlistClips.size(); i++) {
        if (m_playlistClips[i].getName() == clipName) {
            return m_playlistClips[i].getId();
        }
    }
    return 0;
}

/**
 * SOUNDSCAPE
 */

void Player::startSoundScape()
{
    qDebug() << "startSoundScape";
    retrieveMidiSoundScape(m_soundScapeClip.getName());
    m_device->playMovie(1, m_soundScapeLayer, m_soundScapeClip.getName(), "", 0, "", "", 0, 0, true, true);
    m_soundScapeActive = true;
}

void Player::pauseSoundScape() const
{
    qDebug() << "pauseSoundScape";
    m_device->pause(1, m_soundScapeLayer);
}

void Player::resumeSoundScape() const
{
    qDebug() << "resumeSoundScape";
    m_device->resume(1, m_soundScapeLayer);
}

void Player::stopSoundScape()
{
    qDebug() << "stopSoundScape";
    m_device->stop(1, m_soundScapeLayer);
    m_soundScapeActive = false;
}

void Player::stopOverlay()
{
    qDebug() << "stopOverlay";
    m_device->stop(1, m_overlayLayer);
    m_activeVideoLayer = m_defaultLayer;
    emit insertFinished();
}

/**
 * @brief Player::getStatus
 * @return
 */
PlayerStatus Player::getStatus() const
{
    return m_status;
}


/**
 * @brief Player::setStatus
 * @param status
 */
void Player::setStatus(PlayerStatus status)
{
    if (status != m_status) {
        m_status = status;
    }
    emit playerStatus(m_status, m_recording);
}

/**
 * @brief Player::loadClip
 * @param clipName
 */
void Player::loadClip(QString clipName)
{
    m_device->loadMovie(1, m_defaultLayer, clipName, "", 0, "", "", 0, 0, false, false, true);
}


/**
 * @brief Player::loadNextClip
 */
void Player::loadNextClip()
{
    // Loading next clip from an active playlist
    if (!m_singlePlay && !m_insertedClip && m_nextClip.getName() != "") {
        m_currentClip = m_nextClip;
        emit activeClip(m_currentClip.getId());
        qDebug() << "Playing:" << m_currentClip.getName();
        retrieveMidiPlayList(m_currentClip.getName());
        if (midiRead->isReady()) {
            qDebug("MIDI file found...");
            pauseSoundScape();
        }
        else {
            qDebug("No MIDI file found...");
            resumeSoundScape();
        }
        if (m_recording) {
            midiLog->openMidiLog(m_currentClip.getName());
        }
        if (m_random) {
            int randomNumber = QRandomGenerator::global()->bounded(m_playlistClips.size());
            while (m_playlistClips[randomNumber].getId() == m_currentClip.getId()) {
                randomNumber = QRandomGenerator::global()->bounded(m_playlistClips.size());
            }
            m_nextClip = m_playlistClips[randomNumber];
        } else if (m_currentClip.getPlaylistOrder() > (m_playlistClips.size()-2)) {
            m_nextClip = m_playlistClips[0];
        } else {
            m_nextClip = m_playlistClips[m_currentClip.getPlaylistOrder() + 1];
        }
        loadClip(m_nextClip.getName());
        setStatus(PlayerStatus::PLAYLIST_PLAYING);
        emit activeClipName(m_currentClip.getName(), m_nextClip.getName());
    }
    if (m_singlePlay) {
        emit activeClip(m_currentClip.getId());
        m_singlePlay = false;
    }
    if (m_insertedClip) {
        emit activeClip(m_currentClip.getId());
        qDebug() << "Playing:" << m_currentClip.getName();
        retrieveMidiPlayList(m_currentClip.getName());
        if (midiRead->isReady()) {
            qDebug("MIDI file found...");
            pauseSoundScape();
        }
        else {
            qDebug("No MIDI file found...");
            resumeSoundScape();
        }
        if (m_recording) {
            midiLog->openMidiLog(m_currentClip.getName());
        }
        loadClip(m_nextClip.getName());
        setStatus(PlayerStatus::PLAYLIST_PLAYING);
        m_insertedClip = false;
        emit activeClipName(m_currentClip.getName(), m_nextClip.getName());
    }
}


/**
 * @brief Player::timecode
 * @param time
 */
void Player::timecode(double time, double duration, int videoLayer)
{
    Q_UNUSED(duration)
    if (videoLayer == m_defaultLayer && getStatus() != PlayerStatus::IDLE && getStatus() != PlayerStatus::READY) {
        if (time > 0.0 && m_endOfClipDetected) {
            double prev_timecode = m_timecode;
            m_timecode = time;
            if (prev_timecode - m_timecode > 0.1) {
                m_endOfClipDetected = false;
                qDebug() << "m_endOfClipDetected = false";
            }
        } else if (time > 0.0 && getStatus() != PlayerStatus::PLAYLIST_PAUSED && getStatus() != PlayerStatus::PLAYLIST_INSERT && !m_endOfClipDetected) {
            double prev_timecode = m_timecode;
            m_timecode = time;
            // Next clip has started automatically (Playlist)
            if (prev_timecode > m_timecode && m_timecode < 1.0) {
                qDebug() << "NEXT CLIP HAS STARTED AUTOMATICALLY" << prev_timecode << " - " << m_timecode;
                delayedLoadNextClip(100); //delay in ms
            }
            // Previous clip has just stopped
            if (qFabs(prev_timecode - m_timecode) < 0.001) {
                qDebug() << "PREVIOUS CLIP HAS JUST STOPPED" << prev_timecode << " - " << m_timecode;;
                m_endOfClipDetected = true;
                qDebug() << "m_endOfClipDetected = true";
                delayedLoadNextClip(100); //delay in ms
                midiLog->closeMidiLog();
            }
            if (midiPlayList.count() > 0) {
                double fps = m_currentClip.getFps();
                QString timecode = Timecode::fromTime(time, fps, false);
                if (m_triggersActive && midiPlayListIterator != midiPlayList.end()) {
                    if (midiPlayListIterator.key().length() > 0) {
                        if (midiPlayListIterator->timeCode <= timecode) {
                            if (midiPlayList[midiPlayListIterator->timeCode].type == "ON") {
                                playNote(midiPlayList[midiPlayListIterator->timeCode].pitch, true);
                            } else {
                                playNote(midiPlayList[midiPlayListIterator->timeCode].pitch, false);
                            }
                            midiPlayListIterator++;
                        }
                    }
                }
            }
        }
    } else if (videoLayer == m_overlayLayer) {
        if (time > 0.0 && m_activeVideoLayer == m_overlayLayer) {
            double prev_timecode = m_timecodeOverlayLayer;
            m_timecodeOverlayLayer = time;
            // Inserted clip has just stopped
            if (qFabs(prev_timecode - m_timecodeOverlayLayer) < 0.001) {
                qDebug() << "INSERTED CLIP HAS STOPPED";
                stopOverlay();
                resumePlayList();
            }
            if (midiPlayList.count() > 0) {
                double fps = m_currentClip.getFps();
                QString timecode = Timecode::fromTime(time, fps, false);
                if (m_triggersActive && midiPlayListIterator != midiPlayList.end()) {
                    if (midiPlayListIterator.key().length() > 0) {
                        if (midiPlayListIterator->timeCode <= timecode) {
                            if (midiPlayList[midiPlayListIterator->timeCode].type == "ON") {
                                playNote(midiPlayList[midiPlayListIterator->timeCode].pitch, true);
                            } else {
                                playNote(midiPlayList[midiPlayListIterator->timeCode].pitch, false);
                            }
                            midiPlayListIterator++;
                        }
                    }
                }
            }
        }
    } else if (videoLayer == m_soundScapeLayer) {
        if (m_soundScapeActive && midiSoundScape.count() > 0) {
            double fps = m_soundScapeClip.getFps();
            double prev_timecode = m_timecodeSoundScapeLayer;
            m_timecodeSoundScapeLayer = time;
            if (prev_timecode > m_timecodeSoundScapeLayer) {
                midiSoundScapeIterator = midiSoundScape.begin();
                qDebug() << "Soundscape restarted";
            }
            QString timecode = Timecode::fromTime(time, fps, false);
            if (m_triggersActive && midiSoundScapeIterator != midiSoundScape.end()) {
                if (midiSoundScapeIterator.key().length() > 0) {
                    if (midiSoundScapeIterator->timeCode <= timecode) {
                        if (midiSoundScape[midiSoundScapeIterator->timeCode].type == "ON") {
                            playNote(midiSoundScape[midiSoundScapeIterator->timeCode].pitch, true);
                        } else {
                            playNote(midiSoundScape[midiSoundScapeIterator->timeCode].pitch, false);
                        }
                        midiSoundScapeIterator++;
                    }
                }
            }
        }
    }
}

void Player::currentFrame(int frame, int lastFrame)
{
    m_currentFrame = frame;
    m_lastFrame = lastFrame;
}


/**
 * @brief Player::playNote
 * @param pitch
 */
void Player::playNote(unsigned int pitch, bool noteOn)
{
    if (pitch == 128) {
        auto button = qobject_cast<QPushButton *>(sender());
        if (button && button->property("pitch").isValid()) {
            pitch = button->property("pitch").toUInt();
        }
        else {
            pitch = 60;
        }
    } else if (pitch > 128) {
        switch(pitch) {
        case 129:
            RaspberryPI::getInstance()->setButtonActive(noteOn);
            break;
        case 130:
            RaspberryPI::getInstance()->setLightActive(noteOn);
            break;
        case 131:
            RaspberryPI::getInstance()->setMagnetActive(noteOn);
            break;
        case 132:
            RaspberryPI::getInstance()->setMotionActive(noteOn);
            break;
        case 133:
            RaspberryPI::getInstance()->setSmokeActive(noteOn);
            break;
        }
    }

    QString timecode = Timecode::fromTime(m_timecode, 29.97, false);
    QString onOff = (noteOn ? "ON" : "OFF");
    qDebug() << QString("%1 %2: pitch %3").arg(timecode).arg(onOff).arg(pitch);
    emit currentNote(timecode, noteOn, pitch);
    if (midiLog->isReady() /*&& pitch != previousPitch*/ && noteOn) {
        midiLog->writeNote(QString("%1,%2,%3").arg(timecode).arg(onOff).arg(pitch));
    }
    if (pitch < 128) {
        if(noteOn/* && pitch != previousPitch*/) {
            MidiConnection::getInstance()->killNote(previousPitch);
            MidiConnection::getInstance()->playNote(pitch);
            emit activateButton(pitch);
        } else {
            MidiConnection::getInstance()->killNote(pitch);
        }
    } else {
        emit activateButton(pitch, noteOn);
    }
    previousPitch = pitch;
}


/**
 * @brief Player::killNote
 * @param pitch
 */
void Player::killNote()
{
    unsigned int pitch = 60;
    auto button = qobject_cast<QPushButton *>(sender());
    if (button && button->property("pitch").isValid()) {
        pitch = button->property("pitch").toUInt();
    }
    playNote(pitch, false);
}


void Player::setRecording()
{
    if (m_recording) {
        m_singlePlay = true;
    }
    m_recording = !m_recording;
    setStatus(m_status);
}

void Player::setTriggersActive(bool value)
{
    m_triggersActive = value;
}

void Player::retrieveMidiPlayList(QString clipName)
{
    midiPlayList = midiRead->openLog(clipName);
    midiPlayListIterator = midiPlayList.begin();
    emit newMidiPlaylist(midiPlayList);
}

void Player::retrieveMidiSoundScape(QString clipName)
{
    midiSoundScape = midiRead->openLog(clipName);
    midiSoundScapeIterator = midiSoundScape.begin();
}

void Player::delayedLoadNextClip(int timeout)
{
    QTimer::singleShot(timeout, this, SLOT(onTimer_LoadNextClip()));
}

//----------------------------------------------------------------
//timer slots
//----------------------------------------------------------------
void Player::onTimer_LoadNextClip()
{
    loadNextClip();
}
