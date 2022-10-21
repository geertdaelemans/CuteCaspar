#include "ClipInfo.h"

ClipInfo::ClipInfo()
{
    m_name = QString();
    m_fps = FRM_DEF;
    m_id = 0;
    m_duration = 0.0;
}

double ClipInfo::getFps()
{
    if(m_fps<1.0) m_fps = FRM_DEF;
    return m_fps;
}

void ClipInfo::setFps(double fps)
{
    m_fps = fps;
    if(m_fps<1.0) m_fps = FRM_DEF;
}
