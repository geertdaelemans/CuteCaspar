#include "Player.h"

#include <QSqlQuery>
#include <QtSql>
#include <QPushButton>

#include "MidiConnection.h"
#include "RaspberryPI.h"
#include "DatabaseManager.h"

#include "Timecode.h"

Player* Player::s_inst = nullptr;

Player::Player()
{
    m_device = nullptr;
    m_status = PlayerStatus::IDLE;

    midiRead = new MidiReader();
    midiLog = new MidiLogger();
}

Player* Player::getInstance()
{
    if (!s_inst) {
        s_inst = new Player();
    }
    return s_inst;
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
    if (!query.prepare("SELECT Id, Name, Fps FROM Playlist"))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    m_playlistClips.clear();
    ClipInfo newClip;
    while (query.next()) {
        newClip.setId(query.value(0).toInt());
        newClip.setName(query.value(1).toString());
        newClip.setFps(query.value(2).toDouble());
        midiRead->openLog(query.value(1).toString());
        if (midiRead->isReady()) {
            newClip.setMidi(true);
            DatabaseManager::getInstance().updateMidiStatus(query.value(1).toString(), true);
        }
        m_playlistClips.append(newClip);
    }
    query.finish();
    setStatus(PlayerStatus::READY);
    updateRandomClip();
}


/**
 * @brief Player::getNumberOfClips
 * @param playlist
 * @return
 */
int Player::getNumberOfClips(QString playlist) const
{
    int number = 0;
    QSqlQuery query;
    if (!query.prepare(QString("SELECT Count(*) FROM %1").arg(playlist)))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    while(query.next()) {
        number = query.value(0).toInt();
    }
    query.finish();
    return number;
}


void Player::updateRandomClip()
{
    int amount = getNumberOfClips("Scares");
    qDebug("Random scares: %d", amount);
    if (amount > 0) {
        int id = QRandomGenerator::global()->bounded(amount) + 1;
        QSqlQuery query;
        if (!query.prepare(QString("SELECT Name FROM Scares WHERE id = %1").arg(id)))
            qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
        query.exec();
        while (query.next()) {
            m_randomScare = query.value(0).toString();
            qDebug("Set random scare (m_randomScare) to: %s", qPrintable(m_randomScare));
        }
        query.finish();
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
        m_currentClipIndex = QRandomGenerator::global()->bounded(m_playlistClips.size());
    } else {
        m_currentClipIndex = clipIndex;
    }
    m_nextClipIndex = m_currentClipIndex;
    m_timecode = 100;  // By faking the present timecode, the start of the clip (follows) will trigger loadNextClip()
    m_device->playMovie(1, m_defaultLayer, m_playlistClips[m_currentClipIndex].getName(), "", 0, "", "", 0, 0, false, false);
    m_singlePlay = false;
    emit activeClip(m_currentClipIndex);
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
    emit activeClipName(m_playlistClips[m_currentClipIndex].getName(), m_playlistClips[m_nextClipIndex].getName(), false);
    m_device->resume(1, m_defaultLayer);
    if (m_singlePlay) {
        setStatus(PlayerStatus::CLIP_PLAYING);
    } else {
        setStatus(PlayerStatus::PLAYLIST_PLAYING);
    }
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
    if (m_singlePlay) {
        setStatus(PlayerStatus::CLIP_PLAYING);
    } else {
        setStatus(PlayerStatus::PLAYLIST_PLAYING);
    }
}


/**
 * @brief Player::stopPlayList
 */
void Player::stopPlayList()
{
    m_device->stop(1, m_defaultLayer);
    midiLog->closeMidiLog();
    setStatus(PlayerStatus::READY);
    stopSoundScape();
    stopOverlay();
}


void Player::nextClip()
{
    if (m_activeVideoLayer != m_defaultLayer) {
        stopOverlay();
        resumePlayList();
    } else {
        if (!m_insertedClip) {
            loadClip(m_playlistClips[m_nextClipIndex].getName());
        }
        m_device->play(1, m_defaultLayer);
    }
}

void Player::insertPlaylist(QString clipName)
{
    if (clipName == "random") {
        if (m_randomScare != "") {
            m_interruptedClipName = m_randomScare;
        } else {
            qDebug("No insert clip available");
            return;
        }
    } else {
        m_interruptedClipName = clipName;
    }
    pauseSoundScape();
    m_device-> pause(1, m_defaultLayer);
    m_activeVideoLayer = m_overlayLayer;
    m_device->playMovie(1, m_overlayLayer, m_interruptedClipName, "", 0, "", "", 0, 0, false, false);
    emit activeClipName(m_interruptedClipName, m_playlistClips[m_currentClipIndex].getName(), true);
    qDebug() << "Playing interrupt clip:" << m_interruptedClipName;
    retrieveMidiPlayList(m_interruptedClipName);
    if (midiRead->isReady()) {
        qDebug("MIDI file found...");
    } else {
        qDebug("No MIDI file found...");
    }
    if (m_recording) {
        midiLog->openMidiLog(m_interruptedClipName);
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
        qDebug() << "Writing" << m_playlistClips[m_currentClipIndex].getName();
        midiLog->openMidiLog(m_playlistClips[m_currentClipIndex].getName());
        foreach(auto it, midiPlayList) {
            midiLog->writeNote(QString("%1,%2,%3").arg(it.timeCode).arg(it.type).arg(it.pitch));
        }
        midiLog->closeMidiLog();
    }
}


/**
 * @brief Player::playClip
 * @param clipName
 */
void Player::playClip(int clipIndex)
{
    m_device->loadMovie(1, m_defaultLayer, m_playlistClips[clipIndex].getName(), "", 0, "", "", 0, 0, false, false, true);
    m_currentClipIndex = clipIndex;
    m_insertedClip = true;
    nextClip();
}

/**
 * @brief Player::playClip
 * @param clipName
 */
void Player::playClip(QString clipName)
{
    int clipIndex = getClipIndexByName(clipName);
    m_device->loadMovie(1, m_defaultLayer, m_playlistClips[clipIndex].getName(), "", 0, "", "", 0, 0, false, false, true);
    m_currentClipIndex = clipIndex;
    m_insertedClip = true;
    nextClip();
}

int Player::getClipIndexByName(QString clipName) const
{
    for (int i = 0; i < m_playlistClips.size(); i++) {
        if (m_playlistClips[i].getName() == clipName) {
            qDebug() << "Found clip" << i;
            return i;
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
    if (!m_singlePlay && !m_insertedClip) {
        m_currentClipIndex = m_nextClipIndex;
        emit activeClip(m_currentClipIndex);
        qDebug() << "Playing:" << m_playlistClips[m_currentClipIndex].getName();
        retrieveMidiPlayList(m_playlistClips[m_currentClipIndex].getName());
        if (midiRead->isReady()) {
            qDebug("MIDI file found...");
            pauseSoundScape();
        }
        else {
            qDebug("No MIDI file found...");
            resumeSoundScape();
        }
        if (m_recording) {
            midiLog->openMidiLog(m_playlistClips[m_currentClipIndex].getName());
        }
        if (m_random) {
            int randomNumber = QRandomGenerator::global()->bounded(m_playlistClips.size());
            while (randomNumber == m_nextClipIndex) {
                randomNumber = QRandomGenerator::global()->bounded(m_playlistClips.size());
            }
            m_nextClipIndex = randomNumber;
        } else if (m_currentClipIndex > (m_playlistClips.size()-2)) {
            m_nextClipIndex = 0;
        } else {
            m_nextClipIndex++;
        }
        loadClip(m_playlistClips[m_nextClipIndex].getName());
        setStatus(PlayerStatus::PLAYLIST_PLAYING);
        emit activeClipName(m_playlistClips[m_currentClipIndex].getName(), m_playlistClips[m_nextClipIndex].getName());
    }
    if (m_singlePlay) {
        emit activeClip(m_currentClipIndex);
        m_singlePlay = false;
    }
    if (m_insertedClip) {
        emit activeClip(m_currentClipIndex);
        qDebug() << "Playing:" << m_playlistClips[m_currentClipIndex].getName();
        retrieveMidiPlayList(m_playlistClips[m_currentClipIndex].getName());
        if (midiRead->isReady()) {
            qDebug("MIDI file found...");
            pauseSoundScape();
        }
        else {
            qDebug("No MIDI file found...");
            resumeSoundScape();
        }
        if (m_recording) {
            midiLog->openMidiLog(m_playlistClips[m_currentClipIndex].getName());
        }
        loadClip(m_playlistClips[m_nextClipIndex].getName());
        setStatus(PlayerStatus::PLAYLIST_PLAYING);
        m_insertedClip = false;
        emit activeClipName(m_playlistClips[m_currentClipIndex].getName(), m_playlistClips[m_nextClipIndex].getName());
    }
}


/**
 * @brief Player::timecode
 * @param time
 */
void Player::timecode(double time, double duration, int videoLayer)
{
    Q_UNUSED(duration)
    if (videoLayer == m_defaultLayer) {
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
                qDebug() << " singlePlay=" << m_singlePlay;
                delayedLoadNextClip(100); //delay in ms
            }
            // Previous clip has just stopped
            if (qFabs(prev_timecode - m_timecode) < 0.001) {
                qDebug() << "PREVIOUS CLIP HAS JUST STOPPED" << prev_timecode << " - " << m_timecode;;
                qDebug() << " singlePlay=" << m_singlePlay << " insertedClip=" << m_insertedClip;
                m_endOfClipDetected = true;
                qDebug() << "m_endOfClipDetected = true";
                delayedLoadNextClip(100); //delay in ms
                midiLog->closeMidiLog();
            }
            if (midiPlayList.count() > 0) {
                double fps = m_playlistClips[m_currentClipIndex].getFps();
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
                double fps = m_playlistClips[m_currentClipIndex].getFps();
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
