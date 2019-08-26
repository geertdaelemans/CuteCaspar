#ifndef QMIDI_GLOBAL_H
#define QMIDI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QMIDI_LIBRARY)
#  define QMIDISHARED_EXPORT Q_DECL_EXPORT
#else
#  define QMIDISHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QMIDI_GLOBAL_H
