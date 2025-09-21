// Heuristic.cpp

#include "stdafx.h"

#include "Heuristic.h"

#include "Board.h"
#include "Def.h"
#include "Utils.h"

#ifdef COMMON_HEURISTIC_TABLE
volatile int HeuristicTable[2][6][64]; // [Color][Piece][Square]
#endif // COMMON_HEURISTIC_TABLE

#ifdef MOVES_SORT_HEURISTIC

void UpdateHeuristic(BoardItem* Board, const int Move, const int Bonus)
{
	#ifdef COMMON_HEURISTIC_TABLE
	HeuristicTable[Board->CurrentColor][PIECE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] += Bonus - HeuristicTable[Board->CurrentColor][PIECE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
	#else
	Board->HeuristicTable[Board->CurrentColor][PIECE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] += Bonus - Board->HeuristicTable[Board->CurrentColor][PIECE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
	#endif // COMMON_HEURISTIC_TABLE
}

void ClearHeuristic(BoardItem* Board)
{
	#ifdef COMMON_HEURISTIC_TABLE
	for (int Color = 0; Color < 2; ++Color) {
		for (int Piece = 0; Piece < 6; ++Piece) {
			for (int Square = 0; Square < 64; ++Square) {
				HeuristicTable[Color][Piece][Square] = 0;
			}
		}
	}
	#else
	memset(Board->HeuristicTable, 0, sizeof(Board->HeuristicTable));
	#endif // COMMON_HEURISTIC_TABLE
}

#endif // MOVES_SORT_HEURISTIC