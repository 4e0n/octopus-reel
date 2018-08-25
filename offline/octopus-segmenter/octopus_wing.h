#ifndef OCTOPUS_WING_H
#define OCTOPUS_WING_H

#include <QColor>
#include "octopus_vec3.h"

typedef struct _Vertex { Vec3 r;   } Vertex;
typedef struct _Edge   { int v[2]; } Edge;
typedef struct _Face   { int v[3]; } Face;
typedef struct _Wing   { int f[2]; } Wing;
typedef struct _SNeigh { int v[3]; } SNeigh;

#endif
