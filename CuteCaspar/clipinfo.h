#ifndef CLIPINFO_H
#define CLIPINFO_H

#include <QString>

#define FRM_DEF 29.97;

class ClipInfo
{
public:
    ClipInfo();
private:
    QString m_name;
    double  m_fps;
    int     m_id;
    int     m_playlistOrder;
    double  m_duration;
    int     m_midi = 0;
public:
    QString getName() const { return m_name; }
    double  getFps();
    int     getId() { return m_id; }
    int     getPlaylistOrder() const { return m_playlistOrder; }
    double  getDuration() {return m_duration;}
    bool    hasMidi() {return m_midi;}

    void    setName(QString name) { m_name = name; }
    void    setFps(double fps);
    void    setId(int id) { m_id = id; }
    void    setDisplayOrder(int displayOrder) { m_playlistOrder = displayOrder; }
    void    setDuration(double dur) { m_duration = dur; }
    void    setMidi(int midi) { m_midi = midi; }
};

#endif // CLIPINFO_H
