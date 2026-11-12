#include "simple_logger.h"

#include "turn.h"
#include "player.h"

typedef struct
{
	Uint8 maxPlayers;
	Uint8 maxEnts;
	Uint8 currentEnt;
	Uint16 currentTurn;
	Uint16 maxTurns;
	Entity** turnOrder;
}TurnManager;

static TurnManager turn_manager = {0};

void turn_close(void);

void turn_init(Uint8 maxPlayers, Uint8 maxEnts, Uint16 maxTurns)
{
	if (!maxPlayers || maxEnts <= maxPlayers)
	{
		slog("failed to initialize turn system: no room for players or enemies");
		return;
	}
	else if (!maxTurns)
	{
		slog("failed to initialize turn system: max turns not set");
		return;
	}
	turn_manager.turnOrder = gfc_allocate_array(sizeof(Entity *), maxEnts);
	if (!turn_manager.turnOrder)
	{
		slog("failed to allocate turn order for %i entities", maxEnts);
		return;
	}
	turn_manager.maxPlayers = maxPlayers;
	turn_manager.maxEnts = maxEnts;
	turn_manager.maxTurns = maxTurns;
	turn_manager.currentEnt = 0;
	turn_manager.currentTurn = 0;
	slog("Turn system initialized");
	atexit(turn_close);
}

Uint8 turn_order_add(Entity* ent)
{
	Uint16 i;
	if (!ent || ent->team >= 2) return 0;
	if (ent->team) i = turn_manager.maxPlayers;
	else i = 0;
	for (; i < turn_manager.maxEnts; i++)
	{
		if (turn_manager.turnOrder[i]) continue;
		if (!ent->team && i >= turn_manager.maxPlayers)
		{
			slog("Too many players");
			break;
		}
		turn_manager.turnOrder[i] = ent;
		return 1;
	}
	slog("Could not add entity to the turn order");
	return 0;
}

Uint8 turn_is_my(Entity* ent)
{
	if (!ent) return 0;
	return ent == turn_manager.turnOrder[turn_manager.currentEnt];
}

Uint8 turn_increment(Entity* ent)
{
	int i;
	if (!ent || ent->team >= 2) return 0;
	else if (!turn_is_my(ent))
	{
		slog("Number 2: Wait your turn, f***er!");
		return 0;
	}

	for (i = turn_manager.currentEnt + 1; i < turn_manager.maxEnts; i++)
	{
		if (!turn_manager.turnOrder[i])
		{
			if (i >= turn_manager.maxPlayers && gfc_random() * 200 < 1)
			{
				pokemon_enemy_spawn_random();
			}
			continue;
		}
		turn_manager.currentEnt = i;
		//slog("Turn %i, ent %i", turn_manager.currentTurn, turn_manager.currentEnt);
		return 1;
	}
	
	turn_manager.currentEnt = 0;
	turn_manager.currentTurn++;
	for (i = 0; i < turn_manager.maxPlayers; i++)
	{
		if (!turn_manager.turnOrder[i] || turn_manager.turnOrder[i]->team) continue;
		if (turn_manager.turnOrder[i]->hp < turn_manager.turnOrder[i]->maxHP)
			turn_manager.turnOrder[i]->hp++;
	}
	// add code for nearing/reaching max turns here
	//slog("Turn %i, ent %i", turn_manager.currentTurn, turn_manager.currentEnt);
	return 1;
}

Uint8 turn_remove(Entity* ent)
{
	int i, j;
	if (!ent || !turn_manager.turnOrder) return 0;
	
	j = -1;
	for (i = 0; i < turn_manager.maxEnts; i++)
	{
		if (ent == turn_manager.turnOrder[i])
		{
			j = i;
			break;
		}
	}
	if (j == -1)
	{
		slog("Cannot remove entity that was not found in turn order");
		return 0;
	}
	//if (turn_is_my(ent)) turn_increment(ent); not happening w/ current code
	//if (j < turn_manager.currentEnt && turn_manager.currentEnt != turn_manager.maxPlayers)
	//	turn_manager.currentEnt--; this check is just bad and dumb

	for (; i < turn_manager.maxEnts; i++)
	{
		if (i == turn_manager.maxEnts - 1 || i == turn_manager.maxPlayers - 1)
		{
			turn_manager.turnOrder[i] = NULL;
			break;
		}
		turn_manager.turnOrder[i] = turn_manager.turnOrder[i + 1];
	}

	return 1;
}

void turn_reset()
{
	turn_manager.currentEnt = 0;
	turn_manager.currentTurn = 0;
}

Uint8 turn_set_player(Entity* ent, Uint8 rotate)
{
	int i, j;
	if (!ent || ent->team) return 0;
	for (i = 0; i < turn_manager.maxPlayers + 1; i++)
	{
		if (i == turn_manager.maxPlayers)
		{
			slog("Entity cannot be a player");
			return 0;
		}
		if (ent == turn_manager.turnOrder[i]) break;
	}

	if (i == 0)
	{
		return 1;
	}
	if (rotate)
	{
		for (j = i; j > 0; j--)
		{
			turn_manager.turnOrder[j] = turn_manager.turnOrder[j - 1];
		}
		turn_manager.turnOrder[0] = ent;
	}
	player_set(ent);
	return 1;
}

Uint8 turn_next_player()
{
	Entity* player;
	int i, j;
	player = player_get_the();
	for (i = 0; i < turn_manager.maxPlayers; i++)
	{
		if (!turn_manager.turnOrder[i]) continue;
		if (turn_manager.turnOrder[i]->team) continue;
		if (turn_manager.turnOrder[i]->hp <= 0) continue;
		if (turn_manager.turnOrder[i] == player) continue;
		turn_set_player(turn_manager.turnOrder[i], 0);
		// rotate players (0 <-- 1 <-- 2 <-- 3 <-- 4 <-- 0)
		for (j = i; j < turn_manager.maxPlayers * i; j += i)
		{
			turn_manager.turnOrder[(j - i) % turn_manager.maxPlayers] =
				turn_manager.turnOrder[j % turn_manager.maxPlayers];
		}
		turn_manager.turnOrder[turn_manager.maxPlayers - i] = player;
		return 1;
	}

	return 0;
}

Uint8 turn_anybody_moving()
{
	int i;
	Entity* ent;
	for (i = 0; i < turn_manager.maxEnts; i++)
	{
		ent = turn_manager.turnOrder[i];
		if (!ent) continue;
		if (!gfc_vector3d_compare(ent->position, ent->thinkPos)) return 1;
	}
	return 0;
}

Entity** turn_get_active_entities()
{
	return turn_manager.turnOrder;
}

void turn_close(void)
{
	if (!turn_manager.turnOrder) return;
	free(turn_manager.turnOrder);
	memset(&turn_manager, 0, sizeof(TurnManager));
	slog("Turn system closed");
}