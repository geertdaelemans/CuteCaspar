#include "Player.h"

#include <QSqlQuery>
#include <QtSql>
#include <QPushButton>

#include "MidiConnection.h"
#include "RaspberryPI.h"

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
    if (!query.prepare("SELECT Name FROM Playlist"))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    m_playlistClips.clear();
    while (query.next()) {
        m_playlistClips.append(query.value(0).toString());
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
    int id = QRandomGenerator::global()->bounded(amount) + 1;
    QSqlQuery query;
    if (!query.prepare(QString("SELECT Name FROM Scares WHERE id = %1").arg(id)))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    while (query.next()) {
        m_randomScare = query.value(0).toString();
    }
    query.finish();
}


/**
 * @brief Player::startPlayList
 */
void Player::startPlayList(int clipIndex)
{
    m_device->stop(1, m_defaultLayer);
    if (m_random) {
        m_currentClipIndex = QRandomGenerator::global()->bounded(m_playlistClips.size());
    } else {
        m_currentClipIndex = clipIndex;
    }
    m_nextClipIndex = m_currentClipIndex;
    m_timecode = 100;  // By faking the present timecode, the start of the clip (follows) will trigger loadNextClip()
    m_device->playMovie(1, m_defaultLayer, m_playlistClips[m_currentClipIndex], "", 0, "", "", 0, 0, false, false);
    m_singlePlay = false;
    emit activeClip(m_currentClipIndex);
    setStatus(PlayerStatus::PLAYLIST_PLAYING);
    startSoundScape("EXTRAS/SOUNDSCAPE");  // TODO: SoundScape cLip name should not be hardcoded
}

/**
 * @brief Player::pausePlayList
 */
void Player::pausePlayList()
{
    m_device->pause(1, m_defaultLayer);
    if (m_singlePlay) {
        setStatus(PlayerStatus::CLIP_PAUSED);
    } else {
        setStatus(PlayerStatus::PLAYLIST_PAUSED);
    }
    pauseSoundScape();
}


/**
 * @brief Player::resumePlayList
 */
void Player::resumePlayList()
{
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
}

void Player::insertPlaylist(QString clipName)
{
    pauseSoundScape();
    m_device->stop(1, m_defaultLayer);
    int frameStopped = m_currentFrame;
    m_interrupted = true;
    if (clipName == "random") {
        m_interruptedClipName = m_randomScare;
    } else {
        m_interruptedClipName = clipName;
    }
    if (getStatus() != PlayerStatus::PLAYLIST_PLAYING && getStatus() != PlayerStatus::PLAYLIST_INSERT) {
        m_timecode = 10.0;
        m_singlePlay = true;
    }
    m_nextClipIndex = m_currentClipIndex;
    qDebug() << "Interrupted" << m_playlistClips[m_currentClipIndex];
    m_device->playMovie(1, m_defaultLayer, m_interruptedClipName, "", 0, "", "", 0, 0, false, false);
    if (!m_singlePlay) {
        m_device->loadMovie(1, m_defaultLayer, m_playlistClips[m_currentClipIndex], "", 0, "", "", frameStopped, 0, false, false, true);
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
        qDebug() << "Writing" << m_playlistClips[m_currentClipIndex];
        midiLog->openMidiLog(m_playlistClips[m_currentClipIndex]);
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
    m_timecode = 10.0;
    m_singlePlay = true;
    m_currentClipIndex = clipIndex;
    m_nextClipIndex = clipIndex;
    m_device->playMovie(1, m_defaultLayer, m_playlistClips[m_currentClipIndex], "", 0, "", "", 0, 0, false, false);
    setStatus(PlayerStatus::CLIP_PLAYING);
}

/**
 * @brief Player::playClip
 * @param clipName
 */
void Player::playClip(QString clipName)
{
    m_timecode = 10.0;
    m_singlePlay = true;
    m_currentClipIndex = getClipIndexByName(clipName);
    m_nextClipIndex = m_currentClipIndex;
    m_device->playMovie(1, m_defaultLayer, clipName, "", 0, "", "", 0, 0, false, false);
    setStatus(PlayerStatus::CLIP_PLAYING);
}

int Player::getClipIndexByName(QString clipName) const
{
    for (int i = 0; i < m_playlistClips.size(); i++) {
        if (m_playlistClips[i].data() == clipName) {
            qDebug() << "Found clip" << i;
            return i;
        }
    }
    return 0;
}

/**
 * SOUNDSCAPE
 */

void Player::startSoundScape(QString clipName)
{
    qDebug() << "startSoundScape";
    retrieveMidiSoundScape(clipName);
    m_device->playMovie(1, m_soundScapeLayer, clipName, "", 0, "", "", 0, 0, true, true);
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
    if (!m_interrupted) {
        m_currentClipIndex = m_nextClipIndex;
        emit activeClip(m_currentClipIndex);
        qDebug() << "Playing:" << m_playlistClips[m_currentClipIndex];
        retrieveMidiPlayList(m_playlistClips[m_currentClipIndex]);
        if (midiRead->isReady()) {
            qDebug("MIDI file found...");
            pauseSoundScape();
        }
        else {
            qDebug("No MIDI file found...");
            resumeSoundScape();
        }
        if (m_recording) {
            midiLog->openMidiLog(m_playlistClips[m_currentClipIndex]);
        }
        if (!m_singlePlay) {
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
            loadClip(m_playlistClips[m_nextClipIndex]);
            setStatus(PlayerStatus::PLAYLIST_PLAYING);
        }
        if (m_insert) {
            emit insertFinished();
            m_insert = false;
        }
        emit activeClipName(m_playlistClips[m_currentClipIndex], m_playlistClips[m_nextClipIndex]);
    } else {
        emit activeClipName(m_interruptedClipName, m_playlistClips[m_nextClipIndex], true);
        qDebug() << "Playing:" << m_interruptedClipName;
        retrieveMidiPlayList(m_interruptedClipName);
        if (midiRead->isReady()) {
            qDebug("MIDI file found...");
        }
        else {
            qDebug("No MIDI file found...");
        }
        if (m_recording) {
            midiLog->openMidiLog(m_interruptedClipName);
        }
        m_interrupted = false;
        m_insert = true;
        setStatus(PlayerStatus::PLAYLIST_INSERT);
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
        double prev_timecode = m_timecode;
        m_timecode = time;
        if (prev_timecode > m_timecode && m_timecode < 1.0) {
            loadNextClip();
    //        i = midiPlayList.begin();   // EXPERIMENT WITH ITERATOR
        } if (qFabs(prev_timecode - m_timecode) < 0.001) {
            qDebug() << "Clip stopped";
            if (m_insert) {
                emit insertFinished();
                if (!m_singlePlay)
                    loadNextClip();
                m_insert = false;
            }
            midiLog->closeMidiLog();
            if (m_singlePlay) {
                qDebug() << "stopPlayList()";
                stopPlayList();
            }
        }
        QString timecode = Timecode::fromTime(time, 29.97, false);
    //    if (i != midiPlayList.end()) {                   // EXPERIMENT WITH ITERATOR
    //        if (i->timeCode <= timecode) {
    //            qDebug() << "Bang" << i->timeCode;
    //            i++;
    //        }
    //    }
        if (m_triggersActive && midiPlayList.contains(timecode)) {
            if (midiPlayList[timecode].type == "ON") {
                playNote(midiPlayList[timecode].pitch, true);
            } else {
                playNote(midiPlayList[timecode].pitch, false);
            }
        }
    } else if (videoLayer == m_soundScapeLayer && m_soundScapeActive) {
        QString timecode = Timecode::fromTime(time, 29.97, false);
        if (m_triggersActive && midiSoundScape.contains(timecode)) {
            if (midiSoundScape[timecode].type == "ON") {
                playNote(midiSoundScape[timecode].pitch, true);
            } else {
                playNote(midiSoundScape[timecode].pitch, false);
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
    emit newMidiPlaylist(midiPlayList);
}

void Player::retrieveMidiSoundScape(QString clipName)
{
    midiSoundScape = midiRead->openLog(clipName);
}
