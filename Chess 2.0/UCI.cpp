// UCI.cpp

#include "stdafx.h"

#include "UCI.h"

#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "Move.h"
#include "Types.h"
#include "Utils.h"

void UCI(void)
{
	char Buf[4096];
	char* Part;

	int File;
	int Rank;

	int From;
	int To;
	int PromotePiece;

	int Move;

	BOOL MoveFound;
	BOOL MoveInCheck;

	int GenMoveCount;
	MoveItem MoveList[MAX_GEN_MOVES];

	int HashSize;
	int Threads;

	U64 WTime;
	U64 BTime;

	U64 WInc;
	U64 BInc;

	int MovesToGo;

	int Mate;

	U64 ReduceTime;

	double Ratio;

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	printf("id name %s %s\n", NAME, VERSION);
	printf("id author %s\n", AUTHOR);

	printf("option name Hash type spin default %d min %d max %d\n", DEFAULT_HASH_TABLE_SIZE, 1, MAX_HASH_TABLE_SIZE);
	printf("option name Threads type spin default %d min %d max %d\n", DEFAULT_THREADS, 1, MaxThreads);

	SetFen(&CurrentBoard, StartFen);

	printf("uciok\n");

	while (TRUE) {
		fgets(Buf, sizeof(Buf), stdin);

		Part = Buf;

		if (!strncmp(Part, "isready", 7)) {
			printf("readyok\n");
		}
		else if (!strncmp(Part, "ucinewgame", 10)) {
			SetFen(&CurrentBoard, StartFen);

			ClearHash();
		}
		else if (!strncmp(Part, "setoption name Hash value", 25)) {
			Part += 26;

			HashSize = atoi(Part);

			HashSize = (HashSize >= 1 && HashSize <= MAX_HASH_TABLE_SIZE) ? HashSize : DEFAULT_HASH_TABLE_SIZE;

			InitHashTable(HashSize);
			ClearHash();
		}
		else if (!strncmp(Part, "setoption name Threads value", 28)) {
			Part += 29;

			Threads = atoi(Part);

			Threads = (Threads >= 1 && Threads <= MaxThreads) ? Threads : DEFAULT_THREADS;

			omp_set_num_threads(Threads);
		}
		else if (!strncmp(Part, "position", 8)) {
			Part += 9;

			if (!strncmp(Part, "startpos", 8)) {
				Part += 9;

				SetFen(&CurrentBoard, StartFen);
			}
			else if (!strncmp(Part, "fen", 3)) {
				Part += 4;

				Part += SetFen(&CurrentBoard, Part);
			}

			if (!strncmp(Part, "moves", 5)) {
				Part += 6;

				while (*Part != '\r' && *Part != '\n' && *Part != '\0') {
					// Move (e2e4, e7e8q)

					File = Part[0] - 'a';
					Rank = 7 - (Part[1] - '1');

					From = SQUARE(Rank, File);

					File = Part[2] - 'a';
					Rank = 7 - (Part[3] - '1');

					To = SQUARE(Rank, File);

					if (Part[4] == 'Q' || Part[4] == 'q') {
						PromotePiece = QUEEN;

						Part += 5;
					}
					else if (Part[4] == 'R' || Part[4] == 'r') {
						PromotePiece = ROOK;

						Part += 5;
					}
					else if (Part[4] == 'B' || Part[4] == 'b') {
						PromotePiece = BISHOP;

						Part += 5;
					}
					else if (Part[4] == 'N' || Part[4] == 'n') {
						PromotePiece = KNIGHT;

						Part += 5;
					}
					else {
						PromotePiece = 0;

						Part += 4;
					}

					Move = MOVE_CREATE(From, To, PromotePiece);

					MoveFound = FALSE;
					MoveInCheck = FALSE;

					GenMoveCount = 0;
					GenerateAllMoves(&CurrentBoard, MoveList, &GenMoveCount);

					for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
						if (MoveList[MoveNumber].Move == Move) {
							MoveFound = TRUE;

							MakeMove(&CurrentBoard, MoveList[MoveNumber]);

							if (IsInCheck(&CurrentBoard, CHANGE_COLOR(CurrentBoard.CurrentColor))) {
								MoveInCheck = TRUE;
							}

							break; // for
						}
					}

					if (!MoveFound || MoveInCheck) { // No move found or check
						printf("info string illegal move\n");

						break; // while (moves)
					}

					if (*Part == ' ') {
						++Part; // Space
					}
				} // while
			} // if
		}
		else if (!strncmp(Part, "go", 2)) {
			Part += 3;

			WTime = 0ULL;
			BTime = 0ULL;

			WInc = 0ULL;
			BInc = 0ULL;

			MovesToGo = 0;

			MaxDepth = 0;

			MaxTime = 0ULL;

			TimeForMove = 0ULL;

			memset(TargetTime, 0, sizeof(TargetTime));

			while (*Part != '\r' && *Part != '\n' && *Part != '\0') {
				if (!strncmp(Part, "wtime", 5)) {
					Part += 6;

					WTime = (U64)atoi(Part);
				}
				else if (!strncmp(Part, "btime", 5)) {
					Part += 6;

					BTime = (U64)atoi(Part);
				}
				else if (!strncmp(Part, "winc", 4)) {
					Part += 5;

					WInc = (U64)atoi(Part);
				}
				else if (!strncmp(Part, "binc", 4)) {
					Part += 5;

					BInc = (U64)atoi(Part);
				}
				else if (!strncmp(Part, "movestogo", 9)) {
					Part += 10;

					MovesToGo = atoi(Part);
				}
				else if (!strncmp(Part, "depth", 5)) {
					Part += 6;

					MaxDepth = atoi(Part);
				}
				else if (!strncmp(Part, "mate", 4)) {
					Part += 5;

					Mate = atoi(Part);

					MaxDepth = Mate * 2 - 1;
				}
				else if (!strncmp(Part, "movetime", 8)) {
					Part += 9;

					MaxTime = (U64)atoi(Part);
				}
				else if (!strncmp(Part, "infinite", 8)) {
					Part += 8;

					MaxTime = 0ULL;
				}

				while (*Part != ' ' && *Part != '\r' && *Part != '\n' && *Part != '\0') {
					++Part;
				}

				if (*Part == ' ') {
					++Part; // Space
				}
			} // while

			if (MovesToGo < 1 || MovesToGo > MAX_MOVES_TO_GO) {
				MovesToGo = MAX_MOVES_TO_GO;
			}

			if (MaxDepth < 1 || MaxDepth > MAX_PLY) {
				MaxDepth = MAX_PLY;
			}

			if (MaxTime == 0ULL) {
				if (CurrentBoard.CurrentColor == WHITE && WTime > 0ULL) {
					ReduceTime = MIN(WTime * (U64)REDUCE_TIME_PERCENT / 100ULL, (U64)MAX_REDUCE_TIME) + (U64)REDUCE_TIME;

					MaxTime = MAX(WTime - ReduceTime, 1ULL);

					TimeForMove = (MaxTime / (U64)MovesToGo) + WInc;

					for (int Step = 0; Step < MAX_TIME_STEPS; ++Step) {
						Ratio = MIN_TIME_RATIO + Step * (MAX_TIME_RATIO - MIN_TIME_RATIO) / (MAX_TIME_STEPS - 1);

						TargetTime[Step] = MIN((U64)(Ratio * (double)TimeForMove), MaxTime);
					}

					MaxTime = MIN(((MaxTime / (U64)MIN(MovesToGo, MAX_TIME_MOVES_TO_GO)) + WInc), MaxTime);
				}
				else if (CurrentBoard.CurrentColor == BLACK && BTime > 0ULL) {
					ReduceTime = MIN(BTime * (U64)REDUCE_TIME_PERCENT / 100ULL, (U64)MAX_REDUCE_TIME) + (U64)REDUCE_TIME;

					MaxTime = MAX(BTime - ReduceTime, 1ULL);

					TimeForMove = (MaxTime / (U64)MovesToGo) + BInc;

					for (int Step = 0; Step < MAX_TIME_STEPS; ++Step) {
						Ratio = MIN_TIME_RATIO + Step * (MAX_TIME_RATIO - MIN_TIME_RATIO) / (MAX_TIME_STEPS - 1);

						TargetTime[Step] = MIN((U64)(Ratio * (double)TimeForMove), MaxTime);
					}

					MaxTime = MIN(((MaxTime / (U64)MIN(MovesToGo, MAX_TIME_MOVES_TO_GO)) + BInc), MaxTime);
				}
				else {
					MaxTime = (U64)MAX_TIME * 1000ULL - (U64)REDUCE_TIME;

					TimeForMove = 0ULL;
				}
			}
			else { // MaxTime > 0ULL
				MaxTime = MAX(MaxTime - (U64)REDUCE_TIME, 1ULL);

				TimeForMove = 0ULL;
			}

			_beginthread(ComputerMoveThread, 0, NULL);
		}
		else if (!strncmp(Part, "stop", 4)) {
			StopSearch = TRUE;
		}
		else if (!strncmp(Part, "quit", 4)) {
			StopSearch = TRUE;

			return;
		}
		else {
			printf("info string unknown command\n");
		}
	} // while
}