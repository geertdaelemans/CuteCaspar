#include "Player.h"

#include <QSqlQuery>
#include <QtSql>

#include "MidiConnection.h"

#include "Timecode.h"

Player::Player(CasparDevice* device)
{
    m_device = device;
    m_status = PlayerStatus::IDLE;

    midiRead = new MidiReader();
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
        qDebug() << query.value(0).toString();
    }
    query.finish();
    m_status = PlayerStatus::PLAYLIST_LOADED;
}


/**
 * @brief Player::startPlayList
 */
void Player::startPlayList()
{
    m_device->stop(1, 0);
    m_timecode = 100;  // By faking the present timecode, the start of the clip (follows) will trigger loadNextClip()
    m_device->playMovie(1, 0, m_playlistClips[0], "", 0, "", "", 0, 0, false, false);
    m_currentClipIndex = 0;
    m_singlePlay = false;
    emit activeClip(0);
    m_status = PlayerStatus::PLAYLIST_PLAYING;
}

/**
 * @brief Player::pausePlayList
 */
void Player::pausePlayList()
{
    m_device->pause(1, 0);
    m_status = PlayerStatus::PLAYLIST_PAUSED;
}


void Player::resumePlayList()
{
    m_device->resume(1, 0);
    m_status = PlayerStatus::PLAYLIST_PLAYING;
}


/**
 * @brief Player::stopPlayList
 */
void Player::stopPlayList()
{
    m_device->stop(1, 0);
    m_status = PlayerStatus::PLAYLIST_LOADED;
}


/**
 * @brief Player::playClip
 * @param clipName
 */
void Player::playClip(QString clipName)
{
    m_timecode = 0.0;
    m_singlePlay = true;
    m_device->playMovie(1, 0, clipName, "", 0, "", "", 0, 0, false, false);
    m_status = PlayerStatus::CLIP_PLAYING;
}


/**
 * @brief Player::getStatus
 * @return
 */
PlayerStatus Player::getStatus()
{
    return m_status;
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
    emit activeClip(m_currentClipIndex);
    emit activeClipName(m_playlistClips[m_currentClipIndex]);
    qDebug() << m_playlistClips[m_currentClipIndex];
    midiPlayList = midiRead->openLog(m_playlistClips[m_currentClipIndex]);
    if (midiRead->isReady()) {
        qDebug("MIDI file found...");
    }
    else {
        qDebug("No MIDI file found...");
    }

    if (m_currentClipIndex > (m_playlistClips.size()-2)) {
        m_currentClipIndex = 0;
    } else {
        m_currentClipIndex++;
    }
    loadClip(m_playlistClips[m_currentClipIndex]);
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
        qDebug() << "loadNextClip()";
        loadNextClip();
    } if (qFabs(prev_timecode - m_timecode) < 0.001) {
        qDebug() << "Clip stopped" << prev_timecode - m_timecode;
        if (m_singlePlay) {
            stopPlayList();
        }
    }
    QString timecode = Timecode::fromTime(time, 29.97, false);
    if (midiPlayList.contains(timecode)) {
        qDebug() << "found";
        if (midiPlayList[timecode].type == "ON") {
            emit playNote(midiPlayList[timecode].pitch);
        } else {
            emit killNote(midiPlayList[timecode].pitch);
        }
    }
}
