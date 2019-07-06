#ifndef MAIN_H
#define MAIN_H

#include "Particle.h"

enum Mode { CREATE_FS, IO, IDLE, FORTH };
extern volatile Mode mode_flag;

#endif
