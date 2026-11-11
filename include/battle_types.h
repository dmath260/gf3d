#ifndef __BATTLE_ENUMS_H__
#define __BATTLE_ENUMS_H__

typedef enum
{
	BT_NORMAL,
	BT_FIRE,
	BT_WATER,
	BT_GRASS,
	BT_ELECTRIC,
	BT_ICE,
	BT_FIGHTING,
	BT_POISON,
	BT_GROUND,
	BT_FLYING,
	BT_PSYCHIC,
	BT_BUG,
	BT_ROCK,
	BT_GHOST,
	BT_DRAGON,
	BT_DARK,
	BT_STEEL,
	BT_NONE,
	BT_TOTAL // used solely as a constant for # of types
}BattleType;

typedef enum
{
	BC_PHYSICAL,
	BC_SPECIAL,
	BC_STATUS,
	BC_ERROR // invalid only
} BattleCategory;

typedef enum
{
	BTG_USER,
	BTG_ENEMY,
	BTG_ALL,
	// there are more but this should be good for now
	BTG_ERROR // invalid only
} BattleTarget;

typedef enum
{
	BS_ATTACK,
	BS_DEFENSE,
	BS_ACCURACY,
	BS_EVASIVENESS,
	BS_SPECIALATTACK,
	BS_SPECIALDEFENSE,
	BS_ERROR
} BattleStat;

typedef struct
{
	BattleStat stat;
	Sint8 statChange;
	BattleTarget target;
} BattleStatChanges;

typedef struct
{
	GFC_TextWord name;
	BattleType type;
	BattleCategory category;
	Uint8 power;
	Uint8 maxPP;
	float accuracy;
	// move range
	BattleTarget target;
	// cuts corners (boolean)
	// secondary effect rate (float)
	Uint8 criticalHitRate;
	BattleStatChanges* statChanges; // one for now, change if adding moves w/ multiple
}BattleMove;

#endif
