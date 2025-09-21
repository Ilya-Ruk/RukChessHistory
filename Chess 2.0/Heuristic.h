// Heuristic.h

#pragma once

#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "Board.h"
#include "Def.h"

#define MAX_HEURISTIC_SCORE		(1 << 25)

#define BONUS(Depth)			((Depth) * (Depth))

#ifdef COMMON_HEURISTIC_TABLE
extern volatile int HeuristicTable[2][6][64]; // [Color][Piece][Square]
#endif // COMMON_HEURISTIC_TABLE

#ifdef MOVES_SORT_HEURISTIC

void UpdateHeuristic(BoardItem* Board, const int Move, const int Bonus);

void ClearHeuristic(BoardItem* Board);

#endif // MOVES_SORT_HEURISTIC

#endif // !HEURISTIC_H