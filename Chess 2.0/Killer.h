// Killer.h

#pragma once

#ifndef KILLER_H
#define KILLER_H

#include "Board.h"
#include "Def.h"

#ifdef COMMON_KILLER_MOVE_TABLE
extern volatile int KillerMoveTable[MAX_PLY + 1][2]; // [Max. ply + 1][Two killer moves]
#endif // COMMON_KILLER_MOVE_TABLE

#ifdef KILLER_MOVE
void UpdateKiller(BoardItem* Board, const int Move, const int Ply);

void ClearKiller(BoardItem* Board);
#endif // KILLER_MOVE

#endif // !KILLER_H