#include "simple_logger.h"
#include "simple_json.h"

#include "gfc_config_def.h"

#include "battle.h"

typedef struct
{
	int* typeChart;
}TypeManager;

typedef struct
{
	BattleMove* moveList;
	Uint16 maxMoves;
}MoveManager;

Uint8 battle_type_chart_populate();
void battle_close(void);

TypeManager type_manager = {0};
MoveManager move_manager = {0};

void battle_init()
{
	type_manager.typeChart = gfc_allocate_array(sizeof(int), BT_TOTAL * BT_TOTAL);
	if (!battle_type_chart_populate())
	{
		slog("Failed to initialize battle system: type chart not loaded properly");
		battle_close();
		return;
	}
	move_manager.maxMoves = gfc_config_def_get_resource_count("moves");
	if (!move_manager.maxMoves)
	{
		slog("Failed to initialize battle system: moves not loaded");
		battle_close();
		return;
	}
	move_manager.moveList = gfc_allocate_array(
		sizeof(BattleMove),
		gfc_config_def_get_resource_count("moves")
	);
	atexit(battle_close);
}

Uint8 battle_type_chart_populate()
{
	SJson* typeDef, *rows, *col, *cell;
	int i, j;
	typeDef = gfc_config_def_get_by_name("types", "typeChart");
	slog("A");
	if (!typeDef || !type_manager.typeChart) return 0;

	rows = sj_object_get_value(typeDef, "typeChart");
	slog("B");
	if (sj_array_count(rows) != BT_TOTAL) return 0;

	col = sj_array_nth(rows, 0);
	slog("C");
	if (sj_array_count(col) != BT_TOTAL) return 0;

	for (j = 0; j < BT_TOTAL; j++)
	{
		col = sj_array_get_nth(rows, j);
		if (!col) continue;
		for (i = 0; i < BT_TOTAL; i++)
		{
			cell = sj_array_get_nth(col, i);
			if (!cell) continue;
			sj_get_integer_value(cell, &type_manager.typeChart[i + j * BT_TOTAL]);
		}
	}

	slog("D");
	return 1;
}

BattleType battle_type_to_enum(const char* type)
{
	// ugly code warning lol
	if (!type) return BT_NONE;
	if (!gfc_stricmp(type, "Normal")) return BT_NORMAL;
	if (!gfc_stricmp(type, "Fire")) return BT_FIRE;
	if (!gfc_stricmp(type, "Water")) return BT_WATER;
	if (!gfc_stricmp(type, "Grass")) return BT_GRASS;
	if (!gfc_stricmp(type, "Electric")) return BT_ELECTRIC;
	if (!gfc_stricmp(type, "Ice")) return BT_ICE;
	if (!gfc_stricmp(type, "Fighting")) return BT_FIGHTING;
	if (!gfc_stricmp(type, "Poison")) return BT_POISON;
	if (!gfc_stricmp(type, "Ground")) return BT_GROUND;
	if (!gfc_stricmp(type, "Flying")) return BT_FLYING;
	if (!gfc_stricmp(type, "Psychic")) return BT_PSYCHIC;
	if (!gfc_stricmp(type, "Bug")) return BT_BUG;
	if (!gfc_stricmp(type, "Rock")) return BT_ROCK;
	if (!gfc_stricmp(type, "Ghost")) return BT_GHOST;
	if (!gfc_stricmp(type, "Dragon")) return BT_DRAGON;
	if (!gfc_stricmp(type, "Dark")) return BT_DARK;
	if (!gfc_stricmp(type, "Steel")) return BT_STEEL;
	return BT_NONE;
}

BattleCategory battle_category_to_enum(const char* category)
{
	// somewhat less ugly
	if (!category) return BC_ERROR;
	if (!gfc_stricmp(category, "Physical")) return BC_PHYSICAL;
	if (!gfc_stricmp(category, "Special")) return BC_SPECIAL;
	if (!gfc_stricmp(category, "Status")) return BC_STATUS;
	return BC_ERROR;
}

BattleTarget battle_target_to_enum(const char* target)
{
	// somewhat less ugly
	if (!target) return BTG_ERROR;
	if (!gfc_stricmp(target, "User")) return BTG_USER;
	if (!gfc_stricmp(target, "Enemy")) return BTG_ENEMY;
	if (!gfc_stricmp(target, "All")) return BTG_ALL;
	return BTG_ERROR;
}

BattleMove* battle_new_move()
{
	int i;
	if (!move_manager.moveList) return NULL;
	for (i = 0; i < move_manager.maxMoves; i++)
	{
		if (move_manager.moveList[i].maxPP) continue; // never 0 for a valid move

		// ANY BOILERPLATE CODE GOES HERE
		move_manager.moveList[i].criticalHitRate = 8;

		return &move_manager.moveList[i];
	}
	return NULL;
}

void battle_delete_move(BattleMove* move)
{
	if (!move) return;
	memset(move, 0, sizeof(BattleMove));
}

BattleMove* battle_load_move(const char* name)
{
	SJson* moveDef;
	BattleMove* move;
	Uint8 i;
	const char* str;
	if (!name) return NULL;
	move = battle_new_move();
	if (!move) return NULL;
	moveDef = gfc_config_def_get_by_name("moves", name);
	if (!moveDef)
	{
		slog("Move not recognized, %s", name);
		battle_delete_move(move);
		return NULL;
	}
	gfc_word_cpy(move->name, name);

	str = sj_object_get_string(moveDef, "type");
	move->type = battle_type_to_enum(str);

	str = sj_object_get_string(moveDef, "category");
	move->category = battle_category_to_enum(str);
	if (move->category == BC_ERROR)
	{
		slog("Move category not recognized, %s", str);
		battle_delete_move(move);
		return NULL;
	}

	if (!sj_object_get_uint8(moveDef, "power", &move->power))
	{
		slog("Move power not found");
		battle_delete_move(move);
		return NULL;
	}
	if (!sj_object_get_uint8(moveDef, "maxPP", &move->maxPP) || !move->maxPP)
	{
		slog("Move maximum PP not found or invalid");
		battle_delete_move(move);
		return NULL;
	}
	if (!sj_object_get_float(moveDef, "maxPP", &move->accuracy) || move->accuracy <= 0)
	{
		slog("Move accuracy not found or invalid");
		battle_delete_move(move);
		return NULL;
	}

	str = sj_object_get_string(moveDef, "target");
	move->target = battle_target_to_enum(str);
	if (move->target == BTG_ERROR)
	{
		slog("Move target not recognized, %s", str);
		battle_delete_move(move);
		return NULL;
	}
	if (sj_object_get_uint8(moveDef, "criticalHitRate", &i)) move->criticalHitRate = i;

	return move;
}

BattleMove* battle_get_move(const char* name)
{
	Uint16 i;
	if (!name) return NULL;
	for (i = 0; i < move_manager.maxMoves; i++)
	{
		if (!move_manager.moveList[i].maxPP) continue;
		if (!gfc_stricmp(name, move_manager.moveList[i].name))
			return &move_manager.moveList[i];
	}
	return battle_load_move(name);
}

void battle_close(void)
{
	Uint16 i;
	if (type_manager.typeChart) free(type_manager.typeChart);
	memset(&type_manager, 0, sizeof(TypeManager));
	if (move_manager.moveList)
	{
		for (i = 0; i < move_manager.maxMoves; i++)
		{
			battle_delete_move(&move_manager.moveList[i]);
		}
		free(move_manager.moveList);
	}
	memset(&move_manager, 0, sizeof(MoveManager));
}