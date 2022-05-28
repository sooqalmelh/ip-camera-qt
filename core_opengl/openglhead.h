#ifndef OPENGLHEAD_H
#define OPENGLHEAD_H

#include "qdatetime.h"

#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef TIMEMS
#define TIMEMS qPrintable(QTime::currentTime().toString("HH:mm:ss zzz"))
#endif

#endif // OPENGLHEAD_H
