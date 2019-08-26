#ifndef CASPAR_GLOBAL_H
#define CASPAR_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CASPAR_LIBRARY)
#  define CASPARSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CASPARSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CASPAR_GLOBAL_H
