// Search.h

#pragma once

#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

#ifndef ABDADA
int Search(BoardItem* Board, int Alpha, int Beta, int Depth, const int Ply, MoveItem* BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL UsePruning, const int SkipMove);
#endif // !ABDADA

#endif // !SEARCH_H