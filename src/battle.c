#include "simple_logger.h"
#include "simple_json.h"

#include "gfc_config_def.h"

#include "battle.h"
#include "turn.h"

typedef struct
{
	BattleMove* moveList;
	Uint16 maxMoves;
	int* typeChart;
	int* statChart;
}BattleManager;

Uint8 battle_type_chart_populate();
Uint8 battle_stat_chart_populate();
void battle_close(void);

BattleManager battle_manager = {0};

void battle_init()
{
	battle_manager.typeChart = gfc_allocate_array(sizeof(int), BT_TOTAL * BT_TOTAL);
	battle_manager.statChart = gfc_allocate_array(sizeof(int), 84);
	if (!battle_type_chart_populate())
	{
		slog("Failed to initialize battle system: type chart not loaded properly");
		battle_close();
		return;
	}
	if (!battle_stat_chart_populate())
	{
		slog("Failed to initialize battle system: stat chart not loaded properly");
		battle_close();
		return;
	}
	battle_manager.maxMoves = gfc_config_def_get_resource_count("moves");
	if (!battle_manager.maxMoves)
	{
		slog("Failed to initialize battle system: moves not loaded");
		battle_close();
		return;
	}
	battle_manager.moveList = gfc_allocate_array(
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
	if (!typeDef || !battle_manager.typeChart) return 0;

	rows = sj_object_get_value(typeDef, "typeChart");
	if (sj_array_count(rows) != BT_TOTAL) return 0;

	col = sj_array_nth(rows, 0);
	if (sj_array_count(col) != BT_TOTAL) return 0;

	for (j = 0; j < BT_TOTAL; j++)
	{
		col = sj_array_get_nth(rows, j);
		if (!col) continue;
		for (i = 0; i < BT_TOTAL; i++)
		{
			cell = sj_array_get_nth(col, i);
			if (!cell) continue;
			sj_get_integer_value(cell, &battle_manager.typeChart[i + j * BT_TOTAL]);
		}
	}

	return 1;
}

Uint8 battle_stat_chart_populate()
{
	SJson* statDef, *rows, *col, *cell;
	int i, j;
	statDef = gfc_config_def_get_by_name("stats", "statChart");
	if (!statDef || !battle_manager.statChart) return 0;

	rows = sj_object_get_value(statDef, "statChart");
	if (sj_array_count(rows) != 4) return 0;

	col = sj_array_nth(rows, 0);
	if (sj_array_count(col) != 21) return 0;

	for (j = 0; j < 4; j++)
	{
		col = sj_array_get_nth(rows, j);
		if (!col) continue;
		for (i = 0; i < 21; i++)
		{
			cell = sj_array_get_nth(col, i);
			if (!cell) continue;
			sj_get_integer_value(cell, &battle_manager.statChart[i + j * 21]);
		}
	}

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

BattleStat battle_stat_to_enum(const char* stat)
{
	// somewhat less ugly
	if (!stat) return BS_ERROR;
	if (!gfc_stricmp(stat, "Attack")) return BS_ATTACK;
	if (!gfc_stricmp(stat, "Defense")) return BS_DEFENSE;
	if (!gfc_stricmp(stat, "Accuracy")) return BS_ACCURACY;
	if (!gfc_stricmp(stat, "Evasiveness")) return BS_EVASIVENESS;
	if (!gfc_stricmp(stat, "Special Attack")) return BS_SPECIALATTACK;
	if (!gfc_stricmp(stat, "Special Defense")) return BS_SPECIALDEFENSE;
	return BS_ERROR;
}

const char *battle_enum_to_stat(BattleStat stat)
{
	switch (stat)
	{
		case BS_ATTACK: return "Attack";
		case BS_DEFENSE: return "Defense";
		case BS_ACCURACY: return "Accuracy";
		case BS_EVASIVENESS: return "Evasiveness";
		case BS_SPECIALATTACK: return "Special Attack";
		case BS_SPECIALDEFENSE: return "Special Defense";
		default: return "{error}";
	}
}

BattleMove* battle_new_move()
{
	int i;
	if (!battle_manager.moveList) return NULL;
	for (i = 0; i < battle_manager.maxMoves; i++)
	{
		if (battle_manager.moveList[i].maxPP) continue; // never 0 for a valid move

		// ANY BOILERPLATE CODE GOES HERE
		battle_manager.moveList[i].criticalHitRate = 8;

		return &battle_manager.moveList[i];
	}
	return NULL;
}

void battle_delete_move(BattleMove* move)
{
	if (!move) return;
	if (move->statChanges) memset(move->statChanges, 0, sizeof(BattleStatChanges));
	memset(move, 0, sizeof(BattleMove));
}

BattleMove* battle_load_move(const char* name)
{
	SJson* moveDef, *stat;
	BattleMove* move;
	BattleStatChanges* changes;
	Uint8 i;
	int j;
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
	if (!sj_object_get_float(moveDef, "accuracy", &move->accuracy) || move->accuracy <= 0)
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

	stat = sj_object_get_value(moveDef, "statChange");
	if (stat)
	{
		changes = gfc_allocate_array(sizeof(BattleStatChanges), 1);
		str = sj_object_get_string(stat, "stat");
		changes->stat = battle_stat_to_enum(str);
		str = sj_object_get_string(stat, "target");
		changes->target = battle_target_to_enum(str);
		if (sj_object_get_int(stat, "statChange", &j) && changes->stat != BS_ERROR &&
			changes->target != BTG_ERROR)
		{
			changes->statChange = j;
			move->statChanges = changes;
		}
		else
		{
			slog("Invalid stat data found");
			memset(changes, 0, sizeof(BattleStatChanges));
		}
	}

	return move;
}

BattleMove* battle_get_move(const char* name)
{
	Uint16 i;
	if (!name) return NULL;
	for (i = 0; i < battle_manager.maxMoves; i++)
	{
		if (!battle_manager.moveList[i].maxPP) continue;
		if (!gfc_stricmp(name, battle_manager.moveList[i].name))
			return &battle_manager.moveList[i];
	}
	return battle_load_move(name);
}

float battle_type_matchup(BattleMove* move, Entity* target)
{
	float effectiveness, type1, type2;
	if (!move || !target) return 0.0;
	type1 = battle_manager.typeChart[move->type * BT_TOTAL + target->type1];
	type2 = battle_manager.typeChart[move->type * BT_TOTAL + target->type2];
	effectiveness = type1 * type2 / 65536.0;
	if (effectiveness <= 0.55) slog("It has little effect...");
	else if (effectiveness <= 0.95) slog("It's not very effective...");
	else if (effectiveness > 1.05) slog("It's super effective!");
	return effectiveness;
}

float battle_get_stat_mod(Entity *user, BattleStat stat)
{
	if (!user || stat == BS_ERROR) return 0.0;
	return (float)(battle_manager.statChart[user->statChanges[stat] + 21 * (stat % 4)]) / 256;
}

void battle_use_move(Entity* user, Entity* target, Uint8 moveIndex)
{
	BattleMove* move;
	float baseDamage, mult, atk, def, pow, logInt, acc, atkMod, defMod;
	Sint16 finalDamage;
	Entity* statTarget;
	const char* str;
	if (!user || moveIndex > 3)
	{
		slog("The move failed!");
		return;
	}
	move = user->moves[moveIndex];
	user->currentPP[moveIndex]--;
	if (!move || move->target == BTG_ERROR || move->category == BC_ERROR || !move->maxPP)
	{
		slog("The move failed!");
		return;
	}
	slog("%s used %s!", user->name, move->name);
	if ((move->target == BTG_ENEMY || move->target == BTG_ALL) && (!target || target->team >= 2))
	{
		slog("The move failed!");
		return;
	}
	if (move->target == BTG_ENEMY && target->team == user->team)
	{
		slog("The move failed!");
		return;
	}
	acc = move->accuracy * battle_get_stat_mod(user, BS_ACCURACY);
	acc *= battle_get_stat_mod(target, BS_EVASIVENESS);
	if (gfc_random() < acc)
	{
		if (move->power)
		{
			if (move->category == BC_PHYSICAL)
			{
				atkMod = battle_get_stat_mod(user, BS_ATTACK);
				defMod = battle_get_stat_mod(target, BS_DEFENSE);
				atk = user->attack * atkMod;
				def = target->defense * defMod;
			}
			else if (move->category == BC_SPECIAL)
			{
				atkMod = battle_get_stat_mod(user, BS_SPECIALATTACK);
				defMod = battle_get_stat_mod(target, BS_SPECIALDEFENSE);
				atk = user->specialAttack * atkMod;
				def = target->specialDefense * defMod;
			}
			pow = move->power;
			mult = 1.0;
			if (user->team) mult *= 0.75;
			if (gfc_random() * 100.0 < move->criticalHitRate)
			{
				slog("A critical hit!");
				mult *= 1.5;
			}
			if (move->type == target->type1 || move->type == target->type2) mult *= 1.5;
			mult *= battle_type_matchup(move, target);
			atk *= atkMod;
			def *= defMod;
			pow *= atkMod;
			logInt = 10.0 * (user->level + (atk - def) / 8.0) + 500.0;
			if (logInt < 1.0) logInt = 1.0;
			else if (logInt > 2047.0) logInt = 2047.0;
			baseDamage = mult * (153.0 / 256.0) * (atk + pow) - def / 2.0 + 50.0 * log(logInt) - 311.0;
			finalDamage = round(baseDamage * (1.0 + gfc_crandom() / 8.0));

			if (finalDamage > 0)
			{
				slog("%s took %i damage!", target->name, finalDamage);
				target->hp -= finalDamage;
				if (target->hp <= 0)
				{
					if (target->team) slog("%s was defeated!", target->name);
					else slog("Oh no! %s was defeated!", target->name);
					entity_free(target);
					return;
				}
			}
			else slog("The move failed!");
		}

		if (move->statChanges)
		{
			statTarget = BTG_ERROR;
			if (move->statChanges->target == BTG_USER) statTarget = user;
			else if (move->statChanges->target == BTG_ENEMY) statTarget = target;
			if (statTarget != BTG_ERROR)
			{
				str = battle_enum_to_stat(move->statChanges->stat);
				if (statTarget->statChanges[move->statChanges->stat] + move->statChanges->statChange > 20)
				{
					if (move->statChanges->statChange > 0)
						slog("%s's %s cannot go any higher!", statTarget->name, str);
					else slog("%s's %s cannot go any lower!", statTarget->name, str);
				}
				else
				{
					statTarget->statChanges[move->statChanges->stat] += move->statChanges->statChange;
					if (move->statChanges->statChange > 0)
						slog("%s's %s rose slightly!", statTarget->name, str);
					else slog("%s's %s fell slightly!", statTarget->name, str);
				}
			}
		}
	}
	else
	{
		slog("The move missed %s!", target->name);
	}
}

void battle_close(void)
{
	Uint16 i;
	if (battle_manager.typeChart) free(battle_manager.typeChart);
	if (battle_manager.statChart) free(battle_manager.statChart);
	memset(&battle_manager, 0, sizeof(BattleManager));
	if (battle_manager.moveList)
	{
		for (i = 0; i < battle_manager.maxMoves; i++)
		{
			battle_delete_move(&battle_manager.moveList[i]);
		}
		free(battle_manager.moveList);
	}
	memset(&battle_manager, 0, sizeof(BattleManager));
}