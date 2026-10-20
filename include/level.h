#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gfc_audio.h"

#include "model.h"

typedef struct
{
	Model* sky;
	GFC_TextLine skyFile;
	Model* terrain;
	GFC_TextLine terrainFile;
	GFC_Matrix4 terrainMatrix;
	GFC_Box bounds; //don't let anything leave this
	Mix_Music* backgroundMusic;
	GFC_TextLine musicFile;
	SJson* json; // keep the json around in case we need to reference it
}Level;

Uint8 level_line_test(Level* level, GFC_Edge3D edge, GFC_Vector3D* contact);

Level* level_new();

Level* level_load(const char* filename);

Level* level_get_the();

void level_free(Level* level);

void level_draw(Level* level);

#endif
