#include "simple_logger.h"
#include "simple_json.h"

#include "gf3d_obj_load.h"

#include "level.h"

static Level* theLevel = NULL;

Uint8 level_line_test(Level* level, GFC_Edge3D edge, GFC_Vector3D *contact)
{
	MeshPrimitive* prim;
	GFC_Triangle3D t;
	int i, c, j;
	GFC_Vector3D out = { -1 };

	if (!level || !level->terrain || !level->terrain->mesh)
	{
		slog("No level data");
		return 0;
	}

	c = gfc_list_count(level->terrain->mesh->primitives);
	for (i = 0; i < c; i++)
	{
		prim = gfc_list_nth(level->terrain->mesh->primitives, i);
		if (!prim || !prim->objData)
		{
			slog("Primitive or its data is NULL");
			continue;
		}
		for (j = 0; j < prim->objData->face_count; j++)
		{
			t.a = prim->objData->faceVertices[prim->objData->outFace[j].verts[0]].vertex;
			t.b = prim->objData->faceVertices[prim->objData->outFace[j].verts[1]].vertex;
			t.c = prim->objData->faceVertices[prim->objData->outFace[j].verts[2]].vertex;
			if (gfc_triangle_edge_test(edge, t, &out))
			{
				if (contact) *contact = out;
				return 1;
			}
		}
	}

	return 0;
}

Uint8 level_bounds_check(Level* level, GFC_Box aabb)
{
	if (!level) return 0;
	if (
		aabb.x < level->bounds.x || aabb.x + aabb.w > level->bounds.x + level->bounds.w ||
		aabb.y < level->bounds.y || aabb.y + aabb.h > level->bounds.y + level->bounds.h ||
		aabb.z < level->bounds.z || aabb.z + aabb.d > level->bounds.z + level->bounds.d
	) return 1;
	return 0;
}

Level* level_new() {
	Level* level;
	level = gfc_allocate_array(sizeof(Level), 1);
	if (!level)
	{
		slog("Failed to allocate level data");
		return NULL;
	}

	// boilerplate init stuff
	gfc_matrix4_identity(level->terrainMatrix);
	return level;
}

Level* level_load(const char* filename)
{
	SJson* json;
	SJson* data;
	const char* str;
	Level* level;
	int i, j, index;
	SJson* rows, * tiles, * tile;

	json = sj_load(filename);
	if (!json)
	{
		slog("Failed to open level file %s", filename);
		return NULL;
	}
	data = sj_object_get_value(json, "level");
	if (!data)
	{
		sj_free(json);
		slog("File %s does not contain level data", filename);
		return NULL;
	}
	level = level_new();
	if (!level)
	{
		sj_free(json);
		return NULL;
	}

	str = sj_object_get_string(data, "sky");
	if (str)
	{
		level->sky = model_load(str);
		if (level->sky) gfc_line_cpy(level->skyFile, str);
	}
	str = sj_object_get_string(data, "terrain");
	if (str)
	{
		level->terrain = model_load(str);
		if (level->terrain) gfc_line_cpy(level->terrainFile, str);
	}
	str = sj_object_get_string(data, "tile");
	if (str)
	{
		level->tile = model_load(str);
		if (level->tile) gfc_line_cpy(level->tileFile, str);
	}
	sj_object_get_int(data, "tileDim", &level->tileDim);
	if (!level->tileDim)
	{
		level_free(level);
		sj_free(json);
		return NULL;
	}

	rows = sj_object_get_value(data, "tilemap");
	level->tileSize.y = sj_array_count(rows);
	if (!level->tileSize.y)
	{
		level_free(level);
		sj_free(json);
		return NULL;
	}

	tiles = sj_array_nth(rows, 0);
	level->tileSize.x = sj_array_count(tiles);
	if (!level->tileSize.x)
	{
		level_free(level);
		sj_free(json);
		return NULL;
	}

	level->tileMap = gfc_allocate_array(sizeof(Uint8), level->tileSize.x * level->tileSize.y);
	if (!level->tileMap)
	{
		level_free(level);
		sj_free(json);
		return NULL;
	}

	for (j = 0; j < level->tileSize.y; j++)
	{
		tiles = sj_array_get_nth(rows, j);
		if (!tiles) continue;
		for (i = 0; i < level->tileSize.x; i++)
		{
			tile = sj_array_get_nth(tiles, i);
			if (!tile) continue;
			index = level_get_tile_index(level, i, j);
			if (index == -1) continue;
			sj_get_uint8_value(tile, &level->tileMap[index]);
		}
	}
	
	level->entityMap = gfc_allocate_array(sizeof(Entity *), level->tileSize.x * level->tileSize.y);
	if (!level->entityMap)
	{
		level_free(level);
		sj_free(json);
		return NULL;
	}

	level->bounds = gfc_box(
		0,
		0,
		-20,
		level->tileDim * level->tileSize.x,
		level->tileDim * level->tileSize.y,
		40
	);

	/*
	for (i = 0; i < level->tileSize.x; i++)
	{
		for (j = 0; j < level->tileSize.y; j++)
		{
			slog("(%i, %i): %i", i, j, level->tileMap[level_get_tile_index(level, i, j)]);
		}
	}
	*/

	str = sj_object_get_string(data, "music");
	if (str)
	{
		level->backgroundMusic = gfc_sound_load_music(str);
		if (level->backgroundMusic)
		{
			gfc_line_cpy(level->musicFile, str);
			Mix_PlayMusic(level->backgroundMusic, -1);
		}
	}

	level->json = json;
	theLevel = level;
	return level;
}

Level* level_get_the()
{
	return theLevel;
}

void level_free(Level* level)
{
	if (!level) return;
	model_free(level->sky);
	model_free(level->terrain);
	Mix_HaltMusic();
	Mix_FreeMusic(level->backgroundMusic);
	sj_free(level->json);
	free(level);
}

void level_draw(Level* level)
{
	int i, j, k;
	GFC_Matrix4 id;
	if (!level) return;
	gfc_matrix4_identity(id);
	model_queue_render_sky(level->sky, id, GFC_COLOR_WHITE);
	model_queue_render(level->terrain, level->terrainMatrix, GFC_COLOR_WHITE);
	for (i = -8; i < level->tileSize.x + 8; i++)
	{
		for (j = -5; j < level->tileSize.y + 5; j++)
		{
			k = -16;
			while (1)
			{
				gfc_matrix4_from_vectors(
					id,
					gfc_vector3d(
						level->tileDim * i + level->tileDim / 2,
						level->tileDim * j + level->tileDim / 2,
						k
					),
					gfc_vector3d(0, 0, 0),
					gfc_vector3d(1, 1, 1)
				);
				model_queue_render(level->tile, id, GFC_COLOR_WHITE);
				if (k == -16 && (i < 0 || j < 0 || level_get_tile_index(level, i, j) == -1)) k = -8;
				else if (k == -16 && !level->tileMap[level_get_tile_index(level, i, j)]) k = -8;
				else break;
			}
		}
	}
}

Sint32 level_get_tile_index(Level *level, Uint16 x, Uint16 y)
{
	if (!level || !level->tileMap) return -1;
	if (x >= level->tileSize.x || y >= level->tileSize.y) return -1;
	return ((y * level->tileSize.x) + x);
}

GFC_Vector2D coords_to_tile(Level *level, float x, float y)
{
	if (!level || !level->tileMap) return gfc_vector2d(-1,-1);
	if (x >= level->tileSize.x * level->tileDim) return gfc_vector2d(-1, -1);
	if (y >= level->tileSize.y * level->tileDim) return gfc_vector2d(-1, -1);
	return gfc_vector2d(floor(x / level->tileDim), floor(y / level->tileDim));
}