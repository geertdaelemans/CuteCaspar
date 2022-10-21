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
    double  m_duration;
public:
    QString getName() const {return m_name;}
    double  getFps();
    int     getId() {return m_id;}
    double  getDuration() {return m_duration;}

    void    setName(QString name) {m_name = name;}
    void    setFps(double fps);
    void    setId(int id) {m_id = id;}
    void    setDuration(double dur) {m_duration = dur;}
};

#endif // CLIPINFO_H
