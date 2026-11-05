#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gfc_audio.h"

#include "model.h"
#include "entity.h"

typedef struct
{
	Model* sky;
	GFC_TextLine skyFile;
	Model* terrain;
	GFC_TextLine terrainFile;
	GFC_Matrix4 terrainMatrix;
	Model* tile;
	GFC_TextLine tileFile;
	Uint8 tileDim;
	Model* wall;
	GFC_TextLine wallFile;
	GFC_Box bounds; //don't let anything leave this
	Mix_Music* backgroundMusic;
	GFC_TextLine musicFile;
	SJson* json; // keep the json around in case we need to reference it
	Uint8* tileMap;
	Entity** entityMap;
	GFC_Vector2I tileSize; // how many tiles wide and tall
}Level;

Uint8 level_line_test(Level* level, GFC_Edge3D edge, GFC_Vector3D* contact);

/**
* @brief check if the bounding box provided is clipping or outside the level bounds
* @param level the level to compare
* @param aabb the bounding box in world space
*/
Uint8 level_bounds_check(Level* level, GFC_Box aabb);

Level* level_new();

Level* level_load(const char* filename);

Level* level_get_the();

void level_free(Level* level);

void level_draw(Level* level);

/**
* @brief returns the tile index for a given x and y
* @param level the level to check
* @param x the x-position of the tile in the 2D tilemap
* @param y the y-position of the tile in the 2D tilemap
* @return -1 on failure, index of the tile otherwise
*/
Sint32 level_get_tile_index(Level* level, Uint16 x, Uint16 y);

/**
* @brief translates level coordinates to tilemap coordinates
* @param level the level to check
* @param x the x-position in the coordinate space
* @param y the y-position in the coordinate space
* @return vector (-1, -1) on failure, coordinates on the tilemap otherwise
*/
GFC_Vector2D coords_to_tile(Level* level, float x, float y);

#endif
