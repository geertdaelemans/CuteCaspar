#include "Player.h"

#include <QSqlQuery>
#include <QtSql>
#include <QPushButton>

#include "MidiConnection.h"

#include "Timecode.h"

Player::Player(CasparDevice* device)
{
    m_device = device;
    m_status = PlayerStatus::IDLE;

    midiRead = new MidiReader();
    midiLog = new MidiLogger();
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
}


/**
 * @brief Player::startPlayList
 */
void Player::startPlayList(int clipIndex)
{
    m_device->stop(1, 0);
    m_currentClipIndex = clipIndex;
    m_nextClipIndex = clipIndex;
    m_timecode = 100;  // By faking the present timecode, the start of the clip (follows) will trigger loadNextClip()
    m_device->playMovie(1, 0, m_playlistClips[m_currentClipIndex], "", 0, "", "", 0, 0, false, false);
    m_singlePlay = false;
    emit activeClip(m_currentClipIndex);
    setStatus(PlayerStatus::PLAYLIST_PLAYING);
}

/**
 * @brief Player::pausePlayList
 */
void Player::pausePlayList()
{
    m_device->pause(1, 0);
    if (m_singlePlay) {
        setStatus(PlayerStatus::CLIP_PAUSED);
    } else {
        setStatus(PlayerStatus::PLAYLIST_PAUSED);
    }
}

/**
 * @brief Player::resumePlayList
 */
void Player::resumePlayList()
{
    m_device->resume(1, 0);
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
    m_device->stop(1, 0);
    midiLog->closeMidiLog();
    setStatus(PlayerStatus::READY);
}

void Player::insertPlaylist(QString clipName)
{
    m_device->stop(1, 0);
    m_interrupted = true;
    m_interruptedClipName = clipName;
    if (getStatus() != PlayerStatus::PLAYLIST_PLAYING && getStatus() != PlayerStatus::PLAYLIST_INSERT) {
        m_timecode = 10.0;
        m_singlePlay = true;
    }
    m_nextClipIndex = m_currentClipIndex;
    qDebug() << "Interrupted" << m_playlistClips[m_currentClipIndex];
    m_device->playMovie(1, 0, clipName, "", 0, "", "", 0, 0, false, false);
    if (!m_singlePlay) {
        m_device->loadMovie(1, 0, m_playlistClips[m_currentClipIndex], "", 0, "", "", 0, 0, false, false, true);
    }
    setStatus(PlayerStatus::PLAYLIST_INSERT);
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
    m_device->playMovie(1, 0, m_playlistClips[m_currentClipIndex], "", 0, "", "", 0, 0, false, false);
    setStatus(PlayerStatus::CLIP_PLAYING);
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
    m_device->loadMovie(1, 0, clipName, "", 0, "", "", 0, 0, false, false, true);
}


/**
 * @brief Player::loadNextClip
 */
void Player::loadNextClip()
{
    if (!m_interrupted) {
        m_currentClipIndex = m_nextClipIndex;
        emit activeClip(m_currentClipIndex);
        emit activeClipName(m_playlistClips[m_currentClipIndex]);
        qDebug() << "Playing:" << m_playlistClips[m_currentClipIndex];
        midiPlayList = midiRead->openLog(m_playlistClips[m_currentClipIndex]);
        if (midiRead->isReady()) {
            qDebug("MIDI file found...");
        }
        else {
            qDebug("No MIDI file found...");
        }
        if (m_recording) {
            midiLog->openMidiLog(m_playlistClips[m_currentClipIndex]);
        }
        if (!m_singlePlay) {
            if (m_currentClipIndex > (m_playlistClips.size()-2)) {
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
    } else {
        emit activeClipName(m_interruptedClipName, true);
        qDebug() << "Playing:" << m_interruptedClipName;
        midiPlayList = midiRead->openLog(m_interruptedClipName);
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
void Player::timecode(double time)
{
    double prev_timecode = m_timecode;
    m_timecode = time;
    if (prev_timecode > m_timecode && m_timecode < 1.0) {
        loadNextClip();
    } if (qFabs(prev_timecode - m_timecode) < 0.001) {
        qDebug() << "Clip stopped";
        if (m_insert) {
            emit insertFinished();
            m_insert = false;
        }
        midiLog->closeMidiLog();
        if (m_singlePlay) {
            qDebug() << "stopPlayList()";
            stopPlayList();
        }
    }
    QString timecode = Timecode::fromTime(time, 29.97, false);
    if (!m_renew && midiPlayList.contains(timecode)) {
        if (midiPlayList[timecode].type == "ON") {
            playNote(midiPlayList[timecode].pitch, true);
        } else {
            playNote(midiPlayList[timecode].pitch, false);
        }
    }
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
    }

    QString timecode = Timecode::fromTime(m_timecode, 29.97, false);
    QString onOff = (noteOn ? "ON" : "OFF");
    qDebug() << QString("%1 %2: pitch %3").arg(timecode).arg(onOff).arg(pitch);
    if (midiLog->isReady() && pitch != previousPitch && noteOn) {
        midiLog->writeNote(QString("%1,%2,%3").arg(timecode).arg(onOff).arg(pitch));
    }
    if(noteOn && pitch != previousPitch) {
        MidiConnection::getInstance()->killNote(previousPitch);
        MidiConnection::getInstance()->playNote(pitch);
        emit activateButton(pitch);
    } else {
        MidiConnection::getInstance()->killNote(pitch);
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

void Player::setRenew(bool value)
{
    m_renew = value;
}