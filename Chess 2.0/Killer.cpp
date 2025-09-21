// Killer.cpp

#include "stdafx.h"

#include "Killer.h"

#include "Board.h"
#include "Def.h"

#ifdef COMMON_KILLER_MOVE_TABLE
volatile int KillerMoveTable[MAX_PLY + 1][2]; // [Max. ply + 1][Two killer moves]
#endif // COMMON_KILLER_MOVE_TABLE

#ifdef KILLER_MOVE

void UpdateKiller(BoardItem* Board, const int Move, const int Ply)
{
	#ifdef KILLER_MOVE_2
	int TempMove;
	#endif // KILLER_MOVE_2

	#ifdef COMMON_KILLER_MOVE_TABLE
	if (KillerMoveTable[Ply][0] != Move) {
		#ifdef KILLER_MOVE_2
		TempMove = KillerMoveTable[Ply][0];
		#endif // KILLER_MOVE_2

		KillerMoveTable[Ply][0] = Move;

		#ifdef KILLER_MOVE_2
		KillerMoveTable[Ply][1] = TempMove;
		#endif // KILLER_MOVE_2
	}
	#else
	if (Board->KillerMoveTable[Ply][0] != Move) {
		#ifdef KILLER_MOVE_2
		TempMove = Board->KillerMoveTable[Ply][0];
		#endif // KILLER_MOVE_2

		Board->KillerMoveTable[Ply][0] = Move;

		#ifdef KILLER_MOVE_2
		Board->KillerMoveTable[Ply][1] = TempMove;
		#endif // KILLER_MOVE_2
	}
	#endif // COMMON_KILLER_MOVE_TABLE
}

void ClearKiller(BoardItem* Board)
{
	#ifdef COMMON_KILLER_MOVE_TABLE
	for (int Ply = 0; Ply < (MAX_PLY + 1); ++Ply) {
		KillerMoveTable[Ply][0] = 0;
		KillerMoveTable[Ply][1] = 0;
	}
	#else
	memset(Board->KillerMoveTable, 0, sizeof(Board->KillerMoveTable));
	#endif // COMMON_KILLER_MOVE_TABLE
}

#endif // KILLER_MOVE