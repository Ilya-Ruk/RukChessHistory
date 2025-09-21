// Search.cpp

#include "stdafx.h"

#include "Search.h"

#include "Board.h"
#include "Def.h"
#include "Evaluate.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "Heuristic.h"
#include "Killer.h"
#include "Move.h"
#include "QuiescenceSearch.h"
#include "SEE.h"
#include "Sort.h"
#include "Types.h"
#include "Utils.h"

#ifndef ABDADA

#ifdef FUTILITY_PRUNING
int FutilityMargin(const int Depth) // Hakkapeliitta
{
	return (25 * Depth + 100);
}
#endif // FUTILITY_PRUNING

#ifdef REVERSE_FUTILITY_PRUNING
int ReverseFutilityMargin(const int Depth) // Hakkapeliitta
{
	return (50 * Depth + 100);
}
#endif // REVERSE_FUTILITY_PRUNING

#ifdef RAZORING
int RazoringMargin(const int Depth) // Hakkapeliitta
{
	return (50 * Depth + 50);
}
#endif // RAZORING

int Search(BoardItem* Board, int Alpha, int Beta, int Depth, const int Ply, MoveItem* BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL UsePruning, const int SkipMove)
{
	int GenMoveCount;
	MoveItem MoveList[MAX_GEN_MOVES];

	int QuietMoveCount = 0;
	int QuietMoveList[MAX_GEN_MOVES]; // Move only

	int LegalMoveCount = 0;

	MoveItem BestMove = { 0, 0, 0 };

	MoveItem TempBestMoves[MAX_PLY];

	#if defined(HASH_SCORE) || defined(HASH_MOVE)
	int HashScore = -INF;
	int HashStaticScore = -INF;
	int HashMove = 0;
	int HashDepth = -MAX_PLY;
	int HashFlag = 0;
	#endif // HASH_SCORE || HASH_MOVE

	int Score;
	int BestScore;
	int StaticScore;

	BOOL GiveCheck;

	int Extension;

	int NewDepth;

	#if defined(REVERSE_FUTILITY_PRUNING) || defined(NULL_MOVE_PRUNING)
	BOOL NonPawnMaterial = (bool)(Board->BB_Pieces[Board->CurrentColor][KNIGHT] | Board->BB_Pieces[Board->CurrentColor][BISHOP] | Board->BB_Pieces[Board->CurrentColor][ROOK] | Board->BB_Pieces[Board->CurrentColor][QUEEN]);
	#endif // REVERSE_FUTILITY_PRUNING || NULL_MOVE_PRUNING

	#ifdef RAZORING
	int RazoringAlpha;
	#endif // RAZORING

	#ifdef NULL_MOVE_PRUNING
	int NullMoveReduction;
	#endif // NULL_MOVE_PRUNING

	#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
	int SEE_Value;
	#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

	#if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
	int LateMoveReduction;
	#endif // NEGA_SCOUT && LATE_MOVE_REDUCTION

	#if defined(SINGULAR_EXTENSION) && defined(HASH_SCORE) && defined(HASH_MOVE)
	int SingularBeta;
	#endif // SINGULAR_EXTENSION && HASH_SCORE && HASH_MOVE

	#ifdef QUIESCENCE
	if (Depth <= 0) {
		return QuiescenceSearch(Board, Alpha, Beta, 0, Ply, BestMoves, IsPrincipal, InCheck);
	}
	#endif // QUIESCENCE

	if (
		Ply > 0
		&& omp_get_thread_num() == 0 // Master thread
		&& (Board->Nodes & 65535) == 0
		&& CompletedDepth >= MIN_SEARCH_DEPTH
		&& Clock() >= TimeStop
	) {
		StopSearch = TRUE;

		return 0;
	}

	if (StopSearch) {
		return 0;
	}

	#ifdef MATE_DISTANCE_PRUNING
	Alpha = MAX(Alpha, -INF + Ply);
	Beta = MIN(Beta, INF - Ply - 1);

	if (Alpha >= Beta) {
		return Alpha;
	}
	#endif // MATE_DISTANCE_PRUNING

	if (Ply > 0) {
		if (IsInsufficientMaterial(Board)) {
			return 0;
		}

		if (Board->FiftyMove >= 100) {
			if (InCheck) {
				GenMoveCount = 0;
				GenerateAllMoves(Board, MoveList, &GenMoveCount);

				for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
					MakeMove(Board, MoveList[MoveNumber]);

					if (IsInCheck(Board, CHANGE_COLOR(Board->CurrentColor))) {
						UnmakeMove(Board);

						continue; // Next move
					}

					// Legal move

					UnmakeMove(Board);

					return 0;
				}

				// No legal move

				return -INF + Ply;
			}

			return 0;
		}

		if (PositionRepeat1(Board)) {
			return 0;
		}
	}

	if (Ply >= MAX_PLY) {
		return Evaluate(Board);
	}

	#ifndef QUIESCENCE
	if (Depth <= 0) {
		return Evaluate(Board);
	}
	#endif // !QUIESCENCE

	if (IsPrincipal && Board->SelDepth < Ply + 1) {
		Board->SelDepth = Ply + 1;
	}

	#if defined(HASH_SCORE) || defined(HASH_MOVE)
	if (!SkipMove) {
		LoadHash(Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

		if (HashFlag) {
			#ifdef DEBUG_STATISTIC
			++Board->HashCount;
			#endif // DEBUG_STATISTIC

			#ifdef HASH_SCORE
			if (!IsPrincipal && HashDepth >= Depth) {
				if (
					(HashFlag == HASH_BETA && HashScore >= Beta)
					|| (HashFlag == HASH_ALPHA && HashScore <= Alpha)
					|| (HashFlag == HASH_EXACT)
				) {
					#if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
					if (HashMove) {
						if (
							Board->Pieces[MOVE_TO(HashMove)] == EMPTY
							&& (
								PIECE(Board->Pieces[MOVE_FROM(HashMove)]) != PAWN
								|| (
									MOVE_TO(HashMove) != Board->PassantSquare
									&& RANK(MOVE_TO(HashMove)) != 0
									&& RANK(MOVE_TO(HashMove)) != 7
								)
							)
						) { // Not capture/promote move
							if (HashFlag == HASH_BETA) {
								#ifdef MOVES_SORT_HEURISTIC
								UpdateHeuristic(Board, HashMove, BONUS(Depth));
								#endif // MOVES_SORT_HEURISTIC

								#ifdef KILLER_MOVE
								UpdateKiller(Board, HashMove, Ply);
								#endif // KILLER_MOVE
							}
							#ifdef MOVES_SORT_HEURISTIC
							else if (HashFlag == HASH_ALPHA) {
								UpdateHeuristic(Board, HashMove, -BONUS(Depth));
							}
							#endif // MOVES_SORT_HEURISTIC
						}
					}
					#endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

					return HashScore;
				}
			} // if
			#endif // HASH_SCORE
		} // if
	} // if
	#endif // HASH_SCORE || HASH_MOVE

	if (InCheck) {
		StaticScore = -INF + Ply;
	}
	else {
		#ifdef HASH_SCORE
		if (HashFlag) {
			StaticScore = HashStaticScore;
		}
		else {
		#endif // HASH_SCORE
			StaticScore = Evaluate(Board);

		#ifdef HASH_SCORE
			if (!SkipMove) {
				SaveHash(Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
			}
		}
		#endif // HASH_SCORE
	}

	#if defined(REVERSE_FUTILITY_PRUNING) || defined(RAZORING) || defined(NULL_MOVE_PRUNING)
	if (UsePruning && !IsPrincipal && !InCheck) {
		#ifdef REVERSE_FUTILITY_PRUNING
		if (NonPawnMaterial && Depth <= 5 && (StaticScore - ReverseFutilityMargin(Depth)) >= Beta) { // Hakkapeliitta
			return (StaticScore - ReverseFutilityMargin(Depth));
		}
		#endif // REVERSE_FUTILITY_PRUNING

		#ifdef RAZORING
		if (Depth <= 3 && (StaticScore + RazoringMargin(Depth)) <= Alpha) { // Hakkapeliitta
			RazoringAlpha = Alpha - RazoringMargin(Depth);

			Score = QuiescenceSearch(Board, RazoringAlpha, RazoringAlpha + 1, 0, Ply, BestMoves, FALSE, FALSE);

			if (Score <= RazoringAlpha) {
				return Score;
			}
		}
		#endif // RAZORING

		#ifdef NULL_MOVE_PRUNING
		if (NonPawnMaterial && Depth > 1 && StaticScore >= Beta) { // Hakkapeliitta
			NullMoveReduction = 3 + Depth / 6;

			MakeNullMove(Board);

			#if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
			Prefetch(Board->Hash);
			#endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

			++Board->Nodes;

			// Zero window search for reduced depth
			TempBestMoves[0] = { 0, 0, 0 };

			Score = -Search(Board, -Beta, -Beta + 1, Depth - 1 - NullMoveReduction, Ply + 1, TempBestMoves, FALSE, FALSE, FALSE, 0);

			UnmakeNullMove(Board);

			if (StopSearch) {
				return 0;
			}

			if (Score >= Beta) {
				if (Score >= INF - MAX_PLY) {
					return Beta;
				}
				else {
					return Score;
				}
			}
		}
		#endif // NULL_MOVE_PRUNING
	} // if
	#endif // REVERSE_FUTILITY_PRUNING || RAZORING || NULL_MOVE_PRUNING

	#if defined(HASH_MOVE) && defined(IID)
	if (!SkipMove && !HashMove && (IsPrincipal ? Depth > 4 : Depth > 7)) { // Hakkapeliitta
		#ifdef DEBUG_IID
		printf("-- IID: Depth = %d\n", Depth);
		#endif // DEBUG_IID

		// Search with full window for reduced depth
		TempBestMoves[0] = { 0, 0, 0 };

		Search(Board, Alpha, Beta, (IsPrincipal ? Depth - 2 : Depth / 2), Ply, TempBestMoves, IsPrincipal, InCheck, FALSE, 0);

		if (StopSearch) {
			return 0;
		}

		LoadHash(Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

		#ifdef DEBUG_IID
		if (HashMove) {
			printf("-- IID: Depth = %d Move = %s%s\n", Depth, BoardName[MOVE_FROM(HashMove)], BoardName[MOVE_TO(HashMove)]);
		}
		#endif // DEBUG_IID
	}
	#endif // HASH_MOVE && IID

	#if defined(HASH_MOVE) && defined(IIR)
	if (!SkipMove && !HashMove && Depth > 4) {
		--Depth;
	}
	#endif // HASH_MOVE && IIR

	BestScore = -INF + Ply;

	GenMoveCount = 0;
	GenerateAllMoves(Board, MoveList, &GenMoveCount);

	#ifdef PVS
	SetPvsMoveSortValue(Board, Ply, MoveList, GenMoveCount);
	#endif // PVS

	#ifdef HASH_MOVE
	SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);
	#endif // HASH_MOVE

	#ifdef KILLER_MOVE
	SetKillerMove1SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
	#ifdef KILLER_MOVE_2
	SetKillerMove2SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
	#endif // KILLER_MOVE_2
	#endif // KILLER_MOVE

	#ifdef KILLER_MOVE
	#ifdef COMMON_KILLER_MOVE_TABLE
	KillerMoveTable[Ply + 1][0] = 0;
	#ifdef KILLER_MOVE_2
	KillerMoveTable[Ply + 1][1] = 0;
	#endif // KILLER_MOVE_2
	#else
	Board->KillerMoveTable[Ply + 1][0] = 0;
	#ifdef KILLER_MOVE_2
	Board->KillerMoveTable[Ply + 1][1] = 0;
	#endif // KILLER_MOVE_2
	#endif // COMMON_KILLER_MOVE_TABLE
	#endif // KILLER_MOVE

	for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
NextMove:
#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

		#if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
		PrepareNextMove(MoveNumber, MoveList, GenMoveCount);
		#endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

		if (MoveList[MoveNumber].Move == SkipMove) {
			continue; // Next move
		}

		#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
		if (
			(MoveList[MoveNumber].Type & MOVE_CAPTURE)
			&& !(MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE)
			&& MoveList[MoveNumber].SortValue >= SORT_CAPTURE_MOVE_BONUS
			#ifdef PVS
			&& MoveList[MoveNumber].SortValue != SORT_PVS_MOVE_VALUE
			#endif // PVS
			#ifdef HASH_MOVE
			&& MoveList[MoveNumber].Move != HashMove
			#endif // HASH_MOVE
		) {
			SEE_Value = CaptureSEE(Board, MOVE_FROM(MoveList[MoveNumber].Move), MOVE_TO(MoveList[MoveNumber].Move), MOVE_PROMOTE_PIECE(MoveList[MoveNumber].Move), MoveList[MoveNumber].Type);

			if (SEE_Value < 0) { // Bad capture move
				MoveList[MoveNumber].SortValue = SEE_Value - SORT_CAPTURE_MOVE_BONUS; // Search move later

				goto NextMove;
			}
		}
		#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

		MakeMove(Board, MoveList[MoveNumber]);

		#if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
		Prefetch(Board->Hash);
		#endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

		if (IsInCheck(Board, CHANGE_COLOR(Board->CurrentColor))) {
			UnmakeMove(Board);

			continue; // Next move
		}

		++LegalMoveCount;

		++Board->Nodes;
/*
		#if !defined(ROOT_SPLITTING) && !defined(PV_SPLITTING)
		if (omp_get_thread_num() == 0) { // Master thread
			if (Ply == 0 && PrintMode == PRINT_MODE_UCI && (Clock() - TimeStart) >= 3000) {
				#pragma omp critical
				{
					printf("info depth %d currmovenumber %d currmove %s%s", Depth, MoveNumber + 1, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);

					if (MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE) {
						printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(MoveList[MoveNumber].Move)]);
					}

					printf(" nodes %llu\n", Board->Nodes);
				}
			}
		}
		#endif // !ROOT_SPLITTING && !PV_SPLITTING
*/
		GiveCheck = IsInCheck(Board, Board->CurrentColor);

		Extension = 0;

		#ifdef CHECK_EXTENSION
		if (!Extension && GiveCheck) {
			Extension = 1;
		}
		#endif // CHECK_EXTENSION

		#if defined(SINGULAR_EXTENSION) && defined(HASH_SCORE) && defined(HASH_MOVE)
		if (
			!Extension
			&& Ply > 0
			&& Depth >= 8
			&& MoveList[MoveNumber].Move == HashMove
			&& HashFlag == HASH_BETA
			&& HashDepth >= Depth - 3
			&& (HashScore > -INF + MAX_PLY && HashScore < INF - MAX_PLY)
		) { // Xiphos
			UnmakeMove(Board);

			SingularBeta = HashScore - Depth;

			// Zero window search for reduced depth
			TempBestMoves[0] = { 0, 0, 0 };

			Score = Search(Board, SingularBeta - 1, SingularBeta, Depth / 2, Ply, TempBestMoves, FALSE, InCheck, FALSE, MoveList[MoveNumber].Move);

			#ifdef DEBUG_SINGULAR_EXTENSION
			PrintBoard(Board);

			printf("-- SE: Ply = %d Depth = %d SkipMove = %s%s HashScore = %d SingularBeta = %d Score = %d\n", Ply, Depth, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)], HashScore, SingularBeta, Score);
			#endif // DEBUG_SINGULAR_EXTENSION

			if (Score < SingularBeta) {
				#ifdef DEBUG_SINGULAR_EXTENSION
				printf("-- SE: Extension = 1\n");
				#endif // DEBUG_SINGULAR_EXTENSION

				Extension = 1;
			}

			MakeMove(Board, MoveList[MoveNumber]);
		}
		#endif // SINGULAR_EXTENSION && HASH_SCORE && HASH_MOVE

		#if defined(FUTILITY_PRUNING) || defined(LATE_MOVE_PRUNING) || defined(SEE_QUIET_MOVE_PRUNING) || defined(SEE_CAPTURE_MOVE_PRUNING)
		if (
			!Extension
			&& !IsPrincipal
			&& !InCheck
			&& !GiveCheck
			#ifdef HASH_MOVE
			&& MoveList[MoveNumber].Move != HashMove
			#endif // HASH_MOVE
		) {
			#if defined(SEE_CAPTURE_MOVE_PRUNING) && (defined(MOVES_SORT_SEE) || defined(BAD_CAPTURE_LAST))
			if (
				Depth <= 3
				&& MoveList[MoveNumber].SortValue < -SORT_CAPTURE_MOVE_BONUS - 100 * Depth // Bad capture move
			) { // Xiphos
				UnmakeMove(Board);

				continue; // Next move
			}
			#endif // SEE_CAPTURE_MOVE_PRUNING && (MOVES_SORT_SEE || BAD_CAPTURE_LAST)

			#if defined(FUTILITY_PRUNING) || defined(LATE_MOVE_PRUNING) || defined(SEE_QUIET_MOVE_PRUNING)
			if (!(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) { // Quiet move
				#ifdef FUTILITY_PRUNING
				if (Depth <= 7 && (StaticScore + FutilityMargin(Depth)) <= Alpha) { // Hakkapeliitta
					BestScore = MAX(BestScore, StaticScore + FutilityMargin(Depth));

					UnmakeMove(Board);

					continue; // Next move
				}
				#endif // FUTILITY_PRUNING

				#ifdef LATE_MOVE_PRUNING
				if (Depth <= 6 && MoveNumber >= LateMovePruningTable[Depth]) { // Hakkapeliitta
					UnmakeMove(Board);

					continue; // Next move
				}
				#endif // LATE_MOVE_PRUNING

				#ifdef SEE_QUIET_MOVE_PRUNING
				if (Depth <= 3) { // Hakkapeliitta
					UnmakeMove(Board);

					if (CaptureSEE(Board, MOVE_FROM(MoveList[MoveNumber].Move), MOVE_TO(MoveList[MoveNumber].Move), MOVE_PROMOTE_PIECE(MoveList[MoveNumber].Move), MoveList[MoveNumber].Type) < 0) { // Bad quiet move
						continue; // Next move
					}

					MakeMove(Board, MoveList[MoveNumber]);
				}
				#endif // SEE_QUIET_MOVE_PRUNING
			}
			#endif // FUTILITY_PRUNING || LATE_MOVE_PRUNING || SEE_QUIET_MOVE_PRUNING
		} // if
		#endif // FUTILITY_PRUNING || LATE_MOVE_PRUNING || SEE_QUIET_MOVE_PRUNING || SEE_CAPTURE_MOVE_PRUNING

		NewDepth = Depth - 1 + Extension;

		#ifdef NEGA_SCOUT
		if (IsPrincipal && LegalMoveCount == 1) { // First move
		#endif // NEGA_SCOUT
			// Search with full window
			TempBestMoves[0] = { 0, 0, 0 };

			Score = -Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, TRUE, 0);
		#ifdef NEGA_SCOUT
		}
		else { // Other moves
			#ifdef LATE_MOVE_REDUCTION
			if (
				!Extension
				&& !InCheck
				&& !GiveCheck
				&& !(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))
				#ifdef HASH_MOVE
				&& MoveList[MoveNumber].Move != HashMove
				#endif // HASH_MOVE
				&& Depth >= 5
			) {
				LateMoveReduction = LateMoveReductionTable[MIN(Depth, 63)][MIN(MoveNumber, 63)]; // Hakkapeliitta

				if (IsPrincipal) {
					--LateMoveReduction;
				}
			}
			else {
				LateMoveReduction = 0;
			}

			// Zero window search for reduced depth
			TempBestMoves[0] = { 0, 0, 0 };

			Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth - LateMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, TRUE, 0);

			if (LateMoveReduction > 0 && Score > Alpha) {
				// Zero window search
				TempBestMoves[0] = { 0, 0, 0 };

				Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, TRUE, 0);
			}
			#else
			// Zero window search
			TempBestMoves[0] = { 0, 0, 0 };

			Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, TRUE, 0);
			#endif // LATE_MOVE_REDUCTION

			if (IsPrincipal && Score > Alpha && (Ply == 0 || Score < Beta)) {
				// Search with full window
				TempBestMoves[0] = { 0, 0, 0 };

				Score = -Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, TRUE, 0);
			}
		}
		#endif // NEGA_SCOUT

		UnmakeMove(Board);

		if (StopSearch) {
			return 0;
		}

		if (Score > BestScore) {
			BestScore = Score;

			if (BestScore > Alpha) {
				BestMove = MoveList[MoveNumber];

				if (IsPrincipal) {
					if (Ply == 0) { // Root node
						if (BestMove.Move == BestMoves[0].Move) { // Move not changed
							if (TimeStep > 0) {
								--TimeStep;
							}
						}
						else { // Move changed
							TimeStep = MAX_TIME_STEPS - 1;
						}
					}

					SaveBestMoves(BestMoves, BestMove, TempBestMoves);
				}

				if (IsPrincipal && BestScore < Beta) {
					Alpha = BestScore;
				}
				#ifdef ALPHA_BETA_PRUNING
				else { // !IsPrincipal || BestScore >= Beta
					#ifdef DEBUG_STATISTIC
					++Board->CutoffCount;
					#endif // DEBUG_STATISTIC

					#if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
					if (!(BestMove.Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) { // Not capture/promote move
						#ifdef MOVES_SORT_HEURISTIC
						UpdateHeuristic(Board, BestMove.Move, BONUS(Depth));

						for (int QuietMoveNumber = 0; QuietMoveNumber < QuietMoveCount; ++QuietMoveNumber) {
							UpdateHeuristic(Board, QuietMoveList[QuietMoveNumber], -BONUS(Depth));
						}
						#endif // MOVES_SORT_HEURISTIC

						#ifdef KILLER_MOVE
						UpdateKiller(Board, BestMove.Move, Ply);
						#endif // KILLER_MOVE
					}
					#endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

					break;
				}
				#endif // ALPHA_BETA_PRUNING
			} // if
		} // if

		if (!(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) { // Not capture/promote move
			QuietMoveList[QuietMoveCount++] = MoveList[MoveNumber].Move;
		}
	} // for

	if (LegalMoveCount == 0) { // No legal move (checkmate or stalemate)
		if (SkipMove) {
			BestScore = Alpha;
		}
		else if (InCheck) { // Checkmate
			BestScore = -INF + Ply;
		}
		else { // Stalemate
			BestScore = 0;
		}
	}

	#if defined(HASH_SCORE) || defined(HASH_MOVE)
	if (!SkipMove) {
		if (BestScore >= Beta) {
			HashFlag = HASH_BETA;
		}
		else if (IsPrincipal && BestMove.Move) {
			HashFlag = HASH_EXACT;
		}
		else {
			HashFlag = HASH_ALPHA;
		}

		SaveHash(Board->Hash, Depth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);
	}
	#endif // HASH_SCORE || HASH_MOVE

	return BestScore;
}

#endif // !ABDADA