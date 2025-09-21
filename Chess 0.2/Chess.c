#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
	*************************** (Rank)
	* a8 b8 c8 d8 e8 f8 g8 h8 *    0
	* a7 b7 c7 d7 e7 f7 g7 h7 *    1
	* a6 b6 c6 d6 e6 f6 g6 h6 *    2
	* a5 b5 c5 d5 e5 f5 g5 h5 *    3
	* a4 b4 c4 d4 e4 f4 g4 h4 *    4
	* a3 b3 c3 d3 e3 f3 g3 h3 *    5
	* a2 b2 c2 d2 e2 f2 g2 h2 *    6
	* a1 b1 c1 d1 e1 f1 g1 h1 *    7
	***************************
	   0  1  2  3  4  5  6  7 (File)

	*************************** (Rank)
	*  0  1  2  3  4  5  6  7 *    0
	*  8  9 10 11 12 13 14 15 *    1
	* 16 17 18 19 20 21 22 23 *    2
	* 24 25 26 27 28 29 30 31 *    3
	* 32 33 34 35 36 37 38 39 *    4
	* 40 41 42 43 44 45 46 47 *    5
	* 48 49 50 51 52 53 54 55 *    6
	* 56 57 58 59 60 61 62 63 *    7
	***************************
	   0  1  2  3  4  5  6  7 (File)
*/

#define BOOL int

#define FALSE 0
#define TRUE  1

#define INF 10000

#define EMPTY	 0

#define WHITE	-1
#define BLACK	 1

#define PAWN	1
#define KNIGHT	2
#define BISHOP	3
#define ROOK	4
#define QUEEN	5
#define KING	6

#define PAWN_SCORE		100
#define KNIGHT_SCORE	320
#define BISHOP_SCORE	330
#define ROOK_SCORE		500
#define QUEEN_SCORE		900

#define MOVE_QUIET			 0

#define MOVE_PAWN			 1
#define MOVE_PAWN_2			 2
#define MOVE_PAWN_PASSANT	 4
#define MOVE_PAWN_PROMOTE	 8

#define MOVE_CAPTURE		16

#define MOVE_CASTLE_KING	32
#define MOVE_CASTLE_QUEEN	64

#define MAX_PLY		128
#define MAX_TIME	86400

//#define RAZORING
//#define FUTILITYPRUNING
//#define NULLMOVE

#define CASTLE_WHITE_KING		1 // White О-О
#define CASTLE_WHITE_QUEEN	    2 // White О-О-О
#define CASTLE_BLACK_KING	    4 // Black О-О
#define CASTLE_BLACK_QUEEN		8 // Black О-О-О

#define FILE(i) (i & 7)
#define RANK(i) (i >> 3)

#define SORT_PV_MOVE_VALUE				(1 << 30)
#define SORT_PAWN_PROMOTE_MOVE_BONUS	(1 << 29)
#define SORT_CAPTURE_MOVE_BONUS			(1 << 28)

typedef unsigned long long U64;

typedef struct {
	int Type;

	int From;
	int PieceFrom;
	int PieceFromIndex;

	int To;
	int PieceTo;
	int PieceToIndex;

	int PassantIndex;
	int EatPawnIndex;

	int CastleFlags;
	int FiftyMove;

	U64 BoardHash;
} HistoryItem;

typedef struct {
	int Move;
	int Type;
	int PawnToPiece;
	U64 DeltaScore;
} MoveItem;

const int Board120[120] = { // Board 12x10
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7, -1,
	-1,  8,  9, 10, 11, 12, 13, 14, 15, -1,
	-1, 16, 17, 18, 19, 20, 21, 22, 23, -1,
	-1, 24, 25, 26, 27, 28, 29, 30, 31, -1,
	-1, 32, 33, 34, 35, 36, 37, 38, 39, -1,
	-1, 40, 41, 42, 43, 44, 45, 46, 47, -1,
	-1, 48, 49, 50, 51, 52, 53, 54, 55, -1,
	-1, 56, 57, 58, 59, 60, 61, 62, 63, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

const int Board64[64] = { // Convert index from board 8x8 to board 12x10
	21, 22, 23, 24, 25, 26, 27, 28,
	31, 32, 33, 34, 35, 36, 37, 38,
	41, 42, 43, 44, 45, 46, 47, 48,
	51, 52, 53, 54, 55, 56, 57, 58,
	61, 62, 63, 64, 65, 66, 67, 68,
	71, 72, 73, 74, 75, 76, 77, 78,
	81, 82, 83, 84, 85, 86, 87, 88,
	91, 92, 93, 94, 95, 96, 97, 98
};

const char BoardName[][64] = {
	"a8",	"b8",	"c8",	"d8",	"e8",	"f8",	"g8",	"h8",
	"a7",	"b7",	"c7",	"d7",	"e7",	"f7",	"g7",	"h7",
	"a6",	"b6",	"c6",	"d6",	"e6",	"f6",	"g6",	"h6",
	"a5",	"b5",	"c5",	"d5",	"e5",	"f5",	"g5",	"h5",
	"a4",	"b4",	"c4",	"d4",	"e4",	"f4",	"g4",	"h4",
	"a3",	"b3",	"c3",	"d3",	"e3",	"f3",	"g3",	"h3",
	"a2",	"b2",	"c2",	"d2",	"e2",	"f2",	"g2",	"h2",
	"a1",	"b1",	"c1",	"d1",	"e1",	"f1",	"g1",	"h1"
};

const char PiecesCharWhite[7] = { ' ', 'P', 'N', 'B', 'R', 'Q', 'K' };
const char PiecesCharBlack[7] = { ' ', 'p', 'n', 'b', 'r', 'q', 'k' };

const int CastleMask[64] = {
	 7, 15, 15, 15,  3, 15, 15, 11,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	13, 15, 15, 15, 12, 15, 15, 14
};

const int MoveDelta[7][8] = {
	{   0,	  0,	  0,	 0,		 0,		 0,		 0,		 0 },	// (not used)
	{   0,	  0,	  0,	 0,		 0,		 0,		 0,		 0 },	// Pawn (not used)
	{ -21,	-19,	-12,	-8,		 8,		12,		19,		21 },	// Knight
	{ -11,	 -9,	  9,	11,		 0,		 0,		 0,		 0 },	// Bishop
	{ -10,	 -1,	  1,	10,		 0,		 0,		 0,		 0 },	// Rook
	{ -11,	-10,	 -9,	-1,		 1,		 9,		10,		11 },	// Queen
	{ -11,	-10,	 -9,	-1,		 1,		 9,		10,		11 }	// King
};

const int Pieces[6] = { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN };

int BoardPiece[64];
int BoardColor[64];

int WhitePiecePosition[16];
int BlackPiecePosition[16];

int CurrentColor;
int PassantIndex;
int CastleFlags;
U64 BoardHash;
int HalfMoveNumber;
int FiftyMove;

U64 Nodes;

BOOL FollowPV;

MoveItem BestMovesMain[MAX_PLY];

MoveItem BestMoveMain;
int BestEvaluationMain;

U64 TimeStart;
U64 TimeStop;
U64 TotalTime;

BOOL TimeUp;

U64 PieceHash[2][6][64];
U64 ColorHash;
U64 PassantHash[64];

HistoryItem HistoryTable[1024];

U64 HistoryHeuristics[2][6][64];

int PawnSquareScore[64] = {
	 0,  0,   0,   0,   0,   0,  0,  0,
	50, 50,  50,  50,  50,  50, 50, 50,
	10, 10,  20,  30,  30,  20, 10, 10,
	 5,  5,  10,  25,  25,  10,  5,  5,
	 0,  0,   0,  20,  20,   0,  0,  0,
	 5, -5, -10,   0,   0, -10, -5,  5,
	 5, 10,  10, -20, -20,  10, 10,  5,
	 0,  0,   0,   0,   0,   0,  0,  0
};

int KnightSquareScore[64] = {
	-50, -40, -30, -30, -30, -30, -40, -50,
	-40, -20,   0,   0,   0,   0, -20, -40,
	-30,   0,  10,  15,  15,  10,   0, -30,
	-30,   5,  15,  20,  20,  15,   5, -30,
	-30,   0,  15,  20,  20,  15,   0, -30,
	-30,   5,  10,  15,  15,  10,   5, -30,
	-40, -20,   0,   5,   5,   0, -20, -40,
	-50, -40, -30, -30, -30, -30, -40, -50
};

int BishopSquareScore[64] = {
	-20, -10, -10, -10, -10, -10, -10, -20,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   5,   5,  10,  10,   5,   5, -10,
	-10,   0,  10,  10,  10,  10,   0, -10,
	-10,  10,  10,  10,  10,  10,  10, -10,
	-10,   5,   0,   0,   0,   0,   5, -10,
	-20, -10, -10, -10, -10, -10, -10, -20
};

int RookSquareScore[64] = {
	 0,  0,  0,  0,  0,  0,  0,  0,
	 5, 10, 10, 10, 10, 10, 10,  5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	 0,  0,  0,  5,  5,  0,  0,  0
};

int QueenSquareScore[64] = {
	-20, -10, -10, -5, -5, -10, -10, -20,
	-10,   0,   0,  0,  0,   0,   0, -10,
	-10,   0,   5,  5,  5,   5,   0, -10,
	 -5,   0,   5,  5,  5,   5,   0,  -5,
	  0,   0,   5,  5,  5,   5,   0,  -5,
	-10,   5,   5,  5,  5,   5,   0, -10,
	-10,   0,   5,  0,  0,   0,   0, -10,
	-20, -10, -10, -5, -5, -10, -10, -20
};

int KingSquareScoreOpening[64] = {
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-20, -30, -30, -40, -40, -30, -30, -20,
	-10, -20, -20, -20, -20, -20, -20, -10,
	 20,  20,   0,   0,   0,   0,  20,  20,
	 20,  30,  10,   0,   0,  10,  30,  20
};

int KingSquareScoreEnding[64] = {
	-50, -40, -30, -20, -20, -30, -40, -50,
	-30, -20, -10,   0,   0, -10, -20, -30,
	-30, -10,  20,  30,  30,  20, -10, -30,
	-30, -10,  30,  40,  40,  30, -10, -30,
	-30, -10,  30,  40,  40,  30, -10, -30,
	-30, -10,  20,  30,  30,  20, -10, -30,
	-30, -30,   0,   0,   0,   0, -30, -30,
	-50, -30, -30, -30, -30, -30, -30, -50
};

void GenerateAllMoves(MoveItem *MoveList, int *GenMoveCount);
void SaveGame(void);

U64 RandHash(void)
{
  return ((U64)rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 31) ^ ((U64)rand() << 47) ^ ((U64)rand() << 59));
}

void InitHash(void)
{
	srand((unsigned int)0);

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 64; k++) {
				PieceHash[i][j][k] = RandHash();
			}
		}
	}

	ColorHash = RandHash();

	for (int i = 0; i < 64; i++) {
		PassantHash[i] = RandHash();
	}
}

U64 PositionHash(void)
{
	U64 Hash = 0;

	for (int i = 0; i < 16; i++) {
		if (WhitePiecePosition[i] == -1) {
			continue;
		}

		Hash ^= PieceHash[0][BoardPiece[WhitePiecePosition[i]] - 1][WhitePiecePosition[i]];
	}

	for (int i = 0; i < 16; i++) {
		if (BlackPiecePosition[i] == -1) {
			continue;
		}

		Hash ^= PieceHash[1][BoardPiece[BlackPiecePosition[i]] - 1][BlackPiecePosition[i]];
	}

	if (CurrentColor == BLACK) {
		Hash ^= ColorHash;
	}

	if (PassantIndex != -1) {
		Hash ^= PassantHash[PassantIndex];
	}

	return Hash;
}

void InitEvaluation(void)
{
	for (int Square = 0; Square < 64; Square++) {
		PawnSquareScore[Square] += PAWN_SCORE;
		KnightSquareScore[Square] += KNIGHT_SCORE;
		BishopSquareScore[Square] += BISHOP_SCORE;
		RookSquareScore[Square] += ROOK_SCORE;
		QueenSquareScore[Square] += QUEEN_SCORE;
	}
}

BOOL IsEndGame(void)
{
	int PieceCount[2][7] = {
		{ 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0 }
	};

	for (int i = 0; i < 16; i++) {
		if (WhitePiecePosition[i] == -1) {
			continue;
		}

		PieceCount[0][BoardPiece[WhitePiecePosition[i]]]++;
	}

	for (int i = 0; i < 16; i++) {
		if (BlackPiecePosition[i] == -1) {
			continue;
		}

		PieceCount[1][BoardPiece[BlackPiecePosition[i]]]++;
	}

	if (PieceCount[0][QUEEN] == 0 && PieceCount[1][QUEEN] == 0) {
		return TRUE;
	}

	if (PieceCount[0][QUEEN] && PieceCount[0][ROOK] == 0 && (PieceCount[0][KNIGHT] + PieceCount[0][BISHOP]) <= 1) {
		return TRUE;
	}

	if (PieceCount[1][QUEEN] && PieceCount[1][ROOK] == 0 && (PieceCount[1][KNIGHT] + PieceCount[1][BISHOP]) <= 1) {
		return TRUE;
	}

	return FALSE;
}

int PositionEvaluation(void)
{
	int Score = 0;
	int Square;

	for (int i = 0; i < 16; i++) {
		if (WhitePiecePosition[i] == -1) {
			continue;
		}

		Square = WhitePiecePosition[i];

		switch (BoardPiece[Square]) {
			case PAWN:
				Score += PawnSquareScore[Square];
				break;

			case KNIGHT:
				Score += KnightSquareScore[Square];
				break;

			case BISHOP:
				Score += BishopSquareScore[Square];
				break;

			case ROOK:
				Score += RookSquareScore[Square];
				break;

			case QUEEN:
				Score += QueenSquareScore[Square];
				break;

			case KING:
				if (IsEndGame()) { // End game
					Score += KingSquareScoreEnding[Square];
				}
				else { // Open/Middle game
					Score += KingSquareScoreOpening[Square];
				}
		} // switch
	} // for

	for (int i = 0; i < 16; i++) {
		if (BlackPiecePosition[i] == -1) {
			continue;
		}

		Square = BlackPiecePosition[i];

		switch (BoardPiece[Square]) {
			case PAWN:
				Score -= PawnSquareScore[Square ^ 56];
				break;

			case KNIGHT:
				Score -= KnightSquareScore[Square ^ 56];
				break;

			case BISHOP:
				Score -= BishopSquareScore[Square ^ 56];
				break;

			case ROOK:
				Score -= RookSquareScore[Square ^ 56];
				break;

			case QUEEN:
				Score -= QueenSquareScore[Square ^ 56];
				break;

			case KING:
				if (IsEndGame()) { // End game
					Score -= KingSquareScoreEnding[Square ^ 56];
				}
				else { // Open/Middle game
					Score -= KingSquareScoreOpening[Square ^ 56];
				}
		} // switch
	} // for

	return Score;
}

int GetEvaluation(void)
{
	int Evaluation = PositionEvaluation();

	if (CurrentColor == BLACK) {
		Evaluation = -Evaluation;
	}

	return Evaluation;
}

int GetPiecePosition(const int *PiecePosition, const int Index)
{
	for (int i = 0; i < 16; i++) {
		if (PiecePosition[i] == Index) {
			return i;
		}
	}

	return -1;
}

void SetPiecePosition(int *PiecePosition, const int Index, const int Value)
{
	for (int i = 0; i < 16; i++) {
		if (PiecePosition[i] == Index) {
			PiecePosition[i] = Value;

			return;
		}
	}
}

void MakeMove(const MoveItem Move)
{
	HistoryItem *Info = &HistoryTable[HalfMoveNumber++];

	int From = Move.Move >> 6;
	int To = Move.Move & 63;

	Info->Type = Move.Type;

	Info->From = From;
	Info->PieceFrom = BoardPiece[From];

	Info->To = To;
	Info->PieceTo = BoardPiece[To];

	Info->PassantIndex = PassantIndex;

	Info->CastleFlags = CastleFlags;
	Info->FiftyMove = FiftyMove;
	Info->BoardHash = BoardHash;

	if (Info->PassantIndex != -1) {
		BoardHash ^= PassantHash[Info->PassantIndex];

		PassantIndex = -1;
	}

	if (CurrentColor == WHITE) {
		if (Move.Type & MOVE_PAWN_2) {
			PassantIndex = From - 8;

			BoardHash ^= PassantHash[PassantIndex];
		}

		if (Move.Type & MOVE_CASTLE_KING) { // White O-O
			BoardPiece[61] = ROOK;
			BoardColor[61] = WHITE;
			SetPiecePosition(WhitePiecePosition, 63, 61);

			BoardHash ^= PieceHash[0][ROOK - 1][63];
			BoardHash ^= PieceHash[0][ROOK - 1][61];

			BoardPiece[63] = EMPTY;
			BoardColor[63] = EMPTY;
		}

		if (Move.Type & MOVE_CASTLE_QUEEN) { // White O-O-O
			BoardPiece[59] = ROOK;
			BoardColor[59] = WHITE;
			SetPiecePosition(WhitePiecePosition, 56, 59);

			BoardHash ^= PieceHash[0][ROOK - 1][56];
			BoardHash ^= PieceHash[0][ROOK - 1][59];

			BoardPiece[56] = EMPTY;
			BoardColor[56] = EMPTY;
		}

		CastleFlags &= CastleMask[From] & CastleMask[To];

		Info->PieceFromIndex = GetPiecePosition(WhitePiecePosition, From);

		BoardHash ^= PieceHash[0][Info->PieceFrom - 1][From];

		if (Move.Type & MOVE_PAWN_PASSANT) {
			Info->EatPawnIndex = To + 8;
			Info->PieceTo = PAWN;
			Info->PieceToIndex = GetPiecePosition(BlackPiecePosition, Info->EatPawnIndex);

			BoardHash ^= PieceHash[1][BoardPiece[Info->EatPawnIndex] - 1][Info->EatPawnIndex];

			BoardPiece[Info->EatPawnIndex] = EMPTY;
			BoardColor[Info->EatPawnIndex] = EMPTY;
			BlackPiecePosition[Info->PieceToIndex] = -1;

			BoardHash ^= PassantHash[Info->PassantIndex];

			PassantIndex = -1;
		}
		else if (Move.Type & MOVE_CAPTURE) {
			Info->PieceToIndex = GetPiecePosition(BlackPiecePosition, To);

			BoardHash ^= PieceHash[1][Info->PieceTo - 1][To];

			BlackPiecePosition[Info->PieceToIndex] = -1;
		}

		BoardPiece[To] = BoardPiece[From];
		BoardColor[To] = BoardColor[From];
		WhitePiecePosition[Info->PieceFromIndex] = To;

		if (Move.Type & MOVE_PAWN_PROMOTE) {
			BoardPiece[To] = Move.PawnToPiece;
		}

		BoardPiece[From] = EMPTY;
		BoardColor[From] = EMPTY;

		BoardHash ^= PieceHash[0][BoardPiece[To] - 1][To];
	}
	else { // BLACK
		if (Move.Type & MOVE_PAWN_2) {
			PassantIndex = From + 8;

			BoardHash ^= PassantHash[PassantIndex];
		}

		if (Move.Type & MOVE_CASTLE_KING) { // Black O-O
			BoardPiece[5] = ROOK;
			BoardColor[5] = BLACK;
			SetPiecePosition(BlackPiecePosition, 7, 5);

			BoardHash ^= PieceHash[1][ROOK - 1][7];
			BoardHash ^= PieceHash[1][ROOK - 1][5];

			BoardPiece[7] = EMPTY;
			BoardColor[7] = EMPTY;
		}

		if (Move.Type & MOVE_CASTLE_QUEEN) { // Black O-O-O
			BoardPiece[3] = ROOK;
			BoardColor[3] = BLACK;
			SetPiecePosition(BlackPiecePosition, 0, 3);

			BoardHash ^= PieceHash[1][ROOK - 1][0];
			BoardHash ^= PieceHash[1][ROOK - 1][3];

			BoardPiece[0] = EMPTY;
			BoardColor[0] = EMPTY;
		}

		CastleFlags &= CastleMask[From] & CastleMask[To];

		Info->PieceFromIndex = GetPiecePosition(BlackPiecePosition, From);

		BoardHash ^= PieceHash[1][Info->PieceFrom - 1][From];

		if (Move.Type & MOVE_PAWN_PASSANT) {
			Info->EatPawnIndex = To - 8;
			Info->PieceTo = PAWN;
			Info->PieceToIndex = GetPiecePosition(WhitePiecePosition, Info->EatPawnIndex);

			BoardHash ^= PieceHash[0][BoardPiece[Info->EatPawnIndex] - 1][Info->EatPawnIndex];

			BoardPiece[Info->EatPawnIndex] = EMPTY;
			BoardColor[Info->EatPawnIndex] = EMPTY;
			WhitePiecePosition[Info->PieceToIndex] = -1;

			BoardHash ^= PassantHash[Info->PassantIndex];

			PassantIndex = -1;
		}
		else if (Move.Type & MOVE_CAPTURE) {
			Info->PieceToIndex = GetPiecePosition(WhitePiecePosition, To);

			BoardHash ^= PieceHash[0][Info->PieceTo - 1][To];

			WhitePiecePosition[Info->PieceToIndex] = -1;
		}

		BoardPiece[To] = BoardPiece[From];
		BoardColor[To] = BoardColor[From];
		BlackPiecePosition[Info->PieceFromIndex] = To;

		if (Move.Type & MOVE_PAWN_PROMOTE) {
			BoardPiece[To] = Move.PawnToPiece;
		}

		BoardPiece[From] = EMPTY;
		BoardColor[From] = EMPTY;

		BoardHash ^= PieceHash[1][BoardPiece[To] - 1][To];
	}

	if (Move.Type & (MOVE_CAPTURE | MOVE_PAWN | MOVE_PAWN_2)) {
		FiftyMove = 0;
	}
	else {
		FiftyMove++;
	}
}

void UnmakeMove(void)
{
	HistoryItem *Info = &HistoryTable[--HalfMoveNumber];

	BoardPiece[Info->From] = Info->PieceFrom;
	BoardColor[Info->From] = CurrentColor;

	if (CurrentColor == WHITE) {
		WhitePiecePosition[Info->PieceFromIndex] = Info->From;

		if (Info->Type & MOVE_PAWN_PASSANT) {
			BoardPiece[Info->To] = EMPTY;
			BoardColor[Info->To] = EMPTY;

			BoardPiece[Info->EatPawnIndex] = PAWN;
			BoardColor[Info->EatPawnIndex] = BLACK;
			BlackPiecePosition[Info->PieceToIndex] = Info->EatPawnIndex;
		}
		else if (Info->Type & MOVE_CAPTURE) {
			BoardPiece[Info->To] = Info->PieceTo;
			BoardColor[Info->To] = BLACK;
			BlackPiecePosition[Info->PieceToIndex] = Info->To;
		}
		else {
			BoardPiece[Info->To] = EMPTY;
			BoardColor[Info->To] = EMPTY;
		}

		if (Info->Type & MOVE_CASTLE_KING) { // White O-O
			BoardPiece[63] = ROOK;
			BoardColor[63] = WHITE;
			SetPiecePosition(WhitePiecePosition, 61, 63);

			BoardPiece[61] = EMPTY;
			BoardColor[61] = EMPTY;
		}

		if (Info->Type & MOVE_CASTLE_QUEEN) { // White O-O-O
			BoardPiece[56] = ROOK;
			BoardColor[56] = WHITE;
			SetPiecePosition(WhitePiecePosition, 59, 56);

			BoardPiece[59] = EMPTY;
			BoardColor[59] = EMPTY;
		}
	}
	else { // BLACK
		BlackPiecePosition[Info->PieceFromIndex] = Info->From;

		if (Info->Type & MOVE_PAWN_PASSANT) {
			BoardPiece[Info->To] = EMPTY;
			BoardColor[Info->To] = EMPTY;

			BoardPiece[Info->EatPawnIndex] = PAWN;
			BoardColor[Info->EatPawnIndex] = WHITE;
			WhitePiecePosition[Info->PieceToIndex] = Info->EatPawnIndex;
		}
		else if (Info->Type & MOVE_CAPTURE) {
			BoardPiece[Info->To] = Info->PieceTo;
			BoardColor[Info->To] = WHITE;
			WhitePiecePosition[Info->PieceToIndex] = Info->To;
		}
		else {
			BoardPiece[Info->To] = EMPTY;
			BoardColor[Info->To] = EMPTY;
		}

		if (Info->Type & MOVE_CASTLE_KING) { // Black O-O
			BoardPiece[7] = ROOK;
			BoardColor[7] = BLACK;
			SetPiecePosition(BlackPiecePosition, 5, 7);

			BoardPiece[5] = EMPTY;
			BoardColor[5] = EMPTY;
		}

		if (Info->Type & MOVE_CASTLE_QUEEN) { // Black O-O-O
			BoardPiece[0] = ROOK;
			BoardColor[0] = BLACK;
			SetPiecePosition(BlackPiecePosition, 3, 0);

			BoardPiece[3] = EMPTY;
			BoardColor[3] = EMPTY;
		}
	}

	PassantIndex = Info->PassantIndex;
	CastleFlags = Info->CastleFlags;
	FiftyMove = Info->FiftyMove;
	BoardHash = Info->BoardHash;
}

BOOL CheckField(const int Square)
{
	int To;

	// Pawn
	if (CurrentColor == WHITE) {
		if (
			(RANK(Square) > 1 && FILE(Square) != 0 && BoardPiece[Square - 9] == PAWN && BoardColor[Square - 9] == BLACK)
			|| (RANK(Square) > 1 && FILE(Square) != 7 && BoardPiece[Square - 7] == PAWN && BoardColor[Square - 7] == BLACK)
		) {
			return TRUE;
		}
	}
	else { // BLACK
		if (
			(RANK(Square) < 6 && FILE(Square) != 0 && BoardPiece[Square + 7] == PAWN && BoardColor[Square + 7] == WHITE)
			|| (RANK(Square) < 6 && FILE(Square) != 7 && BoardPiece[Square + 9] == PAWN && BoardColor[Square + 9] == WHITE)
		) {
			return TRUE;
		}
	}

	// Knight
	for (int i = 0; i < 8; i++) {
		To = Board120[Board64[Square] + MoveDelta[KNIGHT][i]];

		if (To == -1) {
			continue;
		}

		if (BoardColor[To] == EMPTY || BoardColor[To] == CurrentColor) {
			continue;
		}

		if (BoardPiece[To] == KNIGHT) {
			return TRUE;
		}
	} // for

	// Rook, Queen, King
	for (int i = 0; i < 4; i++) {
		To = Square;

		for (int j = 1; j < 8; j++) {
			To = Board120[Board64[To] + MoveDelta[ROOK][i]];

			if (To == -1) {
				break;
			}

			if (BoardColor[To] == EMPTY) {
				continue;
			}

			if (BoardColor[To] == CurrentColor) {
				break;
			}

			if (j == 1 && BoardPiece[To] == KING) {
				return TRUE;
			}

			if (BoardPiece[To] == ROOK || BoardPiece[To] == QUEEN) {
				return TRUE;
			}

			break;
		} // for
	} // for

	// Bishop, Queen, King
	for (int i = 0; i < 4; i++) {
		To = Square;

		for (int j = 1; j < 8; j++) {
			To = Board120[Board64[To] + MoveDelta[BISHOP][i]];

			if (To == -1) {
				break;
			}

			if (BoardColor[To] == EMPTY) {
				continue;
			}

			if (BoardColor[To] == CurrentColor) {
				break;
			}

			if (j == 1 && BoardPiece[To] == KING) {
				return TRUE;
			}

			if (BoardPiece[To] == BISHOP || BoardPiece[To] == QUEEN) {
				return TRUE;
			}

			break;
		} // for
	} // for

	return FALSE;
}

BOOL CheckKing(void)
{
	int KingPosition;

	if (CurrentColor == WHITE) { 
		KingPosition = WhitePiecePosition[0];
	}
	else { // BLACK
		KingPosition = BlackPiecePosition[0];
	}

	return CheckField(KingPosition);
}

void PrintBoard(void)
{
	printf("\n");

	printf("     a   b   c   d   e   f   g   h\n");

	for (int Square = 0; Square < 64; Square++) {
		if ((Square % 8) == 0) {
			printf("   +---+---+---+---+---+---+---+---+\n");
			printf(" %d |", (8 - Square / 8));
		}

		if (BoardColor[Square] == EMPTY) {
			printf("   |");
		}
		else if (BoardColor[Square] == WHITE) {
			printf(" %c |", PiecesCharWhite[BoardPiece[Square]]);
		}
		else { // BLACK
			printf(" %c |", PiecesCharBlack[BoardPiece[Square]]);
		}

		if (((Square + 1) % 8) == 0) {
			printf(" %d\n", (8 - Square / 8));
		}
	}

	printf("   +---+---+---+---+---+---+---+---+\n");
	printf("     a   b   c   d   e   f   g   h\n");

	printf("\n");

	printf("Static evaluation %.2f\n", GetEvaluation() / 100.0);
}

void PrintBestMoves(const int *Depth, const MoveItem *BestMoves, const int *BestEvaluation)
{
	if (*Depth == 1) {
		printf("\n");
	}

	printf("Depth %2d Evaluation %6.2f PV", *Depth, *BestEvaluation / 100.0);

	for (int i = 0; i < MAX_PLY && BestMoves[i].Move; i++) {
		printf(" %s%s%c", BoardName[BestMoves[i].Move >> 6], BoardName[BestMoves[i].Move & 63], PiecesCharBlack[BestMoves[i].PawnToPiece]);
	}

	printf("\n");
}

void SaveBestMoves(MoveItem *BestMoves, const MoveItem BestMove, const MoveItem *TempBestMoves)
{
	int i;

	BestMoves[0] = BestMove;

	for (i = 0; i < (MAX_PLY - 2) && TempBestMoves[i].Move; i++) {
		BestMoves[i + 1] = TempBestMoves[i];
	}

	BestMoves[i + 1] = (MoveItem){ 0, 0, 0, 0ULL };
}

void AddMove(MoveItem *MoveList, int *GenMoveCount, const int *From, const int *To, const int MoveType)
{
	if ((MoveType & MOVE_PAWN) && (RANK(*To) == 0 || RANK(*To) == 7)) {
		// Queen
		MoveList[*GenMoveCount].Move = (*From << 6) + *To;
		MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
		MoveList[*GenMoveCount].DeltaScore = SORT_PAWN_PROMOTE_MOVE_BONUS + QUEEN;
		MoveList[*GenMoveCount].PawnToPiece = QUEEN;

		(*GenMoveCount)++;

		// Rook
		MoveList[*GenMoveCount].Move = (*From << 6) + *To;
		MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
		MoveList[*GenMoveCount].DeltaScore = SORT_PAWN_PROMOTE_MOVE_BONUS + ROOK;
		MoveList[*GenMoveCount].PawnToPiece = ROOK;

		(*GenMoveCount)++;

		// Bishop
		MoveList[*GenMoveCount].Move = (*From << 6) + *To;
		MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
		MoveList[*GenMoveCount].DeltaScore = SORT_PAWN_PROMOTE_MOVE_BONUS + BISHOP;
		MoveList[*GenMoveCount].PawnToPiece = BISHOP;

		(*GenMoveCount)++;

		// Knight
		MoveList[*GenMoveCount].Move = (*From << 6) + *To;
		MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
		MoveList[*GenMoveCount].DeltaScore = SORT_PAWN_PROMOTE_MOVE_BONUS + KNIGHT;
		MoveList[*GenMoveCount].PawnToPiece = KNIGHT;

		(*GenMoveCount)++;
	}
	else {
		MoveList[*GenMoveCount].Move = (*From << 6) + *To;
		MoveList[*GenMoveCount].Type = MoveType;

		if (MoveType & MOVE_CAPTURE) {
			if (MoveType & MOVE_PAWN_PASSANT) {
				MoveList[*GenMoveCount].DeltaScore = SORT_CAPTURE_MOVE_BONUS + (PAWN << 3) - PAWN;
			}
			else {
				MoveList[*GenMoveCount].DeltaScore = SORT_CAPTURE_MOVE_BONUS + (BoardPiece[*To] << 3) - BoardPiece[*From];
			}
		}
		else {
			MoveList[*GenMoveCount].DeltaScore = HistoryHeuristics[CurrentColor == WHITE ? 0 : 1][BoardPiece[*From] - 1][*To];
		}

		MoveList[*GenMoveCount].PawnToPiece = 0;

		(*GenMoveCount)++;
	}
}

void GenerateAllMoves(MoveItem *MoveList, int *GenMoveCount)
{
	int From;
	int To;

	int *FromPiecePosition;

	if (CurrentColor == WHITE) {
		FromPiecePosition = WhitePiecePosition;
	}
	else { // BLACK
		FromPiecePosition = BlackPiecePosition;
	}

	for (int i = 0; i < 16; i++) {
		From = FromPiecePosition[i];

		if (From == -1) {
			continue;
		}

		if (BoardPiece[From] == PAWN) {
			if (CurrentColor == WHITE) {
				if (FILE(From) != 0) {
					To = From - 9;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == BLACK) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				if (FILE(From) != 7) {
					To = From - 7;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == BLACK) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				To = From - 8;

				if (BoardPiece[To] == EMPTY) {
					AddMove(MoveList, GenMoveCount, &From, &To, MOVE_PAWN);

					if (RANK(From) == 6) {
						To = From - 16;

						if (BoardPiece[To] == EMPTY) {
							AddMove(MoveList, GenMoveCount, &From, &To, MOVE_PAWN_2);
						}
					}
				}
			}
			else { // BLACK
				if (FILE(From) != 0) {
					To = From + 7;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == WHITE) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				if (FILE(From) != 7) {
					To = From + 9;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == WHITE) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				To = From + 8;

				if (BoardPiece[To] == EMPTY) {
					AddMove(MoveList, GenMoveCount, &From, &To, MOVE_PAWN);

					if (RANK(From) == 1) {
						To = From + 16;

						if (BoardPiece[To] == EMPTY) {
							AddMove(MoveList, GenMoveCount, &From, &To, MOVE_PAWN_2);
						}
					}
				}
			}
		}
		else { // Knight, Bishop, Rook, Queen, King
			for (int j = 0; j < 8; j++) {
				if (MoveDelta[BoardPiece[From]][j] == 0) {
					break;
				}

				To = From;

				while (TRUE) {
					To = Board120[Board64[To] + MoveDelta[BoardPiece[From]][j]];

					if (To == -1) {
						break;
					}

					if (BoardColor[To] == CurrentColor) {
						break;
					}

					if (BoardPiece[To]) {
						AddMove(MoveList, GenMoveCount, &From, &To, MOVE_CAPTURE);

						break;
					}

					AddMove(MoveList, GenMoveCount, &From, &To, MOVE_QUIET);

					if (BoardPiece[From] == KING || BoardPiece[From] == KNIGHT) {
						break;
					}
				} // while
			} // for
		}
	} // for

	if (CurrentColor == WHITE) {
		if (
			(CastleFlags & CASTLE_WHITE_KING)
			&& BoardPiece[61] == EMPTY && BoardPiece[62] == EMPTY
			&& !CheckKing() && !CheckField(61) && !CheckField(62)
		) { // White О-О
			From = 60;
			To = 62;

			AddMove(MoveList, GenMoveCount, &From, &To, MOVE_CASTLE_KING);
		}

		if (
			(CastleFlags & CASTLE_WHITE_QUEEN)
			&& BoardPiece[59] == EMPTY && BoardPiece[58] == EMPTY && BoardPiece[57] == EMPTY
			&& !CheckKing() && !CheckField(59) && !CheckField(58)
		) { // White О-О-О
			From = 60;
			To = 58;

			AddMove(MoveList, GenMoveCount, &From, &To, MOVE_CASTLE_QUEEN);
		}
	}
	else { // BLACK
		if (
			(CastleFlags & CASTLE_BLACK_KING)
			&& BoardPiece[5] == EMPTY && BoardPiece[6] == EMPTY
			&& !CheckKing() && !CheckField(5) && !CheckField(6)
		) { // Black О-О
			From = 4;
			To = 6;

			AddMove(MoveList, GenMoveCount, &From, &To, MOVE_CASTLE_KING);
		}

		if (
			(CastleFlags & CASTLE_BLACK_QUEEN)
			&& BoardPiece[3] == EMPTY && BoardPiece[2] == EMPTY && BoardPiece[1] == EMPTY
			&& !CheckKing() && !CheckField(3) && !CheckField(2)
		) { // Black О-О-О
			From = 4;
			To = 2;

			AddMove(MoveList, GenMoveCount, &From, &To, MOVE_CASTLE_QUEEN);
		}
	}
}

void GenerateCaptureMoves(MoveItem *MoveList, int *GenMoveCount)
{
	int From;
	int To;

	int *FromPiecePosition;

	if (CurrentColor == WHITE) {
		FromPiecePosition = WhitePiecePosition;
	}
	else { // BLACK
		FromPiecePosition = BlackPiecePosition;
	}

	for (int i = 0; i < 16; i++) {
		From = FromPiecePosition[i];

		if (From == -1) {
			continue;
		}

		if (BoardPiece[From] == PAWN) {
			if (CurrentColor == WHITE) {
				if (FILE(From) != 0) {
					To = From - 9;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == BLACK) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				if (FILE(From) != 7) {
					To = From - 7;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == BLACK) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				if (RANK(From) == 1) {
					To = From - 8;

					if (BoardPiece[To] == EMPTY) {
						AddMove(MoveList, GenMoveCount, &From, &To, MOVE_PAWN);
					}
				}
			}
			else { // BLACK
				if (FILE(From) != 0) {
					To = From + 7;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == WHITE) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				if (FILE(From) != 7) {
					To = From + 9;

					if (To == PassantIndex) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
					}

					if (BoardColor[To] == WHITE) {
						AddMove(MoveList, GenMoveCount, &From, &To, (MOVE_PAWN | MOVE_CAPTURE));
					}
				}

				if (RANK(From) == 6) {
					To = From + 8;

					if (BoardPiece[To] == EMPTY) {
						AddMove(MoveList, GenMoveCount, &From, &To, MOVE_PAWN);
					}
				}
			}
		}
		else { // Knight, Bishop, Rook, Queen, King
			for (int j = 0; j < 8; j++) {
				if (MoveDelta[BoardPiece[From]][j] == 0) {
					break;
				}

				To = From;

				while (TRUE) {
					To = Board120[Board64[To] + MoveDelta[BoardPiece[From]][j]];

					if (To == -1) {
						break;
					}

					if (BoardColor[To] == CurrentColor) {
						break;
					}

					if (BoardPiece[To]) {
						AddMove(MoveList, GenMoveCount, &From, &To, MOVE_CAPTURE);

						break;
					}

					if (BoardPiece[From] == KING || BoardPiece[From] == KNIGHT) {
						break;
					}
				} // while
			} // for
		}
	} // for
}

BOOL IsInsufficientMaterial(void)
{
	int AllCount = 0;

	int PawnCount = 0;
	int RookCount = 0;
	int QueenCount = 0;

	for (int i = 0; i < 16; i++) {
		if (WhitePiecePosition[i] == -1) {
			continue;
		}

		AllCount++;

		if (BoardPiece[WhitePiecePosition[i]] == PAWN) {
			PawnCount++;
		}
		else if (BoardPiece[WhitePiecePosition[i]] == ROOK) {
			RookCount++;
		}
		else if (BoardPiece[WhitePiecePosition[i]] == QUEEN) {
			QueenCount++;
		}
	}

	for (int i = 0; i < 16; i++) {
		if (BlackPiecePosition[i] == -1) {
			continue;
		}

		AllCount++;

		if (BoardPiece[BlackPiecePosition[i]] == PAWN) {
			PawnCount++;
		}
		else if (BoardPiece[BlackPiecePosition[i]] == ROOK) {
			RookCount++;
		}
		else if (BoardPiece[BlackPiecePosition[i]] == QUEEN) {
			QueenCount++;
		}
	}

	if (AllCount <= 3) { // KK or KxK
		if (PawnCount) { // KPK
			return FALSE;
		}

		if (RookCount) { // KRK
			return FALSE;
		}

		if (QueenCount) { // KQK
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

int PositionRepeat1(void)
{
	if (FiftyMove < 4) {
		return 0;
	}

	for (int i = HalfMoveNumber - 2; i >= (HalfMoveNumber - FiftyMove); i -= 2) {
		if (HistoryTable[i].BoardHash == BoardHash) { // Position repeated
			return 1;
		}
	}

	return 0;
}

int PositionRepeat2(void)
{
	int RepeatCount = 0;

	if (FiftyMove < 4) {
		return 0;
	}

	for (int i = HalfMoveNumber - 2; i >= (HalfMoveNumber - FiftyMove); i -= 2) {
		if (HistoryTable[i].BoardHash == BoardHash) { // Position repeated
			RepeatCount++;
		}
	}

	return RepeatCount;
}

BOOL NonPawnMaterial(void)
{
	if (CurrentColor == WHITE) {
		for (int i = 0; i < 16; i++) {
			if (WhitePiecePosition[i] == -1) {
				continue;
			}

			if (BoardPiece[WhitePiecePosition[i]] != PAWN && BoardPiece[WhitePiecePosition[i]] != KING) {
				return TRUE;
			}
		}
	}
	else { // BLACK
		for (int i = 0; i < 16; i++) {
			if (BlackPiecePosition[i] == -1) {
				continue;
			}

			if (BoardPiece[BlackPiecePosition[i]] != PAWN && BoardPiece[BlackPiecePosition[i]] != KING) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

int Quiesce(int Alpha, int Beta, const int Depth, const int Ply, MoveItem *BestMoves, const BOOL PV, const BOOL IsCheck)
{
	int Score;
	int CurrentEvaluation;
	MoveItem TempBestMoves[MAX_PLY];

	int GenMoveCount = 0;
	MoveItem MoveList[256];

	int BestMoveIndex;
	U64 BestMoveScore;
	MoveItem TempMoveItem;

	int LegalMoveCount = 0;

	if (
		(Nodes & 65535) == 0
		&& TimeStop <= (U64)time(NULL)
	) {
		TimeUp = TRUE;

		return 0;
	}

	if (TimeUp) {
		return 0;
	}

	if (IsInsufficientMaterial() || PositionRepeat1()) {
		return 0;
	}

	if (Ply >= MAX_PLY) {
		return GetEvaluation();
	}

	if (IsCheck) {
		GenerateAllMoves(MoveList, &GenMoveCount);
	}
	else {
		CurrentEvaluation = GetEvaluation();

		if (CurrentEvaluation >= Beta) {
			return Beta;
		}

		if (CurrentEvaluation > Alpha) {
			Alpha = CurrentEvaluation;
		}

		GenerateCaptureMoves(MoveList, &GenMoveCount);
	}

	if (FollowPV) {
		FollowPV = FALSE;

		for (int i = 0; i < GenMoveCount; i++) {
			if (MoveList[i].Move == BestMovesMain[Ply].Move && MoveList[i].PawnToPiece == BestMovesMain[Ply].PawnToPiece) {
//				printf("%d %s%s%c\n", Ply, BoardName[MoveList[i].Move >> 6], BoardName[MoveList[i].Move & 63], PiecesCharBlack[MoveList[i].PawnToPiece]);

				MoveList[i].DeltaScore = SORT_PV_MOVE_VALUE;

				FollowPV = TRUE;

				break;
			}
		}
	}

	for (int i = 0; i < GenMoveCount; i++) {
		BestMoveIndex = i;
		BestMoveScore = MoveList[i].DeltaScore;

		for (int j = i + 1; j < GenMoveCount; j++) {
			if (MoveList[j].DeltaScore > BestMoveScore) {
				BestMoveIndex = j;
				BestMoveScore = MoveList[j].DeltaScore;
			}
		}

		TempMoveItem = MoveList[i];
		MoveList[i] = MoveList[BestMoveIndex];
		MoveList[BestMoveIndex] = TempMoveItem;

		MakeMove(MoveList[i]);

		if (CheckKing()) {
			UnmakeMove();

			continue;
		}

//		printf("--M %d %d %d %d %s%s\n", Depth, Ply, Alpha, Beta, BoardName[MoveList[i].Move >> 6], BoardName[MoveList[i].Move & 63]);

		LegalMoveCount++;

		CurrentColor = -CurrentColor;

		BoardHash ^= ColorHash;

		Nodes++;

		TempBestMoves[0] = (MoveItem){ 0, 0, 0, 0ULL };

		Score = -Quiesce(-Beta, -Alpha, Depth - 1, Ply + 1, TempBestMoves, PV, CheckKing());

//		printf("---3 %d %d %d %d\n", Depth, Ply, Score, CurrentColor);

		CurrentColor = -CurrentColor;

		BoardHash ^= ColorHash;

		UnmakeMove();

		if (TimeUp) {
			return 0;
		}

		if (Score > Alpha) {
			if (Score >= Beta) {
				return Beta;
			}

			Alpha = Score;

			SaveBestMoves(BestMoves, MoveList[i], TempBestMoves);
		}
	} // for

	if (IsCheck) {
		if (LegalMoveCount == 0) {
			return -INF + Ply;
		}
	}

	if (FiftyMove >= 100) {
		return 0;
	}

	return Alpha;
}

int Search(int Alpha, int Beta, const int Depth, const int Ply, MoveItem *BestMoves, const BOOL PV, const BOOL UsePruning, const BOOL IsCheck)
{
	int Score;

  #if defined(RAZORING) || defined(FUTILITYPRUNING) || defined(NULLMOVE)
	int CurrentEvaluation;
  #endif // RAZORING || FUTILITYPRUNING || NULLMOVE

	MoveItem TempBestMoves[MAX_PLY];

	int GenMoveCount = 0;
	MoveItem MoveList[256];

	int BestMoveIndex;
	U64 BestMoveScore;
	MoveItem TempMoveItem;

	int LegalMoveCount = 0;

	BOOL GiveCheck;

	int NextDepth;

	#ifdef NULLMOVE
	int SavePassantIndex;
	int NullMoveReduction;
	#endif // NULLMOVE
/*
	U64 TestBoardHash = PositionHash();

	if (BoardHash != TestBoardHash) {
		printf("%llu %llu\n", BoardHash, TestBoardHash);
	}
*/
	if (Depth <= 0) {
		return Quiesce(Alpha, Beta, 0, Ply, BestMoves, PV, IsCheck);
	}

	if (
		Ply > 0
		&& (Nodes & 65535) == 0
		&& TimeStop <= (U64)time(NULL)
	) {
		TimeUp = TRUE;

		return 0;
	}

	if (TimeUp) {
		return 0;
	}

	if (Ply > 0 && (IsInsufficientMaterial() || PositionRepeat1())) {
		return 0;
	}

	if (Ply >= MAX_PLY) {
		return GetEvaluation();
	}

  #if defined(RAZORING) || defined(FUTILITYPRUNING) || defined(NULLMOVE)
	if (UsePruning && !PV && !IsCheck) {
		CurrentEvaluation = GetEvaluation();

		#ifdef RAZORING
		if (Depth <= 4 && (CurrentEvaluation - QUEEN_SCORE) >= Beta) {
			return Beta;
		}
		#endif // RAZORING

		#ifdef FUTILITYPRUNING
		if (Depth <= 2 && (CurrentEvaluation - PAWN_SCORE / 2) >= Beta) {
			return Beta;
		}
		#endif // FUTILITYPRUNING

		#ifdef NULLMOVE
		if (Depth > 1 && CurrentEvaluation >= Beta && NonPawnMaterial()) {
			SavePassantIndex = PassantIndex;

			NullMoveReduction = Depth >= 6 ? 3 : 2;

			if (SavePassantIndex != -1) {
				BoardHash ^= PassantHash[PassantIndex];

				PassantIndex = -1;
			}

			FiftyMove++;

			CurrentColor = -CurrentColor;

			BoardHash ^= ColorHash;

			Nodes++;

			TempBestMoves[0] = (MoveItem){ 0, 0, 0, 0ULL };

			Score = -Search(-Beta, -Beta + 1, Depth - 1 - NullMoveReduction, Ply + 1, TempBestMoves, FALSE, FALSE, CheckKing());

			CurrentColor = -CurrentColor;

			BoardHash ^= ColorHash;

			FiftyMove--;

			if (SavePassantIndex != -1) {
				PassantIndex = SavePassantIndex;

				BoardHash ^= PassantHash[PassantIndex];
			}

			if (TimeUp) {
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
		} // if
		#endif // NULLMOVE
	} // if
  #endif // RAZORING || FUTILITYPRUNING || NULLMOVE

	GenerateAllMoves(MoveList, &GenMoveCount);

	if (FollowPV) {
		FollowPV = FALSE;

		for (int i = 0; i < GenMoveCount; i++) {
			if (MoveList[i].Move == BestMovesMain[Ply].Move && MoveList[i].PawnToPiece == BestMovesMain[Ply].PawnToPiece) {
//				printf("%d %s%s%c\n", Ply, BoardName[MoveList[i].Move >> 6], BoardName[MoveList[i].Move & 63], PiecesCharBlack[MoveList[i].PawnToPiece]);

				MoveList[i].DeltaScore = SORT_PV_MOVE_VALUE;

				FollowPV = TRUE;

				break;
			}
		}
	}

	for (int i = 0; i < GenMoveCount; i++) {
		BestMoveIndex = i;
		BestMoveScore = MoveList[i].DeltaScore;

		for (int j = i + 1; j < GenMoveCount; j++) {
			if (MoveList[j].DeltaScore > BestMoveScore) {
				BestMoveIndex = j;
				BestMoveScore = MoveList[j].DeltaScore;
			}
		}

		TempMoveItem = MoveList[i];
		MoveList[i] = MoveList[BestMoveIndex];
		MoveList[BestMoveIndex] = TempMoveItem;

		MakeMove(MoveList[i]);

		if (CheckKing()) {
			UnmakeMove();

			continue;
		}

//		printf("-M %d %d %d %d %s%s\n", Depth, Ply, Alpha, Beta, BoardName[MoveList[i].Move >> 6], BoardName[MoveList[i].Move & 63]);

		LegalMoveCount++;

		CurrentColor = -CurrentColor;

		BoardHash ^= ColorHash;

		Nodes++;

		GiveCheck = CheckKing();

		if (GiveCheck) {
			NextDepth = Depth;
		}
		else {
			NextDepth = Depth - 1;
		}

		if (PV && LegalMoveCount == 1) {
			TempBestMoves[0] = (MoveItem){ 0, 0, 0, 0ULL };

			Score = -Search(-Beta, -Alpha, NextDepth, Ply + 1, TempBestMoves, TRUE, TRUE, GiveCheck);

//			printf("---0 %d %d %d %d\n", Depth, Ply, Score, CurrentColor);
		}
		else {
			TempBestMoves[0] = (MoveItem){ 0, 0, 0, 0ULL };

			Score = -Search(-Alpha - 1, -Alpha, NextDepth, Ply + 1, TempBestMoves, FALSE, TRUE, GiveCheck);

//			printf("---1 %d %d %d %d\n", Depth, Ply, Score, CurrentColor);

			if (PV && Score > Alpha && (Ply == 0 || Score < Beta)) {
				TempBestMoves[0] = (MoveItem){ 0, 0, 0, 0ULL };

				Score = -Search(-Beta, -Alpha, NextDepth, Ply + 1, TempBestMoves, TRUE, TRUE, GiveCheck);

//				printf("---2 %d %d %d %d\n", Depth, Ply, Score, CurrentColor);
			}
		}

		CurrentColor = -CurrentColor;

		BoardHash ^= ColorHash;

		UnmakeMove();

		if (TimeUp) {
			return 0;
		}

		if (Score > Alpha) {
			if (Score >= Beta) {
				if (!(MoveList[i].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) { // Not capture/promote move
					HistoryHeuristics[CurrentColor == WHITE ? 0 : 1][BoardPiece[MoveList[i].Move >> 6] - 1][MoveList[i].Move & 63] += Depth * Depth;
				}

				return Beta;
			}

			Alpha = Score;

			SaveBestMoves(BestMoves, MoveList[i], TempBestMoves);
		}
	} // for

	if (LegalMoveCount == 0) {
		if (IsCheck) {
			return -INF + Ply;
		}

		return 0;
	}

	if (FiftyMove >= 100) {
		return 0;
	}

	return Alpha;
}

BOOL ComputerMove(const int MaxDepth, const int MaxTimeForMove)
{
	int Score = 0;

	Nodes = 0;

	TimeStart = (U64)time(NULL);
	TimeStop = TimeStart + MaxTimeForMove;

	TimeUp = FALSE;

	memset(HistoryHeuristics, 0, sizeof(HistoryHeuristics));

	BestMovesMain[0] = (MoveItem){ 0, 0, 0, 0ULL };

	for (int Depth = 1; Depth <= MaxDepth; Depth++) {
		FollowPV = TRUE;

		Score = Search(-INF, INF, Depth, 0, BestMovesMain, TRUE, FALSE, CheckKing());

		if (TimeUp) {
			break;
		}

		BestMoveMain = BestMovesMain[0];
		BestEvaluationMain = Score;

		PrintBestMoves(&Depth, BestMovesMain, &BestEvaluationMain);

		if (!BestMoveMain.Move || BestEvaluationMain <= -INF + Depth || BestEvaluationMain >= INF - Depth) {
			break;
		}
	}

	TimeStop = (U64)time(NULL);
	TotalTime = TimeStop - TimeStart;

	if (!BestMoveMain.Move) {
		if (CheckKing()) {
			printf("\n");

			printf("Checkmate!\n");

			printf("\n");

			if (CurrentColor == WHITE) {
				printf("{0-1} Black wins!\n");
			}
			else { // BLACK
				printf("{1-0} White wins!\n");
			}
		}
		else {
			printf("\n");

			printf("{1/2-1/2} Stalemate!\n");
		}

		return FALSE;
	}

	MakeMove(BestMoveMain);

	if (BestEvaluationMain >= INF - 1 || BestEvaluationMain <= -INF + 1) {
		PrintBoard();

		printf("\n");

		if (CurrentColor == WHITE) {
			printf("%d: %s%s%c\n", HalfMoveNumber / 2 + 1, BoardName[BestMoveMain.Move >> 6], BoardName[BestMoveMain.Move & 63], PiecesCharWhite[BestMoveMain.PawnToPiece]);
		}
		else { // BLACK
			printf("%d: ... %s%s%c\n", HalfMoveNumber / 2, BoardName[BestMoveMain.Move >> 6], BoardName[BestMoveMain.Move & 63], PiecesCharBlack[BestMoveMain.PawnToPiece]);
		}

		printf("\n");

		printf("Evaluation %.2f Nodes %llu Time %llu sec. NPS %llu\n", BestEvaluationMain / 100.0, Nodes, TotalTime, (TotalTime > 0 ? Nodes / TotalTime : 0));

		printf("\n");

		printf("Checkmate!\n");

		printf("\n");

		if (CurrentColor == WHITE) {
			printf("{1-0} White wins!\n");
		}
		else { // BLACK
			printf("{0-1} Black wins!\n");
		}

		return FALSE;
	}

	if (IsInsufficientMaterial()) {
		printf("\n");

		printf("{1/2-1/2} Draw by insufficient material!\n");

		return FALSE;
	}

	if (FiftyMove >= 100) {
		printf("\n");

		printf("{1/2-1/2} Draw by fifty move rule!\n");

		return FALSE;
	}

	if (PositionRepeat2() == 2) {
		printf("\n");

		printf("{1/2-1/2} Draw by repetition!\n");

		return FALSE;
	}

	return TRUE;
}

BOOL HumanMove(void)
{
	char ReadStr[10];
	char MoveStr[10];

	int GenMoveCount = 0;
	MoveItem MoveList[256];

	PrintBoard();

	if (BestMoveMain.Move) {
		printf("\n");

		if (CurrentColor == WHITE) {
			printf("%d: ... %s%s%c\n", HalfMoveNumber / 2, BoardName[BestMoveMain.Move >> 6], BoardName[BestMoveMain.Move & 63], PiecesCharBlack[BestMoveMain.PawnToPiece]);
		}
		else { // BLACK
			printf("%d: %s%s%c\n", HalfMoveNumber / 2 + 1, BoardName[BestMoveMain.Move >> 6], BoardName[BestMoveMain.Move & 63], PiecesCharWhite[BestMoveMain.PawnToPiece]);
		}

		printf("\n");

		printf("Evaluation %.2f Nodes %llu Time %llu sec. NPS %llu\n", BestEvaluationMain / 100.0, Nodes, TotalTime, (TotalTime > 0 ? Nodes / TotalTime : 0));
	}

	GenerateAllMoves(MoveList, &GenMoveCount);

	while (TRUE) {
		ReadStr[0] = '\0';

		printf("\n");

		printf("Enter move (e2e4, e7e8q, save, exit) %c ", CheckKing() ? '!' : '>');
		scanf("%s", ReadStr);

		if (!strcmp(ReadStr, "exit")) {
			return FALSE;
		}

		if (!strcmp(ReadStr, "save")) {
			SaveGame();

			continue;
		}

		for (int i = 0; i < GenMoveCount; i++) {
			MoveStr[0] = (char)BoardName[MoveList[i].Move >> 6][0];
			MoveStr[1] = (char)BoardName[MoveList[i].Move >> 6][1];

			MoveStr[2] = (char)BoardName[MoveList[i].Move & 63][0];
			MoveStr[3] = (char)BoardName[MoveList[i].Move & 63][1];

			if (MoveList[i].PawnToPiece > 0) {
				MoveStr[4] = (char)PiecesCharBlack[MoveList[i].PawnToPiece];
				MoveStr[5] = '\0';
			}
			else {
				MoveStr[4] = '\0';
			}

			if (!strcmp(ReadStr, MoveStr)) {
				MakeMove(MoveList[i]);

				if (CheckKing()) {
					UnmakeMove();

					break;
				}

				PrintBoard();

				printf("\n");

				if (CurrentColor == WHITE) {
					printf("%d: %s%s%c\n", HalfMoveNumber / 2 + 1, BoardName[MoveList[i].Move >> 6], BoardName[MoveList[i].Move & 63], PiecesCharWhite[MoveList[i].PawnToPiece]);
				}
				else { // BLACK
					printf("%d: ... %s%s%c\n", HalfMoveNumber / 2, BoardName[MoveList[i].Move >> 6], BoardName[MoveList[i].Move & 63], PiecesCharBlack[MoveList[i].PawnToPiece]);
				}

				if (IsInsufficientMaterial()) {
					printf("\n");

					printf("{1/2-1/2} Draw by insufficient material!\n");

					return FALSE;
				}

				if (FiftyMove >= 100) {
					printf("\n");

					printf("{1/2-1/2} Draw by fifty move rule!\n");

					return FALSE;
				}

				if (PositionRepeat2() == 2) {
					printf("\n");

					printf("{1/2-1/2} Draw by repetition!\n");

					return FALSE;
				}

				return TRUE;
			}
		} // for
	} // while
}

void Game(const int HumanColor, const int ComputerColor)
{
	int MaxDepth;
	int MaxInputDepth;

	int MaxTimeForMove;
	int MaxInputTimeForMove;

	printf("Max. depth: ");
	scanf("%d", &MaxInputDepth);

	MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_PLY) ? MaxInputDepth : MAX_PLY;

	printf("Max. time for move, sec.: ");
	scanf("%d", &MaxInputTimeForMove);

	MaxTimeForMove = (MaxInputTimeForMove > 0 && MaxInputTimeForMove < MAX_TIME) ? MaxInputTimeForMove : MAX_TIME;

	Nodes = 0;

	TotalTime = 0;

	BestMovesMain[0] = (MoveItem){ 0, 0, 0, 0ULL };

	BestMoveMain = (MoveItem){ 0, 0, 0, 0ULL };
	BestEvaluationMain = 0;

	if (CurrentColor == ComputerColor) {
		PrintBoard();
	}

	while (TRUE) {
		if (CurrentColor == HumanColor) {
			if (!HumanMove()) {
				return;
			}

			CurrentColor = -CurrentColor;

			BoardHash ^= ColorHash;
		}

		if (CurrentColor == ComputerColor) {
			if (!ComputerMove(MaxDepth, MaxTimeForMove)) {
				return;
			}

			CurrentColor = -CurrentColor;

			BoardHash ^= ColorHash;
		}
	}
}

void GameAuto(void)
{
	int MaxDepth;
	int MaxInputDepth;

	int MaxTimeForMove;
	int MaxInputTimeForMove;

	printf("Max. depth: ");
	scanf("%d", &MaxInputDepth);

	MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_PLY) ? MaxInputDepth : MAX_PLY;

	printf("Max. time for move, sec.: ");
	scanf("%d", &MaxInputTimeForMove);

	MaxTimeForMove = (MaxInputTimeForMove > 0 && MaxInputTimeForMove < MAX_TIME) ? MaxInputTimeForMove : MAX_TIME;

	Nodes = 0;

	TotalTime = 0;

	BestMovesMain[0] = (MoveItem){ 0, 0, 0, 0ULL };

	BestMoveMain = (MoveItem){ 0, 0, 0, 0ULL };
	BestEvaluationMain = 0;

	PrintBoard();

	while (TRUE) {
		if (!ComputerMove(MaxDepth, MaxTimeForMove)) {
			return;
		}

		PrintBoard();

		printf("\n");

		printf("%d: %s%s%c\n", HalfMoveNumber / 2 + 1, BoardName[BestMoveMain.Move >> 6], BoardName[BestMoveMain.Move & 63], PiecesCharWhite[BestMoveMain.PawnToPiece]);

		printf("\n");

		printf("Evaluation %.2f Nodes %llu Time %llu sec. NPS %llu\n", BestEvaluationMain / 100.0, Nodes, TotalTime, (TotalTime > 0 ? Nodes / TotalTime : 0));

		CurrentColor = -CurrentColor;

		BoardHash ^= ColorHash;

		if (!ComputerMove(MaxDepth, MaxTimeForMove)) {
			return;
		}

		PrintBoard();

		printf("\n");

		printf("%d: ... %s%s%c\n", HalfMoveNumber / 2, BoardName[BestMoveMain.Move >> 6], BoardName[BestMoveMain.Move & 63], PiecesCharBlack[BestMoveMain.PawnToPiece]);

		printf("\n");

		printf("Evaluation %.2f Nodes %llu Time %llu sec. NPS %llu\n", BestEvaluationMain / 100.0, Nodes, TotalTime, (TotalTime > 0 ? Nodes / TotalTime : 0));

		CurrentColor = -CurrentColor;

		BoardHash ^= ColorHash;
	}
}

void InitPiecePosition(void)
{
	int WhitePieceCounter = 0;
	int BlackPieceCounter = 0;

	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 64; j++) {
			if (Pieces[i] == BoardPiece[j]) {
				if (BoardColor[j] == WHITE) {
					WhitePiecePosition[WhitePieceCounter++] = j;
				}
				else { // BLACK
					BlackPiecePosition[BlackPieceCounter++] = j;
				}
			}
		}
	}

	while (WhitePieceCounter < 16) {
		WhitePiecePosition[WhitePieceCounter++] = -1;
	}

	while (BlackPieceCounter < 16) {
		BlackPiecePosition[BlackPieceCounter++] = -1;
	}
}

void InitNewGame(void)
{
	CurrentColor = WHITE;

	BoardPiece[0] = BoardPiece[7] = BoardPiece[56] = BoardPiece[63] = ROOK;
	BoardPiece[1] = BoardPiece[6] = BoardPiece[57] = BoardPiece[62] = KNIGHT;
	BoardPiece[2] = BoardPiece[5] = BoardPiece[58] = BoardPiece[61] = BISHOP;
	BoardPiece[3] = BoardPiece[59] = QUEEN;
	BoardPiece[4] = BoardPiece[60] = KING;

	for (int i = 8; i < 16; i++) {
		BoardPiece[i] = PAWN;
		BoardPiece[i + 40] = PAWN;
	}

	for (int i = 0; i < 16; i++) {
		BoardColor[i] = BLACK;
		BoardColor[i + 48] = WHITE;
	}

	for (int i = 16; i < 48; i++) {
		BoardPiece[i] = EMPTY;
		BoardColor[i] = EMPTY;
	}

	InitPiecePosition();

	CastleFlags = (CASTLE_WHITE_KING | CASTLE_WHITE_QUEEN | CASTLE_BLACK_KING | CASTLE_BLACK_QUEEN);
	PassantIndex = -1;
	HalfMoveNumber = 0;
	FiftyMove = 0;

	BoardHash = PositionHash();
}

void LoadGame(void)
{
	FILE *File;
	int Square;
	char ReadChar;
	char Str[3];
	int MoveNumber;

	File = fopen("chess.fen", "r");

	if (File == NULL) { // File open error
		printf("File 'chess.fen' open error!\n");

		exit(0);
	}

	for (Square = 0; Square < 64; Square++) {
		BoardPiece[Square] = EMPTY;
		BoardColor[Square] = EMPTY;
	}

	Square = 0;

	while ((ReadChar = fgetc(File)) != EOF && ReadChar != ' ') {
		if (ReadChar == 'P') { // White pawn
			BoardPiece[Square] = PAWN;
			BoardColor[Square] = WHITE;

			Square++;
		}
		else if (ReadChar == 'p') { // Black pawn
			BoardPiece[Square] = PAWN;
			BoardColor[Square] = BLACK;

			Square++;
		}
		else if (ReadChar == 'N') { // White knight
			BoardPiece[Square] = KNIGHT;
			BoardColor[Square] = WHITE;

			Square++;
		}
		else if (ReadChar == 'n') { // Black knight
			BoardPiece[Square] = KNIGHT;
			BoardColor[Square] = BLACK;

			Square++;
		}
		else if (ReadChar == 'B') { // White bishop
			BoardPiece[Square] = BISHOP;
			BoardColor[Square] = WHITE;

			Square++;
		}
		else if (ReadChar == 'b') { // Black bishop
			BoardPiece[Square] = BISHOP;
			BoardColor[Square] = BLACK;

			Square++;
		}
		else if (ReadChar == 'R') { // White rook
			BoardPiece[Square] = ROOK;
			BoardColor[Square] = WHITE;

			Square++;
		}
		else if (ReadChar == 'r') { // Black rook
			BoardPiece[Square] = ROOK;
			BoardColor[Square] = BLACK;

			Square++;
		}
		else if (ReadChar == 'Q') { // White queen
			BoardPiece[Square] = QUEEN;
			BoardColor[Square] = WHITE;

			Square++;
		}
		else if (ReadChar == 'q') { // Black queen
			BoardPiece[Square] = QUEEN;
			BoardColor[Square] = BLACK;

			Square++;
		}
		else if (ReadChar == 'K') { // White king
			BoardPiece[Square] = KING;
			BoardColor[Square] = WHITE;

			Square++;
		}
		else if (ReadChar == 'k') { // Black king
			BoardPiece[Square] = KING;
			BoardColor[Square] = BLACK;

			Square++;
		}
		else if (ReadChar >= '1' && ReadChar <= '8') { // Empty square(s)
			Square += ReadChar - '0';
		}
	} // while

	InitPiecePosition();

	ReadChar = fgetc(File);

	if (ReadChar == 'w') {
		CurrentColor = WHITE;
	}
	else { // 'b'
		CurrentColor = BLACK;
	}

	fgetc(File); // ' '

	CastleFlags = 0;

	while ((ReadChar = fgetc(File)) != EOF && ReadChar != '-' && ReadChar != ' ') {
		if (ReadChar == 'K') { // White O-O
			CastleFlags |= CASTLE_WHITE_KING;
		}
		else if (ReadChar == 'Q') { // White O-O-O
			CastleFlags |= CASTLE_WHITE_QUEEN;
		}
		else if (ReadChar == 'k') { // Black O-O
			CastleFlags |= CASTLE_BLACK_KING;
		}
		else if (ReadChar == 'q') { // Black O-O-O
			CastleFlags |= CASTLE_BLACK_QUEEN;
		}
	}

	Str[0] = fgetc(File);

	if (Str[0] == '-') {
		PassantIndex = -1;
	}
	else {
		Str[1] = fgetc(File);
		Str[2] = '\0';

		for (int j = 0; j < 64; j++) {
			if (!strcmp(BoardName[j], Str)) {
				PassantIndex = j;

				break;
			}
		}
	}

	fgetc(File); // ' '

	fscanf(File, "%d", &FiftyMove);

	fgetc(File); // ' '

	fscanf(File, "%d", &MoveNumber);

	HalfMoveNumber = (MoveNumber - 1) * 2 + (CurrentColor == WHITE ? 0 : 1);

	fclose(File);

	BoardHash = PositionHash();
}

void SaveGame(void)
{
	int k = 0;
	FILE *File;

	File = fopen("chess.fen", "w");

	if (File == NULL) { // File open error
		printf("File 'chess.fen' open error!\n");

		return;
	}

	for (int Square = 0; Square < 64; Square++) {
		if (Square > 0 && (Square % 8) == 0) {
			if (k > 0) {
				fprintf(File, "%d", k);

				k = 0;
			}

			fprintf(File, "/");
		}

		if (BoardPiece[Square] == EMPTY) {
			k++;

			continue;
		}

		if (k > 0) {
			fprintf(File, "%d", k);

			k = 0;
		}

		if (BoardColor[Square] == WHITE) {
			fprintf(File, "%c", PiecesCharWhite[BoardPiece[Square]]);
		}
		else { // BLACK
			fprintf(File, "%c", PiecesCharBlack[BoardPiece[Square]]);
		}
	} // for

	if (k > 0) {
		fprintf(File, "%d", k);
	}

	fprintf(File, " ");

	if (CurrentColor == WHITE) {
		fprintf(File, "w");
	}
	else { // BLACK
		fprintf(File, "b");
	}

	fprintf(File, " ");

	if (!CastleFlags) {
		fprintf(File, "-");
	}
	else {
		if (CastleFlags & CASTLE_WHITE_KING) { // White O-O
			fprintf(File, "K");
		}

		if (CastleFlags & CASTLE_WHITE_QUEEN) { // White O-O-O
			fprintf(File, "Q");
		}

		if (CastleFlags & CASTLE_BLACK_KING) { // Black O-O
			fprintf(File, "k");
		}

		if (CastleFlags & CASTLE_BLACK_QUEEN) { // Black O-O-O
			fprintf(File, "q");
		}
	}

	fprintf(File, " ");

	if (PassantIndex == -1) {
		fprintf(File, "-");
	}
	else {
		fprintf(File, "%s", BoardName[PassantIndex]);
	}

	fprintf(File, " ");

	fprintf(File, "%d", FiftyMove);

	fprintf(File, " ");

	fprintf(File, "%d", HalfMoveNumber / 2 + 1);

	fclose(File);
}

U64 CountMoves(const int Depth)
{
	int GenMoveCount = 0;
	MoveItem MoveList[256];

	U64 LegalMoveCounter = 0ULL;

	if (Depth == 0) {
		return 1ULL;
	}

	GenerateAllMoves(MoveList, &GenMoveCount);

	for (int i = 0; i < GenMoveCount; i++) {
		MakeMove(MoveList[i]);

		if (CheckKing()) {
			UnmakeMove();

			continue;
		}

//		printf("%s%s%c\n", BoardName[MoveList[i].Move >> 6], BoardName[MoveList[i].Move & 63], PiecesCharBlack[MoveList[i].PawnToPiece]);

		CurrentColor = -CurrentColor;

		LegalMoveCounter += CountMoves(Depth - 1);

		CurrentColor = -CurrentColor;

		UnmakeMove();
	}

	return LegalMoveCounter;
}

void TestGenerateMoves(void)
{
	int MaxDepth;
	int MaxInputDepth;

	printf("\n");

	printf("Max. depth: ");
	scanf("%d", &MaxInputDepth);

	MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_PLY) ? MaxInputDepth : MAX_PLY;

	printf("\n");

	for (int Depth = 1; Depth <= MaxDepth; Depth++) {
		printf("%d %lld\n", Depth, CountMoves(Depth));
	}
}

int main(void)
{
	int Choice;

	InitHash();
	InitEvaluation();

	while (TRUE) {
		printf("Menu:\n");

		printf("\n");

		printf("1: New white game\n");
		printf("2: New black game\n");
		printf("3: New auto game\n");

		printf("4: Load game from file and white game\n");
		printf("5: Load game from file and black game\n");
		printf("6: Load game from file and auto game\n");

		printf("7: Load game from file and test moves\n");

		printf("8: Exit\n");

		printf("\n");

		printf("Choice: ");
		scanf("%d", &Choice);

		switch (Choice) {
			case 1:
				InitNewGame();
				Game(WHITE, BLACK);
				break;

			case 2:
				InitNewGame();
				Game(BLACK, WHITE);
				break;

			case 3:
				InitNewGame();
				GameAuto();
				break;

			case 4:
				LoadGame();
				Game(WHITE, BLACK);
				break;

			case 5:
				LoadGame();
				Game(BLACK, WHITE);
				break;

			case 6:
				LoadGame();
				GameAuto();
				break;

			case 7:
				LoadGame();
				PrintBoard();
				TestGenerateMoves();
				break;

			case 8:
				return 0;
		} // switch

		printf("\n");
	} // while

	return 0;
}