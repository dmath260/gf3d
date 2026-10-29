#ifndef __DRAW_3D_H__
#define __DRAW_3D_H__

#include "gfc_primitives.h"

#include "model.h"

void draw_3d_init();

void draw_3d_cube(GFC_Box cube, GFC_Color colorMod);

void draw_3d_sphere(GFC_Sphere sphere, GFC_Color colorMod);

//void draw_3d_cylinder(GFC_Box cube, GFC_Color colorMod);

#endif
