// QuiescenceSearch.h

#pragma once

#ifndef QUIESCENCE_SEARCH_H
#define QUIESCENCE_SEARCH_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

#ifdef QUIESCENCE
int QuiescenceSearch(BoardItem* Board, int Alpha, int Beta, const int Depth, const int Ply, MoveItem* BestMoves, const BOOL IsPrincipal, const BOOL InCheck);
#endif // QUIESCENCE

#endif // !QUIESCENCE_SEARCH_H