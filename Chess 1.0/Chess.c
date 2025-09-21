#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <xmmintrin.h>
#include <omp.h>
#include <pthread.h>

#define NAME                          "Chess"
#define VERSION                       "1.0"

#define YEAR                          "2019"
#define AUTHOR                        "Ilya Rukavishnikov"

#define BOOL                          int

#define FALSE                         0
#define TRUE                          1

#define INF                           10000 // Максимальное число для статической оценки короля

#define EMPTY                         0 // Пустая клетка

#define PAWN                          1 // Пешка
#define KNIGHT                        2 // Конь
#define BISHOP                        3 // Слон
#define ROOK                          4 // Ладья
#define QUEEN                         5 // Ферзь
#define KING                          6 // Король

#define WHITE                         8 // Белые фигуры
#define BLACK                         16 // Чёрные фигуры

#define PIECE(Square)                 ((Square) & 7) // Фигура

#define COL(Square)                   ((Square) & 7) // Колонка
#define ROW(Square)                   ((Square) >> 3) // Строчка

#define BONUS(Depth)                  ((Depth) * (Depth)) // Бонус для MOVES_SORT_HEURISTIC

#define MAX(First, Second)            ((First) >= (Second) ? (First) : (Second))
#define MIN(First, Second)            ((First) < (Second) ? (First) : (Second))

#define ABS(Value)                    ((Value) < 0 ? -(Value) : (Value))

// Типы ходов
#define MOVE_SIMPLE                   0 // Простой ход (любой фигурой кроме пешки)

#define MOVE_PAWN                     1 // Ход пешкой
#define MOVE_PAWN_2                   2 // Ход пешкой через клетку
#define MOVE_PAWN_PASSANT             4 // Взятие на проходе
#define MOVE_PAWN_PROMOTE             8 // Превращение пешки

#define MOVE_CAPTURE                  16 // Взятие

#define MOVE_CASTLE_KING              32 // Короткая рокировка (О-О)
#define MOVE_CASTLE_QUEEN             64 // Длинная рокировка (О-О-О)

#define MOVE_NULL                     128

// Флаги возможности рокировки
#define CASTLE_WHITE_KING             1 // Флаг возможности короткой рокировки белого короля (О-О)
#define CASTLE_WHITE_QUEEN            2 // Флаг возможности длинной рокировки белого короля (О-О-О)
#define CASTLE_BLACK_KING             4 // Флаг возможности короткой рокировки чёрного короля (О-О)
#define CASTLE_BLACK_QUEEN            8 // Флаг возможности длинной рокировки чёрного короля (О-О-О)

// Типы записей в хеш-таблице
#define HASH_EXACT                    1
#define HASH_ALPHA                    2
#define HASH_BETA                     4
#define HASH_STATIC_SCORE             8

#define MAX_PLY                       128   // Максимальная глубина просмотра, полуходов
#define MAX_TIME                      86400 // Максимальное время на ход, секунд
#define MAX_GEN_MOVES                 256   // Максимальное число возможных ходов в одной позиции
#define MAX_GAME_MOVES                1024  // Максимальное число ходов в игре

#define MOVES_TO_GO                   40
#define REDUCE_TIME                   50

#define PVS_MOVE_SORT_VALUE           (1 << 30)
#define HASH_MOVE_SORT_VALUE          (1 << 29)
#define PAWN_PROMOTE_MOVE_BONUS       (1 << 28)
#define CAPTURE_MOVE_BONUS            (1 << 27)

#define KILLER_MOVE_SORT_VALUE        (1 << 26)
#define KILLER_MOVE_2_SORT_VALUE      (KILLER_MOVE_SORT_VALUE - 1)

#define MAX_HEURISTIC_SCORE           (1 << 25)

// Отладка
//#define DEBUG_PVS
//#define DEBUG_IID
//#define DEBUG_SEE
//#define DEBUG_SEE_RECURSIVE
//#define DEBUG_HASH

//#define DEBUG_STATISTIC

// Параллельный поиск
//#define PV_SPLITTING
//#define ROOT_SPLITTING
//#define ABDADA
#define LAZY_SMP

// Общие
#define ALPHA_BETA_PRUNING

//#define MOVES_SORT_SEE
#define MOVES_SORT_MVV_LVA

#define MOVES_SORT_HEURISTIC
//#define MOVES_SORT_SQUARE_SCORE

#define HASH_PREFETCH

// Основной поиск
#define HASH_SCORE
#define CHECK_EXTENSION
#define NULL_MOVE_PRUNING
//#define PVS                         // Требуется MOVES_SORT_...
#define IID
#define HASH_MOVE                     // Требуется MOVES_SORT_...
#define KILLER_MOVE                   // Требуется MOVES_SORT_...
#define KILLER_MOVE_2                 // Требуется MOVES_SORT_... и KILLER_MOVE
#define BAD_CAPTURE_LAST              // Требуется MOVES_SORT_MVV_LVA
#define NEGA_SCOUT
//#define LATE_MOVE_REDUCTION         // Требуется NEGA_SCOUT

// Форсированный поиск
#define QUIESCENCE
#define QUIESCENCE_HASH_SCORE
#define QUIESCENCE_CHECK_EXTENSION
//#define QUIESCENCE_PVS              // Требуется MOVES_SORT_...
#define QUIESCENCE_HASH_MOVE          // Требуется MOVES_SORT_...
//#define QUIESCENCE_BAD_CAPTURE_PRUNING

// Размер хеш-таблицы
//#define HASH_TABLE_SIZE             (1 << 17) //    131072     ~2 Mbyte
//#define HASH_TABLE_SIZE             (1 << 18) //    262144     ~4 Mbyte
//#define HASH_TABLE_SIZE             (1 << 19) //    524288     ~8 Mbyte
//#define HASH_TABLE_SIZE             (1 << 20) //   1048576    ~16 Mbyte
//#define HASH_TABLE_SIZE             (1 << 21) //   2097152    ~32 Mbyte
//#define HASH_TABLE_SIZE             (1 << 22) //   4194304    ~64 Mbyte
//#define HASH_TABLE_SIZE             (1 << 23) //   8388608   ~128 Mbyte
//#define HASH_TABLE_SIZE             (1 << 24) //  16777216   ~256 Mbyte
#define HASH_TABLE_SIZE               (1 << 25) //  33554432   ~512 Mbyte
//#define HASH_TABLE_SIZE             (1 << 26) //  67108864  ~1024 Mbyte
//#define HASH_TABLE_SIZE             (1 << 27) // 134217728  ~2048 Mbyte

// Материальная оценка фигур
#define PAWN_SCORE                    100 // Пешка
#define KNIGHT_SCORE                  320 // Конь
#define BISHOP_SCORE                  330 // Слон
#define ROOK_SCORE                    500 // Ладья
#define QUEEN_SCORE                   900 // Ферзь
#define KING_SCORE                    INF // Король

#define ABDADA_HASH_TABLE_SIZE        (1 << 15) // 32768
#define ABDADA_HASH_WAYS              4
#define ABDADA_DEFER_MIN_DEPTH        3
#define ABDADA_CUTOFF_CHECK_MIN_DEPTH 4

#define TEST_CYCLES                   1

typedef signed char                   I8;
typedef short int                     I16;
//typedef int                         I32;
//typedef long long int               I64;

typedef unsigned char                 U8;
typedef unsigned short int            U16;
//typedef unsigned int                U32;
typedef unsigned long long int        U64;

typedef struct { // Структура записи хода (история ходов)
  int Type; // Тип хода

  int From; // Откуда
  int PieceFrom; // Фигура (откуда)
  int PieceFromIndex; // Индекс фигуры (откуда) по списку фигур

  int To; // Куда
  int PieceTo; // Фигура (куда)
  int PieceToIndex; // Индекс фигуры (куда) по списку фигур

  int PassantSquare; // Индекс клетки, через которую прошла пешка
  int EatPawnSquare; // Индекс клетки, где стоит пешка после прохода клетки

  int CastleFlags; // Флаги возможности рокировки белого/чёрного короля влево/вправо
  int FiftyMove;

  int StaticScore;

  U64 Hash; // Хеш позиции
} HistoryItem;

typedef struct { // Структура записи хода (генератор ходов)
  int Type; // Тип хода
  int Move; // Ход (откуда, куда)
  int PromotePiece; // Фигура в которую превращается пешка

  #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
  int SortValue; // Изменение оценки (для сортировки ходов при переборе)
  #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE
} MoveItem;

typedef struct {
  int Pieces[64]; // Piece & Color

  int WhitePieceList[16]; // Списки белых фигур
  int BlackPieceList[16]; // Списки чёрных фигур

  int CurrentColor; // Текущий цвет хода

  int PassantSquare; // Индекс клетки, через которую прошла пешка

  int CastleFlags; // Флаги возможности рокировки белого/чёрного короля влево/вправо
  int FiftyMove;

  int StaticScore;

  HistoryItem MoveTable[MAX_GAME_MOVES];

  #if defined(PVS) || defined(QUIESCENCE_PVS)
  BOOL FollowPV;
  #endif // PVS || QUIESCENCE_PVS

  int HalfMoveNumber;

  U64 Nodes; // Количество просмотренных позиций

  #ifdef DEBUG_STATISTIC
  U64 HashCount; // Количество оценок/ходов полученных их хеша
  U64 EvaluateCount;
  U64 CutoffCount;
  #endif // DEBUG_STATISTIC

  int SelDepth;

  U64 Hash; // Хеш позиции

  MoveItem BestMovesRoot[MAX_PLY]; // Список лучших ходов

  #if defined(LAZY_SMP) && defined(MOVES_SORT_HEURISTIC)
  int HeuristicTable[2][6][64]; // [Color][Piece][Square]
  #endif // LAZY_SMP && MOVES_SORT_HEURISTIC

  #if defined(LAZY_SMP) && defined(KILLER_MOVE)
  int KillerMoveTable[MAX_PLY][2]; // [Ply][Move number]
  #endif // LAZY_SMP && KILLER_MOVE
} BoardItem;

typedef struct {
  I16 Score; // Оценка
  I16 StaticScore; // Статическая оценка
  U16 Move; // Лучший ход
  I8 Depth; // Глубина
  U8 Flag; // Флаг типа записи
} HashDataS; // 8 byte

typedef union {
  U64 RawData;
  HashDataS Data;
} HashDataU; // 8 byte

typedef struct {
  U64 KeyValue;
  HashDataU Value;
} HashItem; // 16 byte

// Доска 10x12 (с флагами выхода за пределы доски)
const int Board120[120] = {
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

// Перекодировка индекса клетки доски 8x8 в индекс клетки доски 10x12
const int Board64[64] = {
  21, 22, 23, 24, 25, 26, 27, 28,
  31, 32, 33, 34, 35, 36, 37, 38,
  41, 42, 43, 44, 45, 46, 47, 48,
  51, 52, 53, 54, 55, 56, 57, 58,
  61, 62, 63, 64, 65, 66, 67, 68,
  71, 72, 73, 74, 75, 76, 77, 78,
  81, 82, 83, 84, 85, 86, 87, 88,
  91, 92, 93, 94, 95, 96, 97, 98
};

// Смещение фигур по направлениям (в размерности доски 10x12)
const int MoveDelta[7][8] = {
  {   0,   0,   0,   0,   0,   0,   0,   0 }, // Не используется
  {   0,   0,   0,   0,   0,   0,   0,   0 }, // Пешка (не используется)
  { -21, -19, -12,  -8,   8,  12,  19,  21 }, // Конь
  { -11,  -9,   9,  11,   0,   0,   0,   0 }, // Слон
  { -10,  -1,   1,  10,   0,   0,   0,   0 }, // Ладья
  { -11, -10,  -9,  -1,   1,   9,  10,  11 }, // Ферзь
  { -11, -10,  -9,  -1,   1,   9,  10,  11 }  // Король
};

// Перекодировка индекса клетки в обозначение на доске
const char BoardName[][64] = {
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
  "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
  "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
  "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

const char PiecesCharWhite[7] = { ' ', 'P', 'N', 'B', 'R', 'Q', 'K' }; // Символы фигур (белые)
const char PiecesCharBlack[7] = { ' ', 'p', 'n', 'b', 'r', 'q', 'k' }; // Символы фигур (чёрные)

const int OrderPieceList[6] = { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN }; // Список фигур: король всегда первый, остальные фигуры по убыванию веса

char StartFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int ComputerMaxThreads;

int MaxDepth;
U64 MaxTimeForMove;
int MaxThreads;

BOOL UciMode;

U64 TimeStart, TimeStop; // Время начала/окончания поиска хода
U64 TotalTime; // Суммарное время поиска хода

volatile BOOL StopSearch;

const int PiecesScore[7] = { 0, PAWN_SCORE, KNIGHT_SCORE, BISHOP_SCORE, ROOK_SCORE, QUEEN_SCORE, 0 }; // Материальная оценка фигур
const int PiecesScoreSEE[7] = { 0, PAWN_SCORE, KNIGHT_SCORE, BISHOP_SCORE, ROOK_SCORE, QUEEN_SCORE, KING_SCORE }; // Материальная оценка фигур (SEE)

BoardItem CurrentBoard;

U64 PieceHash[2][6][64]; // Массив хешей цвета фигуры, фигуры и индекса клетки доски: [Color][Piece][Square]
U64 ColorHash; // Хеш цвета
U64 PassantHash[64]; // Массив хешей взятия на проходе

HashItem HashTable[HASH_TABLE_SIZE]; // Хеш-таблица

#if !defined(LAZY_SMP) && defined(MOVES_SORT_HEURISTIC)
volatile int HeuristicTable[2][6][64]; // [Color][Piece][Square]
#endif // !LAZY_SMP && MOVES_SORT_HEURISTIC

#if !defined(LAZY_SMP) && defined(KILLER_MOVE)
volatile int KillerMoveTable[MAX_PLY][2]; // [Ply][Move number]
#endif // !LAZY_SMP && KILLER_MOVE

#if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
int LateMoveReductionTable[64][64];
#endif // NEGA_SCOUT && LATE_MOVE_REDUCTION

#ifdef ABDADA
volatile int SearchingMovesHashTable[ABDADA_HASH_TABLE_SIZE][ABDADA_HASH_WAYS]; // 32768 * 4 ways * 4 byte = 512 Kbyte
#endif // ABDADA

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

// Прототипы функций
void SaveGame(BoardItem *Board); // Сохранение партии в файле

U64 Clock(void)
{
  struct timeb timebuffer;

  ftime(&timebuffer);

  return timebuffer.time * (U64)1000 + (U64)timebuffer.millitm;
}

U64 RandHash(void) // Получаем 64-битное псевдо-случайное число
{
  return ((U64)rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 31) ^ ((U64)rand() << 47) ^ ((U64)rand() << 59)); // Возвращаем 64-битное псевдо-случайное число
}

void InitHash(void) // Инициализируем хеш фигур, цвета (чёрного) и взятия на проходе
{
//  srand((unsigned int)time(NULL)); // Инициализируем генератор псевдо-случайных чисел
  srand((unsigned int)0); // Инициализируем генератор псевдо-случайных чисел

  for (int Color = 0; Color < 2; ++Color) { // Перебираем цвета фигур (0 - белая; 1 - чёрная)
    for (int Piece = 0; Piece < 6; ++Piece) { // Перебираем список фигур
      for (int Square = 0; Square < 64; ++Square) { // Перебираем все клетки доски
        PieceHash[Color][Piece][Square] = RandHash(); // Инициализируем хеш фигур
      }
    }
  }

  ColorHash = RandHash(); // Инициализируем хеш цвета (чёрного)

  for (int Square = 0; Square < 64; ++Square) { // Перебираем все клетки доски
    PassantHash[Square] = RandHash(); // Инициализируем хеш взятия на проходе
  }
}

void SetBoardHash(BoardItem *Board) // Получаем хеш текущей позиции
{
  U64 Hash = (U64)0; // Хеш текущей позиции

  for (int Index = 0; Index < 16; ++Index) { // Перебираем список белых фигур
    if (Board->WhitePieceList[Index] == -1) { // Нет фигуры
      continue;
    }

    Hash ^= PieceHash[0][PIECE(Board->Pieces[Board->WhitePieceList[Index]]) - 1][Board->WhitePieceList[Index]];
  }

  for (int Index = 0; Index < 16; ++Index) { // Перебираем список чёрных фигур
    if (Board->BlackPieceList[Index] == -1) { // Нет фигуры
      continue;
    }

    Hash ^= PieceHash[1][PIECE(Board->Pieces[Board->BlackPieceList[Index]]) - 1][Board->BlackPieceList[Index]];
  }

  if (Board->CurrentColor == BLACK) { // Ход чёрных
    Hash ^= ColorHash;
  }

  if (Board->PassantSquare != -1) { // Взятие на проходе
    Hash ^= PassantHash[Board->PassantSquare];
  }

  Board->Hash = Hash;
}

#if defined(HASH_SCORE) || defined(HASH_MOVE) || defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
void SaveHash(HashItem *HashItemPointer, const U64 Hash, const int Depth, const int Ply, const int Score, const int StaticScore, const int Move, const int Flag) // Сохраняем оценку и лучший ход в хеш-таблице
{
  HashDataU DataU;

  if ((Flag & HASH_ALPHA) && (HashItemPointer->Value.Data.Flag & (HASH_EXACT | HASH_BETA))) {
    return;
  }

  if (HashItemPointer->Value.Data.Flag && HashItemPointer->Value.Data.Depth > Depth) {
    return;
  }

  if (Score > INF - MAX_PLY) {
    DataU.Data.Score = (I16)(Score + Ply);
  }
  else if (Score < -INF + MAX_PLY) {
    DataU.Data.Score = (I16)(Score - Ply);
  }
  else {
    DataU.Data.Score = (I16)Score;
  }

  DataU.Data.StaticScore = (I16)StaticScore;
  DataU.Data.Move = (U16)Move;
  DataU.Data.Depth = (I8)Depth;
  DataU.Data.Flag = (U8)Flag;

  HashItemPointer->KeyValue = (Hash ^ DataU.RawData);
  HashItemPointer->Value = DataU;
}

void LoadHash(HashItem *HashItemPointer, const U64 Hash, int *Depth, const int Ply, int *Score, int *StaticScore, int *Move, int *Flag)
{
  HashDataU DataU = (*HashItemPointer).Value;

  if ((Hash ^ (*HashItemPointer).KeyValue) != DataU.RawData) {
    return;
  }

  if (DataU.Data.Score > INF - MAX_PLY) {
    *Score = DataU.Data.Score - Ply;
  }
  else if (DataU.Data.Score < -INF + MAX_PLY) {
    *Score = DataU.Data.Score + Ply;
  }
  else {
    *Score = DataU.Data.Score;
  }

  *StaticScore = DataU.Data.StaticScore;
  *Move = DataU.Data.Move;
  *Depth = DataU.Data.Depth;
  *Flag = DataU.Data.Flag;
}
#endif // HASH_SCORE || HASH_MOVE || QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE

int FullHash(void)
{
  int HashHit = 0;

  for (int Index = 0; Index < 1000; ++Index) {
    if (HashTable[Index].Value.Data.Flag) {
      ++HashHit;
    }
  }

  return HashHit;
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

BOOL IsEndGame(BoardItem *Board)
{
  int Square;

	int PieceCount[2][7] = {
		{ 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0 }
	};

	for (int Index = 0; Index < 16; ++Index) { // Перебираем список белых фигур
    Square = Board->WhitePieceList[Index];

		if (Square == -1) { // Нет фигуры
			continue;
		}

		PieceCount[0][PIECE(Board->Pieces[Square])]++;
	}

	for (int Index = 0; Index < 16; ++Index) { // Перебираем список чёрных фигур
    Square = Board->BlackPieceList[Index];

		if (Square == -1) { // Нет фигуры
			continue;
		}

		PieceCount[1][PIECE(Board->Pieces[Square])]++;
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

int Evaluate(BoardItem *Board)
{
	int Score = 0;
	int Square;

  #ifdef DEBUG_STATISTIC
  ++Board->EvaluateCount;
  #endif // DEBUG_STATISTIC

	for (int Index = 0; Index < 16; ++Index) { // Перебираем список белых фигур
		Square = Board->WhitePieceList[Index];

		if (Square == -1) { // Нет фигуры
			continue;
		}

		switch (PIECE(Board->Pieces[Square])) {
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
				if (IsEndGame(Board)) { // End game
					Score += KingSquareScoreEnding[Square];
				}
				else { // Open/Middle game
					Score += KingSquareScoreOpening[Square];
				}
		} // switch
	} // for

	for (int Index = 0; Index < 16; ++Index) { // Перебираем список чёрных фигур
		Square = Board->BlackPieceList[Index];

		if (Square == -1) { // Нет фигуры
			continue;
		}

		switch (PIECE(Board->Pieces[Square])) {
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
				if (IsEndGame(Board)) { // End game
					Score -= KingSquareScoreEnding[Square ^ 56];
				}
				else { // Open/Middle game
					Score -= KingSquareScoreOpening[Square ^ 56];
				}
		} // switch
	} // for

  if (Board->CurrentColor == WHITE) { // Ход белых
    return Score;
  }
  else { // Ход чёрных
    return -Score;
  }
}

int GetPieceIndex(int *PieceList, const int Square) // Получение индекса фигуры по списку фигур
{
  for (int Index = 0; Index < 16; ++Index) { // Перебираем список фигур
    if (PieceList[Index] == Square) { // Нашли фигуру
      return Index; // Возвращаем индекс фигуры по списку фигур
    }
  }

  return -1; // Фигура не найдена
}

void SetPieceIndex(int *PieceList, const int SquareFrom, const int SquareTo) // Перемещение фигуры в списке фигур
{
  for (int Index = 0; Index < 16; ++Index) { // Перебираем список фигур
    if (PieceList[Index] == SquareFrom) { // Нашли фигуру
      PieceList[Index] = SquareTo; // Перемещяем фигуру в списке фигур

      return;
    }
  }
}

void MakeMove(BoardItem *Board, const MoveItem Move) // Сделать ход
{
  HistoryItem *Info = &Board->MoveTable[Board->HalfMoveNumber++];

  int From = Move.Move >> 6;
  int To = Move.Move & 63;

  Info->Type = Move.Type;

  Info->From = From; // Сохраняем откуда пошла фигура
  Info->PieceFrom = PIECE(Board->Pieces[From]); // Сохраняем фигуру в клетке (откуда)

  Info->To = To; // Сохраняем куда пошла фигура
  Info->PieceTo = PIECE(Board->Pieces[To]); // Сохраняем фигуру в клетке (куда)

  Info->PassantSquare = Board->PassantSquare; // Сохраняем индекс клетки, через которую прошла пешка

  Info->CastleFlags = Board->CastleFlags; // Сохраняем флаги возможности рокировки белого/чёрного короля влево/вправо
  Info->FiftyMove = Board->FiftyMove;

  Info->StaticScore = Board->StaticScore;

  Info->Hash = Board->Hash; // Сохраняем хеш текущей позиции

  if (Info->PassantSquare != -1) {
    Board->Hash ^= PassantHash[Info->PassantSquare];

    Board->PassantSquare = -1; // Индекс клетки, через которую прошла пешка
  }

  if (Board->CurrentColor == WHITE) { // Ход белых
    if (Move.Type & MOVE_PAWN_2) { // Первый ход пешки через клетку
      Board->PassantSquare = From - 8; // Индекс клетки, через которую прошла пешка

      Board->Hash ^= PassantHash[Board->PassantSquare];
    }

    if (Move.Type & MOVE_CASTLE_KING) { // Короткая рокировка (О-О)
      // Перемещение ладьи
      Board->Pieces[61] = (ROOK | WHITE);

      SetPieceIndex(Board->WhitePieceList, 63, 61); // Перемещение ладьи в списке фигур

      Board->Hash ^= PieceHash[0][ROOK - 1][63];
      Board->Hash ^= PieceHash[0][ROOK - 1][61];

      Board->Pieces[63] = EMPTY; // Очищаем клетку
    }

    if (Move.Type & MOVE_CASTLE_QUEEN) { // Длинная рокировка (О-О-О)
      // Перемещение ладьи
      Board->Pieces[59] = (ROOK | WHITE);

      SetPieceIndex(Board->WhitePieceList, 56, 59); // Перемещение ладьи в списке фигур

      Board->Hash ^= PieceHash[0][ROOK - 1][56];
      Board->Hash ^= PieceHash[0][ROOK - 1][59];

      Board->Pieces[56] = EMPTY; // Очищаем клетку
    }

    if (Info->PieceFrom == KING && From == 60) {
      Board->CastleFlags &= ~(CASTLE_WHITE_KING | CASTLE_WHITE_QUEEN);
    }

    if (Info->PieceFrom == ROOK) {
      if (From == 63) {
        Board->CastleFlags &= ~CASTLE_WHITE_KING;
      }
      else if (From == 56) {
        Board->CastleFlags &= ~CASTLE_WHITE_QUEEN;
      }
    }

    Info->PieceFromIndex = GetPieceIndex(Board->WhitePieceList, From); // Сохраняем индекс фигуры (откуда) по списку фигур

    Board->Hash ^= PieceHash[0][Info->PieceFrom - 1][From];

    if (Move.Type & MOVE_PAWN_PASSANT) { // Взятие на проходе
      Info->EatPawnSquare = To + 8; // Индекс клетки, где стоит пешка после прохода клетки

      Info->PieceTo = PAWN; // Сохраняем фигуру в клетке (взятие на проходе)
      Info->PieceToIndex = GetPieceIndex(Board->BlackPieceList, Info->EatPawnSquare); // Сохраняем индекс взятой на проходе фигуры по списку фигур

      Board->Hash ^= PieceHash[1][PAWN - 1][Info->EatPawnSquare];

      Board->Pieces[Info->EatPawnSquare] = EMPTY; // Очищаем клетку

      Board->BlackPieceList[Info->PieceToIndex] = -1;
    }
    else if (Move.Type & MOVE_CAPTURE) { // Взятие
      Info->PieceToIndex = GetPieceIndex(Board->BlackPieceList, To); // Сохраняем индекс взятой фигуры по списку фигур

      Board->Hash ^= PieceHash[1][Info->PieceTo - 1][To];

      Board->BlackPieceList[Info->PieceToIndex] = -1;
    }

    Board->Pieces[To] = Board->Pieces[From]; // Копируем фигуру

    Board->WhitePieceList[Info->PieceFromIndex] = To; // Переместили фигуру

    if (Move.Type & MOVE_PAWN_PROMOTE) { // Пешка становится ферзем
      Board->Pieces[To] = (Move.PromotePiece | WHITE); // Pawn -> Queen/Rook/Bishop/Knight
    }

    Board->Pieces[From] = EMPTY; // Очищаем клетку

    Board->Hash ^= PieceHash[0][PIECE(Board->Pieces[To]) - 1][To];
  }
  else { // Ход чёрных
    if (Move.Type & MOVE_PAWN_2) { // Первый ход пешки через клетку
      Board->PassantSquare = From + 8; // Индекс клетки, через которую прошла пешка

      Board->Hash ^= PassantHash[Board->PassantSquare];
    }

    if (Move.Type & MOVE_CASTLE_KING) { // Короткая рокировка (О-О)
      // Перемещение ладьи
      Board->Pieces[5] = (ROOK | BLACK);

      SetPieceIndex(Board->BlackPieceList, 7, 5); // Перемещение ладьи в списке фигур

      Board->Hash ^= PieceHash[1][ROOK - 1][7];
      Board->Hash ^= PieceHash[1][ROOK - 1][5];

      Board->Pieces[7] = EMPTY; // Очищаем клетку
    }

    if (Move.Type & MOVE_CASTLE_QUEEN) { // Длинная рокировка (О-О-О)
      // Перемещение ладьи
      Board->Pieces[3] = (ROOK | BLACK);

      SetPieceIndex(Board->BlackPieceList, 0, 3); // Перемещение ладьи в списке фигур

      Board->Hash ^= PieceHash[1][ROOK - 1][0];
      Board->Hash ^= PieceHash[1][ROOK - 1][3];

      Board->Pieces[0] = EMPTY; // Очищаем клетку (фигура)
    }

    if (Info->PieceFrom == KING && From == 4) {
      Board->CastleFlags &= ~(CASTLE_BLACK_KING | CASTLE_BLACK_QUEEN);
    }

    if (Info->PieceFrom == ROOK) {
      if (From == 7) {
        Board->CastleFlags &= ~CASTLE_BLACK_KING;
      }
      else if (From == 0) {
        Board->CastleFlags &= ~CASTLE_BLACK_QUEEN;
      }
    }

    Info->PieceFromIndex = GetPieceIndex(Board->BlackPieceList, From); // Сохраняем индекс фигуры (откуда) по списку фигур

    Board->Hash ^= PieceHash[1][Info->PieceFrom - 1][From];

    if (Move.Type & MOVE_PAWN_PASSANT) { // Взятие на проходе
      Info->EatPawnSquare = To - 8; // Индекс клетки, где стоит пешка после прохода клетки

      Info->PieceTo = PAWN; // Сохраняем фигуру в клетке (взятие на проходе)
      Info->PieceToIndex = GetPieceIndex(Board->WhitePieceList, Info->EatPawnSquare); // Сохраняем индекс взятой на проходе фигуры по списку фигур

      Board->Hash ^= PieceHash[0][PAWN - 1][Info->EatPawnSquare];

      Board->Pieces[Info->EatPawnSquare] = EMPTY; // Очищаем клетку

      Board->WhitePieceList[Info->PieceToIndex] = -1;
    }
    else if (Move.Type & MOVE_CAPTURE) { // Взятие
      Info->PieceToIndex = GetPieceIndex(Board->WhitePieceList, To); // Сохраняем индекс взятой фигуры по списку фигур

      Board->Hash ^= PieceHash[0][Info->PieceTo - 1][To];

      Board->WhitePieceList[Info->PieceToIndex] = -1;
    }

    Board->Pieces[To] = Board->Pieces[From]; // Копируем фигуру

    Board->BlackPieceList[Info->PieceFromIndex] = To; // Переместили фигуру

    if (Move.Type & MOVE_PAWN_PROMOTE) { // Пешка становится ферзем
      Board->Pieces[To] = (Move.PromotePiece | BLACK); // Pawn -> Queen/Rook/Bishop/Knight
    }

    Board->Pieces[From] = EMPTY; // Очищаем клетку (фигура)

    Board->Hash ^= PieceHash[1][PIECE(Board->Pieces[To]) - 1][To];
  }

  if (Move.Type & (MOVE_CAPTURE | MOVE_PAWN | MOVE_PAWN_2)) {
    Board->FiftyMove = 0;
  }
  else {
    ++Board->FiftyMove;
  }

  Board->CurrentColor ^= (WHITE | BLACK); // Меняем цвет хода

  Board->Hash ^= ColorHash;
}

void UnmakeMove(BoardItem *Board) // Восстановить ход
{
  HistoryItem *Info = &Board->MoveTable[--Board->HalfMoveNumber];

  Board->CurrentColor ^= (WHITE | BLACK); // Восстанавливаем цвет хода

  Board->Pieces[Info->From] = (Info->PieceFrom | Board->CurrentColor); // Восстанавливаем фигуру в клетке (откуда)

  if (Board->CurrentColor == WHITE) { // Ход белых
    Board->WhitePieceList[Info->PieceFromIndex] = Info->From; // Восстанавливаем фигуру (откуда) в списке фигур

    if (Info->Type & MOVE_PAWN_PASSANT) { // Взятие на проходе
      Board->Pieces[Info->To] = EMPTY; // Очищаем клетку

      Board->Pieces[Info->EatPawnSquare] = (PAWN | BLACK); // Восстанавливаем фигуру в клетке (взятие на проходе)

      Board->BlackPieceList[Info->PieceToIndex] = Info->EatPawnSquare; // Восстанавливаем фигуру (куда) в списке фигур
    }
    else if (Info->Type & MOVE_CAPTURE) { // Взятие
      Board->Pieces[Info->To] = (Info->PieceTo | BLACK); // Восстанавливаем фигуру в клетке (куда)

      Board->BlackPieceList[Info->PieceToIndex] = Info->To; // Восстанавливаем фигуру (куда) в списке фигур
    }
    else {
      Board->Pieces[Info->To] = EMPTY; // Очищаем клетку
    }

    if (Info->Type & MOVE_CASTLE_KING) { // Возвращение короткой рокировки О-О
      // Возврат ладьи
      Board->Pieces[63] = (ROOK | WHITE);

      SetPieceIndex(Board->WhitePieceList, 61, 63); // Перемещение ладьи в списке фигур

      Board->Pieces[61] = EMPTY; // Очищаем клетку
    }

    if (Info->Type & MOVE_CASTLE_QUEEN) { // Возвращение длинной рокировки О-О-О
      // Возврат ладьи
      Board->Pieces[56] = (ROOK | WHITE);

      SetPieceIndex(Board->WhitePieceList, 59, 56); // Перемещение ладьи в списке фигур

      Board->Pieces[59] = EMPTY; // Очищаем клетку
    }
  }
  else { // Ход чёрных
    Board->BlackPieceList[Info->PieceFromIndex] = Info->From; // Восстанавливаем фигуру (откуда) в списке фигур

    if (Info->Type & MOVE_PAWN_PASSANT) { // Взятие на проходе
      Board->Pieces[Info->To] = EMPTY; // Очищаем клетку

      Board->Pieces[Info->EatPawnSquare] = (PAWN | WHITE); // Восстанавливаем фигуру в клетке (взятие на проходе)

      Board->WhitePieceList[Info->PieceToIndex] = Info->EatPawnSquare; // Восстанавливаем фигуру (куда) в списке фигур
    }
    else if (Info->Type & MOVE_CAPTURE) { // Взятие
      Board->Pieces[Info->To] = (Info->PieceTo | WHITE); // Восстанавливаем фигуру в клетке (куда)

      Board->WhitePieceList[Info->PieceToIndex] = Info->To; // Восстанавливаем фигуру (куда) в списке фигур
    }
    else {
      Board->Pieces[Info->To] = EMPTY; // Очищаем клетку
    }

    if (Info->Type & MOVE_CASTLE_KING) { // Возвращение короткой рокировки О-О
      // Возврат ладьи
      Board->Pieces[7] = (ROOK | BLACK);

      SetPieceIndex(Board->BlackPieceList, 5, 7); // Перемещение ладьи в списке фигур

      Board->Pieces[5] = EMPTY; // Очищаем клетку
    }

    if (Info->Type & MOVE_CASTLE_QUEEN) { // Возвращение длинной рокировки О-О-О
      // Возврат ладьи
      Board->Pieces[0] = (ROOK | BLACK);

      SetPieceIndex(Board->BlackPieceList, 3, 0); // Перемещение ладьи в списке фигур

      Board->Pieces[3] = EMPTY; // Очищаем клетку
    }
  }

  Board->PassantSquare = Info->PassantSquare; // Восстанавливаем индекс клетки, через которую прошла пешка

  Board->CastleFlags = Info->CastleFlags; // Восстанавливаем флаги возможности рокировки белого/чёрного короля влево/вправо
  Board->FiftyMove = Info->FiftyMove;

  Board->StaticScore = Info->StaticScore;

  Board->Hash = Info->Hash; // Восстанавливаем хеш текущей позиции
}

void MakeNullMove(BoardItem *Board) // Сделать ход
{
  HistoryItem *Info = &Board->MoveTable[Board->HalfMoveNumber++];

  Info->Type = MOVE_NULL;

  Info->PassantSquare = Board->PassantSquare; // Сохраняем индекс клетки, через которую прошла пешка

  Info->FiftyMove = Board->FiftyMove;

  Info->StaticScore = Board->StaticScore;

  Info->Hash = Board->Hash; // Сохраняем хеш текущей позиции

  if (Info->PassantSquare != -1) {
    Board->Hash ^= PassantHash[Info->PassantSquare];

    Board->PassantSquare = -1; // Индекс клетки, через которую прошла пешка
  }

  ++Board->FiftyMove;

  Board->CurrentColor ^= (WHITE | BLACK); // Меняем цвет хода

  Board->Hash ^= ColorHash;
}

void UnmakeNullMove(BoardItem *Board) // Восстановить ход
{
  HistoryItem *Info = &Board->MoveTable[--Board->HalfMoveNumber];

  Board->CurrentColor ^= (WHITE | BLACK); // Восстанавливаем цвет хода

  Board->PassantSquare = Info->PassantSquare; // Восстанавливаем индекс клетки, через которую прошла пешка

  Board->FiftyMove = Info->FiftyMove;

  Board->StaticScore = Info->StaticScore;

  Board->Hash = Info->Hash; // Восстанавливаем хеш текущей позиции
}

BOOL IsSquareAttacked(BoardItem *Board, const int Square, const int Color) // Проверка атаки клетки
{
  int To;

  // Пешка
  if (Color == WHITE) { // Ход белых
    if (
      (ROW(Square) > 1 && COL(Square) != 0 && Board->Pieces[Square - 9] == (PAWN | BLACK)) ||
      (ROW(Square) > 1 && COL(Square) != 7 && Board->Pieces[Square - 7] == (PAWN | BLACK))
    ) { // Взятие пешкой возможно
      return TRUE; // Клетка атакована
    }
  }
  else { // Ход чёрных
    if (
      (ROW(Square) < 6 && COL(Square) != 0 && Board->Pieces[Square + 7] == (PAWN | WHITE)) ||
      (ROW(Square) < 6 && COL(Square) != 7 && Board->Pieces[Square + 9] == (PAWN | WHITE))
    ) { // Взятие пешкой возможно
      return TRUE; // Клетка атакована
    }
  }

  // Конь
  for (int Direction = 0; Direction < 8; ++Direction) {
    To = Board120[Board64[Square] + MoveDelta[KNIGHT][Direction]];

    if (To == -1) { // Вышли за пределы доски
      continue;
    }

    if (Board->Pieces[To] == EMPTY || (Board->Pieces[To] & Color)) { // Пустая клетка или своя фигура
      continue;
    }

    // Фигура противника

    if (PIECE(Board->Pieces[To]) == KNIGHT) { // Конь
      return TRUE; // Клетка атакована
    }
  }

  // Ладья, ферзь или король
  for (int Direction = 0; Direction < 4; ++Direction) {
    To = Square;

    for (int Step = 1; Step < 8; ++Step) {
      To = Board120[Board64[To] + MoveDelta[ROOK][Direction]];

      if (To == -1) { // Вышли за пределы доски
        break;
      }

      if (Board->Pieces[To] == EMPTY) { // Пустая клетка
        continue;
      }

      if (Board->Pieces[To] & Color) { // Своя фигура
        break;
      }

      // Фигура противника

      if (Step == 1 && PIECE(Board->Pieces[To]) == KING) { // Король
        return TRUE; // Клетка атакована
      }

      if (PIECE(Board->Pieces[To]) == ROOK || PIECE(Board->Pieces[To]) == QUEEN) { // Ладья или ферзь
        return TRUE; // Клетка атакована
      }

      break;
    }
  }

  // Слон, ферзь или король
  for (int Direction = 0; Direction < 4; ++Direction) {
    To = Square;

    for (int Step = 1; Step < 8; ++Step) {
      To = Board120[Board64[To] + MoveDelta[BISHOP][Direction]];

      if (To == -1) { // Вышли за пределы доски
        break;
      }

      if (Board->Pieces[To] == EMPTY) { // Пустая клетка
        continue;
      }

      if (Board->Pieces[To] & Color) { // Своя фигура
        break;
      }

      // Фигура противника

      if (Step == 1 && PIECE(Board->Pieces[To]) == KING) { // Король
        return TRUE; // Клетка атакована
      }

      if (PIECE(Board->Pieces[To]) == BISHOP || PIECE(Board->Pieces[To]) == QUEEN) { // Слон или ферзь
        return TRUE; // Клетка атакована
      }

      break;
    }
  }

  return FALSE; // Клетка не атакована
}

BOOL IsInCheck(BoardItem *Board, const int Color) // Проверка на шах
{
  int KingSquare; // Позиция короля

  if (Color == WHITE) { // Ход белых
    KingSquare = Board->WhitePieceList[0]; // Позиция белого короля
  }
  else { // Ход чёрных
    KingSquare = Board->BlackPieceList[0]; // Позиция чёрного короля
  }

  return IsSquareAttacked(Board, KingSquare, Color);
}

void PrintBoard(BoardItem *Board) // Выводим доску и фигуры
{
  printf("\n     a   b   c   d   e   f   g   h\n");

  for (int Square = 0; Square < 64; ++Square) { // Перебираем все клетки доски
    if (!(Square % 8)) {
      printf("   +---+---+---+---+---+---+---+---+\n");
      printf(" %d |", (8 - Square / 8));
    }

    if (Board->Pieces[Square] == EMPTY) { // Пустая клетка
      printf("   |");
    }
    else if (Board->Pieces[Square] & WHITE) { // Белая фигура
      printf(" %c |", PiecesCharWhite[PIECE(Board->Pieces[Square])]);
    }
    else { // Чёрная фигура
      printf(" %c |", PiecesCharBlack[PIECE(Board->Pieces[Square])]);
    }

    if (!((Square + 1) % 8)) {
      printf(" %d\n", (8 - Square / 8));
    }
  }

  printf("   +---+---+---+---+---+---+---+---+\n");
  printf("     a   b   c   d   e   f   g   h\n\n");
}

void PrintBestMoves(BoardItem *Board, const int Depth, MoveItem *BestMoves, const int BestScore) // Выводим список лучших ходов
{
  TotalTime = Clock() - TimeStart; // Суммарное время поиска хода

  if (UciMode) {
    printf("info depth %d seldepth %d nodes %llu time %llu", Depth, Board->SelDepth, Board->Nodes, TotalTime);

    if (BestScore >= INF - MAX_PLY) {
      printf(" score mate %d", INF - BestScore);
    }
    else if (BestScore <= -INF + MAX_PLY) {
      printf(" score mate %d", INF + BestScore);
    }
    else {
      printf(" score cp %d", BestScore);
    }

    if (TotalTime > 0) {
      printf(" nps %d", (int)(1000 * Board->Nodes / TotalTime));
    }

    printf(" hashfull %d", FullHash());

    printf(" pv");
  }
  else {
    printf("Depth %2d / %2d Score %6.2f Nodes %9llu Hashfull %5.2f%% Time %6.2f", Depth, Board->SelDepth, ((double)(BestScore) / PAWN_SCORE), Board->Nodes, ((double)FullHash() / 10), ((double)TotalTime / 1000));

    printf(" PV");
  }

  for (int MoveNumber = 0; (MoveNumber < MAX_PLY && BestMoves[MoveNumber].Move); ++MoveNumber) {
    printf(" %s%s", BoardName[BestMoves[MoveNumber].Move >> 6], BoardName[BestMoves[MoveNumber].Move & 63]);

    if (BestMoves[MoveNumber].PromotePiece) {
      printf("%c", PiecesCharBlack[BestMoves[MoveNumber].PromotePiece]);
    }
  }

  printf("\n");
}

void SaveBestMoves(MoveItem *BestMoves, const MoveItem BestMove, MoveItem *TempBestMoves) // Сохраняем лучший ход
{
  int MoveNumber;

  BestMoves[0] = BestMove; // Лучший ход в начало списка

  // Переносим лучшие ходы с нижнего уровня
  for (MoveNumber = 0; (MoveNumber < (MAX_PLY - 2) && TempBestMoves[MoveNumber].Move); ++MoveNumber) {
    BestMoves[MoveNumber + 1] = TempBestMoves[MoveNumber];
  }

  BestMoves[MoveNumber + 1] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов
}

#if defined(MOVES_SORT_SEE) || (defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)) || defined(QUIESCENCE_BAD_CAPTURE_PRUNING)
int GetSmallestAttacker(BoardItem *Board, const int Square)
{
  int To;

  // Пешка
  if (Board->CurrentColor == WHITE) { // Ход белых
    if (ROW(Square) > 1 && COL(Square) != 0 && Board->Pieces[Square - 9] == (PAWN | BLACK)) { // Взятие пешкой возможно
      return Square - 9;
    }
    else if (ROW(Square) > 1 && COL(Square) != 7 && Board->Pieces[Square - 7] == (PAWN | BLACK)) { // Взятие пешкой возможно
      return Square - 7;
    }
  }
  else { // Ход чёрных
    if (ROW(Square) < 6 && COL(Square) != 0 && Board->Pieces[Square + 7] == (PAWN | WHITE)) { // Взятие пешкой возможно
      return Square + 7;
    }
    else if (ROW(Square) < 6 && COL(Square) != 7 && Board->Pieces[Square + 9] == (PAWN | WHITE)) { // Взятие пешкой возможно
      return Square + 9;
    }
  }

  // Конь
  for (int Direction = 0; Direction < 8; ++Direction) {
    To = Board120[Board64[Square] + MoveDelta[KNIGHT][Direction]];

    if (To == -1) { // Вышли за пределы доски
      continue;
    }

    if (Board->Pieces[To] == EMPTY || (Board->Pieces[To] & Board->CurrentColor)) { // Пустая клетка или своя фигура
      continue;
    }

    // Фигура противника

    if (PIECE(Board->Pieces[To]) == KNIGHT) { // Конь
      return To;
    }
  }

  // Слон
  for (int Direction = 0; Direction < 4; ++Direction) {
    To = Square;

    for (int Step = 1; Step < 8; ++Step) {
      To = Board120[Board64[To] + MoveDelta[BISHOP][Direction]];

      if (To == -1) { // Вышли за пределы доски
        break;
      }

      if (Board->Pieces[To] == EMPTY) { // Пустая клетка
        continue;
      }

      if (Board->Pieces[To] & Board->CurrentColor) { // Своя фигура
        break;
      }

      // Фигура противника

      if (PIECE(Board->Pieces[To]) == BISHOP) { // Слон
        return To;
      }

      break;
    }
  }

  // Ладья
  for (int Direction = 0; Direction < 4; ++Direction) {
    To = Square;

    for (int Step = 1; Step < 8; ++Step) {
      To = Board120[Board64[To] + MoveDelta[ROOK][Direction]];

      if (To == -1) { // Вышли за пределы доски
        break;
      }

      if (Board->Pieces[To] == EMPTY) { // Пустая клетка
        continue;
      }

      if (Board->Pieces[To] & Board->CurrentColor) { // Своя фигура
        break;
      }

      // Фигура противника

      if (PIECE(Board->Pieces[To]) == ROOK) { // Ладья
        return To;
      }

      break;
    }
  }

  // Ферзь
  for (int Direction = 0; Direction < 8; ++Direction) {
    To = Square;

    for (int Step = 1; Step < 8; ++Step) {
      To = Board120[Board64[To] + MoveDelta[QUEEN][Direction]];

      if (To == -1) { // Вышли за пределы доски
        break;
      }

      if (Board->Pieces[To] == EMPTY) { // Пустая клетка
        continue;
      }

      if (Board->Pieces[To] & Board->CurrentColor) { // Своя фигура
        break;
      }

      // Фигура противника

      if (PIECE(Board->Pieces[To]) == QUEEN) { // Ферзь
        return To;
      }

      break;
    }
  }

  // Король
  for (int Direction = 0; Direction < 8; ++Direction) {
    To = Board120[Board64[Square] + MoveDelta[KING][Direction]];

    if (To == -1) { // Вышли за пределы доски
      continue;
    }

    if (Board->Pieces[To] == EMPTY || (Board->Pieces[To] & Board->CurrentColor)) { // Пустая клетка или своя фигура
      continue;
    }

    // Фигура противника

    if (PIECE(Board->Pieces[To]) == KING) { // Король
      return To;
    }
  }

  return -1; // Клетка не атакована
}

int SEE(BoardItem *Board, const int To)
{
  int From = GetSmallestAttacker(Board, To);

  int PieceTo;
  int Result;

  if (From == -1) { // Клетка не атакована
    return 0;
  }

  // Клетка атакована

  PieceTo = Board->Pieces[To];

  Board->Pieces[To] = Board->Pieces[From];
  Board->Pieces[From] = EMPTY;

  Board->CurrentColor ^= (WHITE | BLACK); // Меняем цвет хода

  Result = PiecesScoreSEE[PIECE(PieceTo)] - SEE(Board, To);

  Board->CurrentColor ^= (WHITE | BLACK); // Восстанавливаем цвет хода

  Board->Pieces[From] = Board->Pieces[To];
  Board->Pieces[To] = PieceTo;

  if (Result < 0) {
    Result = 0;
  }

  #ifdef DEBUG_SEE_RECURSIVE
  if (Board->CurrentColor == WHITE) { // Ход белых
    printf(" %s%s %c -> %c %d\n", BoardName[From], BoardName[To], PiecesCharBlack[PIECE(Board->Pieces[From])], PiecesCharWhite[PIECE(Board->Pieces[To])], Result);
  }
  else { // Ход чёрных
    printf(" %s%s %c -> %c %d\n", BoardName[From], BoardName[To], PiecesCharWhite[PIECE(Board->Pieces[From])], PiecesCharBlack[PIECE(Board->Pieces[To])], Result);
  }
  #endif // DEBUG_SEE_RECURSIVE

  return Result;
}

int CaptureSEE(BoardItem *Board, const int From, const int To, const int MoveType)
{
  int PieceTo;
  int Result;

  if (MoveType & MOVE_PAWN_PASSANT) { // Взятие на проходе
    Board->Pieces[To] = Board->Pieces[From];
    Board->Pieces[From] = EMPTY;

    if (Board->CurrentColor == WHITE) { // Ход белых
      Board->Pieces[To + 8] = EMPTY;
    }
    else { // Ход чёрных
      Board->Pieces[To - 8] = EMPTY;
    }

    Result = PAWN_SCORE - SEE(Board, To);

    if (Board->CurrentColor == WHITE) { // Ход белых
      Board->Pieces[To + 8] = (PAWN | BLACK);
    }
    else { // Ход чёрных
      Board->Pieces[To - 8] = (PAWN | WHITE);
    }

    Board->Pieces[From] = Board->Pieces[To];
    Board->Pieces[To] = EMPTY;
  }
  else/* if(MoveType & MOVE_CAPTURE)*/ { // Взятие
    PieceTo = Board->Pieces[To];

    Board->Pieces[To] = Board->Pieces[From];
    Board->Pieces[From] = EMPTY;

    Result = PiecesScoreSEE[PIECE(PieceTo)] - SEE(Board, To);

    Board->Pieces[From] = Board->Pieces[To];
    Board->Pieces[To] = PieceTo;
  }

  #ifdef DEBUG_SEE
  if (Board->CurrentColor == WHITE) { // Ход белых
    printf("%s%s %c -> %c %d\n", BoardName[From], BoardName[To], PiecesCharWhite[PIECE(Board->Pieces[From])], PiecesCharBlack[PIECE(Board->Pieces[To])], Result);
  }
  else { // Ход чёрных
    printf("%s%s %c -> %c %d\n", BoardName[From], BoardName[To], PiecesCharBlack[PIECE(Board->Pieces[From])], PiecesCharWhite[PIECE(Board->Pieces[To])], Result);
  }
  #endif // DEBUG_SEE

  return Result;
}
#endif // MOVES_SORT_SEE || (MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST) || QUIESCENCE_BAD_CAPTURE_PRUNING

void AddMove(BoardItem *Board, MoveItem *MoveList, int *GenMoveCount, const int From, const int To, const int MoveType)
{
  int Move = (From << 6) | To;

  #ifdef MOVES_SORT_SEE
  int SEE_Value;
  #endif // MOVES_SORT_SEE

  if ((MoveType & MOVE_PAWN) && (ROW(To) == 0 || ROW(To) == 7)) { // Превращение пешки
    // Queen
    MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
    MoveList[*GenMoveCount].Move = Move;
    MoveList[*GenMoveCount].PromotePiece = QUEEN;

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    MoveList[*GenMoveCount].SortValue = PAWN_PROMOTE_MOVE_BONUS + QUEEN_SCORE;
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    ++(*GenMoveCount);

    // Rook
    MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
    MoveList[*GenMoveCount].Move = Move;
    MoveList[*GenMoveCount].PromotePiece = ROOK;

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    MoveList[*GenMoveCount].SortValue = PAWN_PROMOTE_MOVE_BONUS + ROOK_SCORE;
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    ++(*GenMoveCount);

    // Bishop
    MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
    MoveList[*GenMoveCount].Move = Move;
    MoveList[*GenMoveCount].PromotePiece = BISHOP;

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    MoveList[*GenMoveCount].SortValue = PAWN_PROMOTE_MOVE_BONUS + BISHOP_SCORE;
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    ++(*GenMoveCount);

    // Knight
    MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
    MoveList[*GenMoveCount].Move = Move;
    MoveList[*GenMoveCount].PromotePiece = KNIGHT;

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    MoveList[*GenMoveCount].SortValue = PAWN_PROMOTE_MOVE_BONUS + KNIGHT_SCORE;
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    ++(*GenMoveCount);
  }
  else {
    MoveList[*GenMoveCount].Type = MoveType;
    MoveList[*GenMoveCount].Move = Move;
    MoveList[*GenMoveCount].PromotePiece = 0;

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    if (MoveType & MOVE_CAPTURE) { // Взятие
      #ifdef MOVES_SORT_SEE
      SEE_Value = CaptureSEE(Board, From, To, MoveType);

      if (SEE_Value >= 0) {
        MoveList[*GenMoveCount].SortValue = SEE_Value + CAPTURE_MOVE_BONUS;
      }
      else {
        MoveList[*GenMoveCount].SortValue = SEE_Value - CAPTURE_MOVE_BONUS;
      }
      #elif defined(MOVES_SORT_MVV_LVA)
      if (MoveType & MOVE_PAWN_PASSANT) { // Взятие на проходе
        MoveList[*GenMoveCount].SortValue = CAPTURE_MOVE_BONUS + (PAWN << 3) - PAWN;
      }
      else { // Взятие
        MoveList[*GenMoveCount].SortValue = CAPTURE_MOVE_BONUS + (PIECE(Board->Pieces[To]) << 3) - PIECE(Board->Pieces[From]);
      }
      #else // NONE
      MoveList[*GenMoveCount].SortValue = 0;
      #endif // MOVES_SORT_SEE/MOVES_SORT_MVV_LVA/NONE
    }
    else { // Простое перемещение
      #ifdef MOVES_SORT_HEURISTIC
      #ifdef LAZY_SMP
      MoveList[*GenMoveCount].SortValue = Board->HeuristicTable[Board->CurrentColor == WHITE ? 0 : 1][PIECE(Board->Pieces[From]) - 1][To];
      #else
      MoveList[*GenMoveCount].SortValue = HeuristicTable[Board->CurrentColor == WHITE ? 0 : 1][PIECE(Board->Pieces[From]) - 1][To];
      #endif // LAZY_SMP
      #elif defined(MOVES_SORT_SQUARE_SCORE)
      if (PIECE(Board->Pieces[From]) == PAWN) { // Пешка
        if (Board->CurrentColor == WHITE) { // Ход белых
          MoveList[*GenMoveCount].SortValue = PawnSquareScore[To] - PawnSquareScore[From];
        }
        else { // Ход чёрных
          MoveList[*GenMoveCount].SortValue = PawnSquareScore[To ^ 56] - PawnSquareScore[From ^ 56];
        }
      }
      else if (PIECE(Board->Pieces[From]) == KNIGHT) { // Конь
        if (Board->CurrentColor == WHITE) { // Ход белых
          MoveList[*GenMoveCount].SortValue = KnightSquareScore[To] - KnightSquareScore[From];
        }
        else { // Ход чёрных
          MoveList[*GenMoveCount].SortValue = KnightSquareScore[To ^ 56] - KnightSquareScore[From ^ 56];
        }
      }
      else if (PIECE(Board->Pieces[From]) == BISHOP) { // Слон
        if (Board->CurrentColor == WHITE) { // Ход белых
          MoveList[*GenMoveCount].SortValue = BishopSquareScore[To] - BishopSquareScore[From];
        }
        else { // Ход чёрных
          MoveList[*GenMoveCount].SortValue = BishopSquareScore[To ^ 56] - BishopSquareScore[From ^ 56];
        }
      }
      else if (PIECE(Board->Pieces[From]) == ROOK) { // Ладья
        if (Board->CurrentColor == WHITE) { // Ход белых
          MoveList[*GenMoveCount].SortValue = RookSquareScore[To] - RookSquareScore[From];
        }
        else { // Ход чёрных
          MoveList[*GenMoveCount].SortValue = RookSquareScore[To ^ 56] - RookSquareScore[From ^ 56];
        }
      }
      else if (PIECE(Board->Pieces[From]) == QUEEN) { // Ферзь
        if (Board->CurrentColor == WHITE) { // Ход белых
          MoveList[*GenMoveCount].SortValue = QueenSquareScore[To] - QueenSquareScore[From];
        }
        else { // Ход чёрных
          MoveList[*GenMoveCount].SortValue = QueenSquareScore[To ^ 56] - QueenSquareScore[From ^ 56];
        }
      }
      else/* if (PIECE(Board->Pieces[From]) == KING)*/ { // Король
        if (Board->CurrentColor == WHITE) { // Ход белых
          if (IsEndGame(Board)) {
            MoveList[*GenMoveCount].SortValue = KingSquareScoreEnding[To] - KingSquareScoreEnding[From];
          }
          else {
            MoveList[*GenMoveCount].SortValue = KingSquareScoreOpening[To] - KingSquareScoreOpening[From];
          }
        }
        else { // Ход чёрных
          if (IsEndGame(Board)) {
            MoveList[*GenMoveCount].SortValue = KingSquareScoreEnding[To ^ 56] - KingSquareScoreEnding[From ^ 56];
          }
          else {
            MoveList[*GenMoveCount].SortValue = KingSquareScoreOpening[To ^ 56] - KingSquareScoreOpening[From ^ 56];
          }
        }
      }
      #else // NONE
      MoveList[*GenMoveCount].SortValue = 0;
      #endif // MOVES_SORT_HEURISTIC/MOVES_SORT_SQUARE_SCORE/NONE
    }
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    ++(*GenMoveCount);
  }
}

void GenerateAllMoves(BoardItem *Board, MoveItem *MoveList, int *GenMoveCount) // Генерируем все возможные ходы (без учёта шаха после хода)
{
  int From;
  int To;

  int *FromPiecePosition; // Указатель на список фигур (откуда)

  if (Board->CurrentColor == WHITE) { // Ход белых
    FromPiecePosition = Board->WhitePieceList;
  }
  else { // Ход чёрных
    FromPiecePosition = Board->BlackPieceList;
  }

  for (int Index = 0; Index < 16; ++Index) { // Перебираем список фигур (откуда)
    From = FromPiecePosition[Index];

    if (From == -1) { // Нет фигуры
      continue;
    }

    if (PIECE(Board->Pieces[From]) == PAWN) { // Пешка
      if (Board->CurrentColor == WHITE) { // Ход белых
        if (COL(From) != 0) {
          To = From - 9;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & BLACK) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        if (COL(From) != 7) {
          To = From - 7;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & BLACK) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        To = From - 8;

        if (Board->Pieces[To] == EMPTY) { // Ход возможен
          AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_PAWN);

          if (ROW(From) == 6) {
            To = From - 16;

            if (Board->Pieces[To] == EMPTY) { // Ход через клетку возможен
              AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_PAWN_2);
            }
          }
        }
      }
      else { // Ход чёрных
        if (COL(From) != 0) {
          To = From + 7;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & WHITE) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        if (COL(From) != 7) {
          To = From + 9;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & WHITE) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        To = From + 8;

        if (Board->Pieces[To] == EMPTY) { // Ход возможен
          AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_PAWN);

          if (ROW(From) == 1) {
            To = From + 16;

            if (Board->Pieces[To] == EMPTY) { // Ход через клетку возможен
              AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_PAWN_2);
            }
          }
        }
      }
    }
    else { // Конь, слон, ладья, ферзь или король
      for (int Direction = 0; Direction < 8; ++Direction) {
        if (!MoveDelta[PIECE(Board->Pieces[From])][Direction]) {
          break;
        }

        To = From;

        while (TRUE) {
          To = Board120[Board64[To] + MoveDelta[PIECE(Board->Pieces[From])][Direction]];

          if (To == -1) { // Вышли за пределы доски
            break;
          }

          if (Board->Pieces[To] & Board->CurrentColor) { // Своя фигура
            break;
          }

          if (Board->Pieces[To] != EMPTY) { // Взятие
            AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            break;
          }

          // Простой ход

          AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_SIMPLE);

          if (PIECE(Board->Pieces[From]) == KING || PIECE(Board->Pieces[From]) == KNIGHT) { // Король или конь
            break;
          }
        } // while
      }

      if (PIECE(Board->Pieces[From]) == KING) { // Король
        if (Board->CurrentColor == WHITE) { // Ход белых
          if (From == 60) {
            if (
              (Board->CastleFlags & CASTLE_WHITE_KING) &&
              Board->Pieces[61] == EMPTY && Board->Pieces[62] == EMPTY && (Board->Pieces[63] == (ROOK | WHITE)) &&
              !IsSquareAttacked(Board, 60, Board->CurrentColor) && !IsSquareAttacked(Board, 61, Board->CurrentColor) && !IsSquareAttacked(Board, 62, Board->CurrentColor)
            ) { // Короткая рокировка (О-О)
              To = 62;

              AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_CASTLE_KING);
            }

            if (
              (Board->CastleFlags & CASTLE_WHITE_QUEEN) &&
              Board->Pieces[59] == EMPTY && Board->Pieces[58] == EMPTY && Board->Pieces[57] == EMPTY && (Board->Pieces[56] == (ROOK | WHITE)) &&
              !IsSquareAttacked(Board, 60, Board->CurrentColor) && !IsSquareAttacked(Board, 59, Board->CurrentColor) && !IsSquareAttacked(Board, 58, Board->CurrentColor)
            ) { // Длинная рокировка (О-О-О)
              To = 58;

              AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_CASTLE_QUEEN);
            }
          }
        }
        else { // Ход чёрных
          if (From == 4) {
            if (
              (Board->CastleFlags & CASTLE_BLACK_KING) &&
              Board->Pieces[5] == EMPTY && Board->Pieces[6] == EMPTY && (Board->Pieces[7] == (ROOK | BLACK)) &&
              !IsSquareAttacked(Board, 4, Board->CurrentColor) && !IsSquareAttacked(Board, 5, Board->CurrentColor) && !IsSquareAttacked(Board, 6, Board->CurrentColor)
            ) { // Короткая рокировка (О-О)
              To = 6;

              AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_CASTLE_KING);
            }

            if (
              (Board->CastleFlags & CASTLE_BLACK_QUEEN) &&
              Board->Pieces[3] == EMPTY && Board->Pieces[2] == EMPTY && Board->Pieces[1] == EMPTY && (Board->Pieces[0] == (ROOK | BLACK)) &&
              !IsSquareAttacked(Board, 4, Board->CurrentColor) && !IsSquareAttacked(Board, 3, Board->CurrentColor) && !IsSquareAttacked(Board, 2, Board->CurrentColor)
            ) { // Длинная рокировка (О-О-О)
              To = 2;

              AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_CASTLE_QUEEN);
            }
          }
        }
      } // if
    }
  } // for
}

void GenerateCaptureMoves(BoardItem *Board, MoveItem *MoveList, int *GenMoveCount) // Генерируем все возможные взятия (без учёта шаха после хода)
{
  int From;
  int To;

  int *FromPiecePosition; // Указатель на список фигур (откуда)

  if (Board->CurrentColor == WHITE) { // Ход белых
    FromPiecePosition = Board->WhitePieceList;
  }
  else { // Ход чёрных
    FromPiecePosition = Board->BlackPieceList;
  }

  for (int Index = 0; Index < 16; ++Index) { // Перебираем список фигур (откуда)
    From = FromPiecePosition[Index];

    if (From == -1) { // Нет фигуры
      continue;
    }

    if (PIECE(Board->Pieces[From]) == PAWN) { // Пешка
      if (Board->CurrentColor == WHITE) { // Ход белых
        if (COL(From) != 0) {
          To = From - 9;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & BLACK) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        if (COL(From) != 7) {
          To = From - 7;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & BLACK) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        if (ROW(From) == 1) {
          To = From - 8;

          if (Board->Pieces[To] == EMPTY) { // Ход возможен
            AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_PAWN);
          }
        }
      }
      else { // Ход чёрных
        if (COL(From) != 0) {
          To = From + 7;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & WHITE) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        if (COL(From) != 7) {
          To = From + 9;

          if (To == Board->PassantSquare) { // Взятие на проходе возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
          }
          else if (Board->Pieces[To] & WHITE) { // Взятие возможно
            AddMove(Board, MoveList, GenMoveCount, From, To, (MOVE_PAWN | MOVE_CAPTURE));
          }
        }

        if (ROW(From) == 6) {
          To = From + 8;

          if (Board->Pieces[To] == EMPTY) { // Ход возможен
            AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_PAWN);
          }
        }
      }
    }
    else { // Конь, слон, ладья, ферзь или король
      for (int Direction = 0; Direction < 8; ++Direction) {
        if (!MoveDelta[PIECE(Board->Pieces[From])][Direction]) {
          break;
        }

        To = From;

        while (TRUE) {
          To = Board120[Board64[To] + MoveDelta[PIECE(Board->Pieces[From])][Direction]];

          if (To == -1) { // Вышли за пределы доски
            break;
          }

          if (Board->Pieces[To] & Board->CurrentColor) { // Своя фигура
            break;
          }

          if (Board->Pieces[To] != EMPTY) { // Взятие
            AddMove(Board, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            break;
          }

          // Простой ход

          if (PIECE(Board->Pieces[From]) == KING || PIECE(Board->Pieces[From]) == KNIGHT) { // Король или конь
            break;
          }
        } // while
      }
    }
  } // for
}

int PositionRepeat1(BoardItem *Board)
{
  if (Board->FiftyMove < 4) {
    return 0;
  }

  for (int Index = (Board->HalfMoveNumber - 2); Index >= (Board->HalfMoveNumber - Board->FiftyMove); Index -= 2) {
    if (Board->MoveTable[Index].Hash == Board->Hash) {
      return 1;
    }
  }

  return 0;
}

int PositionRepeat2(BoardItem *Board)
{
  int RepeatCounter = 0;

  if (Board->FiftyMove < 4) {
    return 0;
  }

  for (int Index = (Board->HalfMoveNumber - 2); Index >= (Board->HalfMoveNumber - Board->FiftyMove); Index -= 2) {
    if (Board->MoveTable[Index].Hash == Board->Hash) {
      ++RepeatCounter;
    }
  }

  return RepeatCounter;
}

BOOL NonPawnMaterial(BoardItem *Board)
{
  int Square;

	if (Board->CurrentColor == WHITE) {
		for (int Index = 0; Index < 16; ++Index) { // Перебираем список белых фигур
      Square = Board->WhitePieceList[Index];

      if (Square == -1) { // Нет фигуры
        continue;
      }

			if (Board->Pieces[Square] != PAWN && Board->Pieces[Square] != KING) {
				return TRUE;
			}
		}
	}
	else { // BLACK
		for (int Index = 0; Index < 16; ++Index) { // Перебираем список чёрных фигур
      Square = Board->BlackPieceList[Index];

      if (Square == -1) { // Нет фигуры
        continue;
      }

			if (Board->Pieces[Square] != PAWN && Board->Pieces[Square] != KING) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

#ifdef MOVES_SORT_HEURISTIC
void UpdateHeuristic(BoardItem *Board, const int Move, int *QuietMoveList, const int QuietMoveCount, const int Bonus)
{
  int Color = (Board->CurrentColor == WHITE) ? 0 : 1;

  #ifdef LAZY_SMP
  Board->HeuristicTable[Color][PIECE(Board->Pieces[Move >> 6]) - 1][Move & 63] += Bonus - Board->HeuristicTable[Color][PIECE(Board->Pieces[Move >> 6]) - 1][Move & 63] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
  #else
  HeuristicTable[Color][PIECE(Board->Pieces[Move >> 6]) - 1][Move & 63] += Bonus - HeuristicTable[Color][PIECE(Board->Pieces[Move >> 6]) - 1][Move & 63] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
  #endif // LAZY_SMP

  for (int MoveNumber = 0; MoveNumber < QuietMoveCount; ++MoveNumber) {
    #ifdef LAZY_SMP
    Board->HeuristicTable[Color][PIECE(Board->Pieces[QuietMoveList[MoveNumber] >> 6]) - 1][QuietMoveList[MoveNumber] & 63] -= Bonus - Board->HeuristicTable[Color][PIECE(Board->Pieces[QuietMoveList[MoveNumber] >> 6]) - 1][QuietMoveList[MoveNumber] & 63] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
    #else
    HeuristicTable[Color][PIECE(Board->Pieces[QuietMoveList[MoveNumber] >> 6]) - 1][QuietMoveList[MoveNumber] & 63] -= Bonus - HeuristicTable[Color][PIECE(Board->Pieces[QuietMoveList[MoveNumber] >> 6]) - 1][QuietMoveList[MoveNumber] & 63] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
    #endif // LAZY_SMP
  }
}
#endif // MOVES_SORT_HEURISTIC

#ifdef KILLER_MOVE
void UpdateKiller(BoardItem *Board, const int Move, const int Ply)
{
  #ifdef LAZY_SMP
  if (Board->KillerMoveTable[Ply][0] != Move) {
    #ifdef KILLER_MOVE_2
    Board->KillerMoveTable[Ply][1] = Board->KillerMoveTable[Ply][0];
    #endif // KILLER_MOVE_2

    Board->KillerMoveTable[Ply][0] = Move;
  }
  #else
  if (KillerMoveTable[Ply][0] != Move) {
    #ifdef KILLER_MOVE_2
    KillerMoveTable[Ply][1] = KillerMoveTable[Ply][0];
    #endif // KILLER_MOVE_2

    KillerMoveTable[Ply][0] = Move;
  }
  #endif // LAZY_SMP
}
#endif // KILLER_MOVE

#if defined(PVS) || defined(QUIESCENCE_PVS)
void SetPvsMoveSortValue(BoardItem *Board, const int Ply, MoveItem *GenMoveList, const int GenMoveCount)
{
  if (Board->FollowPV) {
    Board->FollowPV = FALSE;

    if (Board->BestMovesRoot[Ply].Move) {
      for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
        if (GenMoveList[MoveNumber].Move == Board->BestMovesRoot[Ply].Move) {
          #ifdef DEBUG_PVS
          printf("-- PVS: Move %s%s\n", BoardName[GenMoveList[MoveNumber].Move >> 6], BoardName[GenMoveList[MoveNumber].Move & 63]);
          #endif // DEBUG_PVS

          GenMoveList[MoveNumber].SortValue = PVS_MOVE_SORT_VALUE;

          Board->FollowPV = TRUE;

          break;
        }
      }
    }
  }
}
#endif // PVS || QUIESCENCE_PVS

#ifdef HASH_MOVE
void SetHashMoveSortValue(MoveItem *GenMoveList, const int GenMoveCount, const int HashMove)
{
  if (HashMove) {
    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
      if (GenMoveList[MoveNumber].Move == HashMove) {
        GenMoveList[MoveNumber].SortValue = HASH_MOVE_SORT_VALUE;

        break;
      }
    }
  }
}
#endif // HASH_MOVE

#ifdef KILLER_MOVE
void SetKillerMoveSortValue(BoardItem *Board, const int Ply, MoveItem *GenMoveList, const int GenMoveCount, const int HashMove)
{
  #ifdef LAZY_SMP
  int KillerMove1 = Board->KillerMoveTable[Ply][0];
  #else
  int KillerMove1 = KillerMoveTable[Ply][0];
  #endif // LAZY_SMP

  if (KillerMove1 && KillerMove1 != HashMove) {
    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
      if (GenMoveList[MoveNumber].Move == KillerMove1) {
        if (!(GenMoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
          GenMoveList[MoveNumber].SortValue = KILLER_MOVE_SORT_VALUE;
        }

        break;
      }
    }
  }
}
#endif // KILLER_MOVE

#ifdef KILLER_MOVE_2
void SetKillerMove2SortValue(BoardItem *Board, const int Ply, MoveItem *GenMoveList, const int GenMoveCount, const int HashMove)
{
  #ifdef LAZY_SMP
  int KillerMove2 = Board->KillerMoveTable[Ply][1];
  #else
  int KillerMove2 = KillerMoveTable[Ply][1];
  #endif // LAZY_SMP

  if (KillerMove2 && KillerMove2 != HashMove) {
    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
      if (GenMoveList[MoveNumber].Move == KillerMove2) {
        if (!(GenMoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
          GenMoveList[MoveNumber].SortValue = KILLER_MOVE_2_SORT_VALUE;
        }

        break;
      }
    }
  }
}
#endif // KILLER_MOVE_2

#if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
void PrepareNextMove(const int StartIndex, MoveItem *GenMoveList, int *GenMoveCount)
{
  int BestMoveIndex = StartIndex;
  int BestMoveScore = GenMoveList[StartIndex].SortValue;

  MoveItem TempMoveItem;

  for (int Index = (StartIndex + 1); Index < *GenMoveCount; ++Index) { // Перебираем все оставшиеся возможные ходы
    if (GenMoveList[Index].SortValue > BestMoveScore) {
      BestMoveIndex = Index;
      BestMoveScore = GenMoveList[Index].SortValue;
    }
  }

  if (StartIndex != BestMoveIndex) {
    TempMoveItem = GenMoveList[StartIndex];
    GenMoveList[StartIndex] = GenMoveList[BestMoveIndex];
    GenMoveList[BestMoveIndex] = TempMoveItem;
  }
}
#endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

#ifdef QUIESCENCE
int QuiescenceSearch(BoardItem *Board, int Alpha, int Beta, const int Depth, const int Ply, MoveItem *BestMoves, const BOOL IsPrincipal, const BOOL InCheck) // Поиск хода и возврат оценки
{
  int GenMoveCount = 0; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  int LegalMoveCount = 0;

  MoveItem TempBestMoves[MAX_PLY]; // Список лучших ходов

  #if defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
  HashItem *HashItemPointer = &HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)]; // Указатель на элемент хеш-таблицы для текущей позиции

  int HashScore = 0;
  int HashStaticScore = 0;
  int HashMove = 0;
  int HashDepth = 0;
  int HashFlag = 0;

  int SaveAlpha = Alpha;

  int QuiescenceHashDepth;
  #endif // QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE

  BOOL GiveCheck;

  int Score;
  int BestScore;
  int StaticScore;

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};

  if (
    !(Board->Nodes & (U64)4095) && // Количество просмотренных позиций кратно 4096
    Clock() >= TimeStop // и время истекло
  ) {
    StopSearch = TRUE;

    return 0;
  }

  if (Ply + 1 > Board->SelDepth) {
    Board->SelDepth = Ply + 1;
  }

  Alpha = MAX(Alpha, -INF + Ply);
  Beta = MIN(Beta, INF - Ply + 1);

  if (Alpha >= Beta) {
    return Alpha;
  }

  if (PositionRepeat1(Board) || Board->FiftyMove >= 100) { // Повтор позиции
    return 0;
  }

  if (Ply >= MAX_PLY) { // Достигли максимальной глубины просмотра
    return Evaluate(Board); // Возвращаем текущую оценку позиции для текущего цвета хода
  }

  #if defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
  LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

  if (InCheck || Depth >= 0) {
    QuiescenceHashDepth = 0;
  }
  else {
    QuiescenceHashDepth = -1;
  }

  if (HashFlag) {
    #ifdef DEBUG_STATISTIC
    ++Board->HashCount;
    #endif // DEBUG_STATISTIC

    #ifdef QUIESCENCE_HASH_SCORE
    if (!IsPrincipal && HashDepth >= QuiescenceHashDepth) { // Глубина узла в хеш-таблице больше или равна текущей глубине
      if (
        (HashFlag == HASH_BETA && HashScore >= Beta) ||
        (HashFlag == HASH_ALPHA && HashScore <= Alpha) ||
        (HashFlag == HASH_EXACT)
      ) {
        return HashScore;
      }
    }
    #endif // QUIESCENCE_HASH_SCORE
  }
  #endif // QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE

  #ifdef QUIESCENCE_CHECK_EXTENSION
  if (InCheck) { // Шах
    BestScore = StaticScore = -INF + Ply;

    GenerateAllMoves(Board, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)
  }
  else { // Не шах
  #endif // QUIESCENCE_CHECK_EXTENSION
    #if defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
    if (HashFlag) {
      BestScore = StaticScore = HashStaticScore;
    }
    else {
    #endif // QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE
      BestScore = StaticScore = Evaluate(Board); // Текущая оценка позиции для текущего цвета хода
    #if defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
    }

    if (
      (HashFlag == HASH_BETA && HashScore > StaticScore) ||
      (HashFlag == HASH_ALPHA && HashScore < StaticScore) ||
      (HashFlag == HASH_EXACT)
    ) {
      BestScore = HashScore;
    }
    #endif // QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE

    if (BestScore >= Beta) { // Отсечение
      #if defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
      if (!HashFlag) {
        SaveHash(HashItemPointer, Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
      }
      #endif // QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE

      return BestScore;
    }

    if (IsPrincipal && BestScore > Alpha) { // Текущая оценка позиции для текущего цвета хода больше Alpha
      Alpha = BestScore; // Обновляем Alpha
    }

    GenerateCaptureMoves(Board, MoveList, &GenMoveCount); // Генерируем все возможные взятия (без учёта шаха после хода)
  #ifdef QUIESCENCE_CHECK_EXTENSION
  }
  #endif // QUIESCENCE_CHECK_EXTENSION

  #ifdef QUIESCENCE_PVS
  SetPvsMoveSortValue(Board, Ply, MoveList, GenMoveCount);
  #endif // QUIESCENCE_PVS

  #ifdef QUIESCENCE_HASH_MOVE
  SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);
  #endif // QUIESCENCE_HASH_MOVE

  for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    PrepareNextMove(MoveNumber, MoveList, &GenMoveCount);
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    #ifdef QUIESCENCE_BAD_CAPTURE_PRUNING
    if (!InCheck) {
      #ifdef MOVES_SORT_SEE
      if (MoveList[MoveNumber].SortValue < -CAPTURE_MOVE_BONUS) {
        break;
      }
      #elif defined(MOVES_SORT_MVV_LVA)
      if ((MoveList[MoveNumber].Type & MOVE_CAPTURE & ~MOVE_PAWN_PROMOTE) && MoveList[MoveNumber].Move != HashMove && CaptureSEE(Board, (MoveList[MoveNumber].Move >> 6), (MoveList[MoveNumber].Move & 63), MoveList[MoveNumber].Type) < 0) {
        continue;
      }
      #endif // MOVES_SORT_SEE/MOVES_SORT_MVV_LVA
    }
    #endif // QUIESCENCE_BAD_CAPTURE_PRUNING

    MakeMove(Board, MoveList[MoveNumber]); // Делаем ход

    #if defined(HASH_PREFETCH) && (defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE))
    _mm_prefetch((char *)&HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE)

    if (IsInCheck(Board, (Board->CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
      UnmakeMove(Board); // Восстанавливаем ход

      continue;
    }

    // Ход возможен (с учётом шаха после хода)

    ++LegalMoveCount;

    ++Board->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(Board, Board->CurrentColor);

    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -QuiescenceSearch(Board, -Beta, -Alpha, Depth - 1, Ply + 1, TempBestMoves, IsPrincipal, GiveCheck); // Получаем оценку текущей позиции (с полным окном)

    UnmakeMove(Board); // Восстанавливаем ход

    if (StopSearch) {
      return 0;
    }

    if (Score > BestScore) {
      BestScore = Score;

      if (BestScore > Alpha) {
        BestMove = MoveList[MoveNumber];

        if (IsPrincipal) {
          SaveBestMoves(BestMoves, BestMove, TempBestMoves); // Сохраняем лучший ход
        }

        if (IsPrincipal && BestScore < Beta) {
          Alpha = BestScore;
        }
        #ifdef ALPHA_BETA_PRUNING
        else {
          #ifdef DEBUG_STATISTIC
          ++Board->CutoffCount;
          #endif // DEBUG_STATISTIC

          #if defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
          SaveHash(HashItemPointer, Board->Hash, QuiescenceHashDepth, Ply, BestScore, StaticScore, BestMove.Move, HASH_BETA); // Сохраняем оценку и лучший ход в хеш-таблице
          #endif // QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE

          return BestScore;
        }
        #endif // ALPHA_BETA_PRUNING
      }
    }
  } // for

  #ifdef QUIESCENCE_CHECK_EXTENSION
  if (InCheck && LegalMoveCount == 0) {
    return -INF + Ply; // Возвращаем оценку текущей позиции: мат (с поправкой на глубину)
  }
  #endif // QUIESCENCE_CHECK_EXTENSION

  #if defined(QUIESCENCE_HASH_SCORE) || defined(QUIESCENCE_HASH_MOVE)
  if (IsPrincipal && BestScore > SaveAlpha) {
    HashFlag = HASH_EXACT;
  }
  else {
    HashFlag = HASH_ALPHA;
  }

  SaveHash(HashItemPointer, Board->Hash, QuiescenceHashDepth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);
  #endif // QUIESCENCE_HASH_SCORE || QUIESCENCE_HASH_MOVE

  return BestScore;
}
#endif // QUIESCENCE

#ifndef ABDADA
int Search(BoardItem *Board, int Alpha, int Beta, int Depth, const int Ply, MoveItem *BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL IsNull) // Поиск хода и возврат оценки
{
  int GenMoveCount = 0; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  int QuietMoveCount = 0;
  int QuietMoveList[MAX_GEN_MOVES];

  #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
  int SEE_Value;
  #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

  int LegalMoveCount = 0;

  MoveItem TempBestMoves[MAX_PLY]; // Список лучших ходов

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  HashItem *HashItemPointer = &HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)]; // Указатель на элемент хеш-таблицы для текущей позиции

  int HashScore = 0;
  int HashStaticScore = 0;
  int HashMove = 0;
  int HashDepth = 0;
  int HashFlag = 0;
  #endif // HASH_SCORE || HASH_MOVE

  BOOL GiveCheck;

  int NewDepth;

  int Score;
  int BestScore;
  int StaticScore;

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};

  #ifdef NULL_MOVE_PRUNING
  int NullMoveReduction;
  #endif // NULL_MOVE_PRUNING

  #if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
  int LateMoveReduction;

  BOOL Improving = FALSE;
  #endif // NEGA_SCOUT && LATE_MOVE_REDUCTION

  #ifdef DEBUG_HASH
  U64 SaveBoardHash = Board->Hash;

  SetBoardHash(Board); // Устанавливаем хеш текущей позиции

  if (Board->Hash != SaveBoardHash) {
    printf("-- Board hash error: SaveBoardHash %llu BoardHash %llu\n", SaveBoardHash, Board->Hash);
  }
  #endif // DEBUG_HASH

  #ifdef QUIESCENCE
  if (Depth <= 0) { // Достигли глубины просмотра
    return QuiescenceSearch(Board, Alpha, Beta, 0, Ply, BestMoves, IsPrincipal, InCheck);
  }
  #endif // QUIESCENCE

  if (
    !(Board->Nodes & (U64)4095) && // Количество просмотренных позиций кратно 4096
    Clock() >= TimeStop // и время истекло
  ) {
    StopSearch = TRUE;

    return 0;
  }

  Alpha = MAX(Alpha, -INF + Ply);
  Beta = MIN(Beta, INF - Ply + 1);

  if (Alpha >= Beta) {
    return Alpha;
  }

  if (Ply > 0 && (PositionRepeat1(Board) || Board->FiftyMove >= 100)) { // Не корень дерева и повтор позиции
    return 0;
  }

  if (Ply >= MAX_PLY) { // Достигли максимальной глубины просмотра
    return Evaluate(Board); // Возвращаем текущую оценку позиции для текущего цвета хода
  }

  #ifndef QUIESCENCE
  if (Depth <= 0) { // Достигли глубины просмотра
    return Evaluate(Board); // Возвращаем текущую оценку позиции для текущего цвета хода
  }
  #endif // !QUIESCENCE

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

  if (HashFlag) {
    #ifdef DEBUG_STATISTIC
    ++Board->HashCount;
    #endif // DEBUG_STATISTIC

    #ifdef HASH_SCORE
    if (!IsPrincipal && HashDepth >= Depth) { // Глубина узла в хеш-таблице больше или равна текущей глубине
      if (
        (HashFlag == HASH_BETA && HashScore >= Beta) ||
        (HashFlag == HASH_ALPHA && HashScore <= Alpha) ||
        (HashFlag == HASH_EXACT)
      ) {
        #if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
        if (HashMove && HashScore >= Beta) {
          if (!Board->Pieces[HashMove & 63] && (PIECE(Board->Pieces[HashMove >> 6]) != PAWN || ((HashMove & 63) != Board->PassantSquare && ROW(HashMove & 63) != 0 && ROW(HashMove & 63) != 7))) {
            #ifdef MOVES_SORT_HEURISTIC
            UpdateHeuristic(Board, HashMove, NULL, 0, BONUS(Depth));
            #endif // MOVES_SORT_HEURISTIC

            #ifdef KILLER_MOVE
            UpdateKiller(Board, HashMove, Ply);
            #endif // KILLER_MOVE
          }
        }
        #endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

        return HashScore;
      } // if
    } // if
    #endif // HASH_SCORE
  } // if
  #endif // HASH_SCORE || HASH_MOVE

  if (InCheck) { // Шах
    Board->StaticScore = BestScore = StaticScore = -INF + Ply;
  }
  else { // Не шах
    #if defined(HASH_SCORE) || defined(HASH_MOVE)
    if (HashFlag) {
      Board->StaticScore = BestScore = StaticScore = HashStaticScore;
    }
    else {
    #endif // HASH_SCORE || HASH_MOVE
      Board->StaticScore = BestScore = StaticScore = Evaluate(Board); // Текущая оценка позиции для текущего цвета хода

    #if defined(HASH_SCORE) || defined(HASH_MOVE)
      SaveHash(HashItemPointer, Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
    }

    if (
      (HashFlag == HASH_BETA && HashScore > StaticScore) ||
      (HashFlag == HASH_ALPHA && HashScore < StaticScore) ||
      (HashFlag == HASH_EXACT)
    ) {
      BestScore = HashScore;
    }
    #endif // HASH_SCORE || HASH_MOVE

    #if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
    if (Ply >= 2 && StaticScore >= Board->MoveTable[Board->HalfMoveNumber - 2].StaticScore) {
      Improving = TRUE;
    }
    #endif // NEGA_SCOUT && LATE_MOVE_REDUCTION
  }

  #ifdef NULL_MOVE_PRUNING
  if (
    !IsPrincipal &&
    !IsNull &&
    !InCheck &&
    Depth > 1 &&
    BestScore >= Beta &&
    NonPawnMaterial(Board)
  ) {
    NullMoveReduction = 3 + (Depth / 6);

    MakeNullMove(Board);

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    ++Board->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(Board, Board->CurrentColor);

    // Поиск с нулевым окном
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -Search(Board, -Beta, -Beta + 1, Depth - 1 - NullMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, TRUE); // Получаем оценку текущей позиции

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
  } // if
  #endif // NULL_MOVE_PRUNING

  #if defined(HASH_MOVE) && defined(IID)
  if (Depth >= 5 && !HashMove) {
    #ifdef DEBUG_IID
    printf("-- IID: Depth %d\n", Depth);
    #endif // DEBUG_IID

    // Поиск с полным окном на сокращенную глубину
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Search(Board, Alpha, Beta, Depth - 4, Ply, TempBestMoves, IsPrincipal, InCheck, IsNull); // Получаем оценку текущей позиции

    if (StopSearch) {
      return 0;
    }

    LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

    #ifdef DEBUG_IID
    if (HashFlag && HashMove) {
      printf("-- IID: Depth %d Move %s%s\n", Depth, BoardName[HashMove >> 6], BoardName[HashMove & 63]);
    }
    #endif // DEBUG_IID
  }
  #endif // HASH_MOVE && IID

  BestScore = -INF + Ply;

  GenerateAllMoves(Board, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)

  #ifdef PVS
  SetPvsMoveSortValue(Board, Ply, MoveList, GenMoveCount);
  #endif // PVS

  #ifdef HASH_MOVE
  SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);
  #endif // HASH_MOVE

  #ifdef KILLER_MOVE
  SetKillerMoveSortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE

  #ifdef KILLER_MOVE_2
  SetKillerMove2SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE_2

  for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
NextMove:
#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    PrepareNextMove(MoveNumber, MoveList, &GenMoveCount);
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
    if ((MoveList[MoveNumber].Type & MOVE_CAPTURE & ~MOVE_PAWN_PROMOTE) && MoveList[MoveNumber].Move != HashMove && MoveList[MoveNumber].SortValue >= CAPTURE_MOVE_BONUS) {
      SEE_Value = CaptureSEE(Board, (MoveList[MoveNumber].Move >> 6), (MoveList[MoveNumber].Move & 63), MoveList[MoveNumber].Type);

      if (SEE_Value < 0) { // Bad capture move
        MoveList[MoveNumber].SortValue = SEE_Value - CAPTURE_MOVE_BONUS;

        goto NextMove;
      }
    }
    #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    MakeMove(Board, MoveList[MoveNumber]); // Делаем ход

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    if (IsInCheck(Board, (Board->CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
      UnmakeMove(Board); // Восстанавливаем ход

      continue;
    }

    // Ход возможен (с учётом шаха после хода)

    ++LegalMoveCount;

    #ifdef LAZY_SMP
    if (omp_get_thread_num() == 0) { // Master thread
    #endif // LAZY_SMP
      if (UciMode && Ply == 0) {
        #pragma omp critical
        {
          printf("info depth %d currmovenumber %d currmove %s%s", Depth, LegalMoveCount, BoardName[MoveList[MoveNumber].Move >> 6], BoardName[MoveList[MoveNumber].Move & 63]);

          if (MoveList[MoveNumber].PromotePiece) {
            printf("%c", PiecesCharBlack[MoveList[MoveNumber].PromotePiece]);
          }

          printf(" nodes %llu\n", Board->Nodes);
        }
      }
    #ifdef LAZY_SMP
    }
    #endif // LAZY_SMP

    ++Board->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(Board, Board->CurrentColor);

    #ifdef CHECK_EXTENSION
    if (GiveCheck) {
      NewDepth = Depth;
    }
    else {
    #endif // CHECK_EXTENSION
      NewDepth = Depth - 1;
    #ifdef CHECK_EXTENSION
    }
    #endif // CHECK_EXTENSION

    #ifdef NEGA_SCOUT
    if (IsPrincipal && LegalMoveCount == 1) { // Первый ход
    #endif // NEGA_SCOUT
      // Поиск с полным окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    #ifdef NEGA_SCOUT
    }
    else { // Оставшиеся ходы
      #ifdef LATE_MOVE_REDUCTION
      LateMoveReduction = 0;

      if (!InCheck && !GiveCheck && Depth >= 3 && MoveList[MoveNumber].SortValue >= -MAX_HEURISTIC_SCORE && MoveList[MoveNumber].SortValue <= MAX_HEURISTIC_SCORE) {
        LateMoveReduction = LateMoveReductionTable[MIN(Depth, 63)][MIN(LegalMoveCount, 63)];

        if (!Improving) {
          ++LateMoveReduction;
        }

        if (LateMoveReduction > 0 && IsPrincipal) {
          --LateMoveReduction;
        }

        if (LateMoveReduction >= NewDepth) {
          LateMoveReduction = NewDepth - 1;
        }
        else if (LateMoveReduction < 0) {
          LateMoveReduction = 0;
        }
      }

      // Поиск с нулевым окном на сокращенную глубину
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth - LateMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции

      if (LateMoveReduction > 0 && Score > Alpha) { // Полученная оценка (с нулевым окном на сокращенную глубину) больше alpha
        // Поиск с нулевым окном
        TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

        Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции
      }
      #else
      // Поиск с нулевым окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции
      #endif // LATE_MOVE_REDUCTION

      if (IsPrincipal && Score > Alpha && (Ply == 0 || Score < Beta)) { // Полученная оценка текущей позиции (с нулевым окном) в диапазоне от alpha до beta
        // Поиск с полным окном
        TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

        Score = -Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
      }
    }
    #endif // NEGA_SCOUT

    UnmakeMove(Board); // Восстанавливаем ход

    if (StopSearch) {
      return 0;
    }

    if (Score > BestScore) {
      BestScore = Score;

      if (BestScore > Alpha) {
        BestMove = MoveList[MoveNumber];

        if (IsPrincipal) {
          SaveBestMoves(BestMoves, BestMove, TempBestMoves); // Сохраняем лучший ход
        }

        if (IsPrincipal && BestScore < Beta) {
          Alpha = BestScore;
        }
        #ifdef ALPHA_BETA_PRUNING
        else {
          #ifdef DEBUG_STATISTIC
          ++Board->CutoffCount;
          #endif // DEBUG_STATISTIC

          break;
        }
        #endif // ALPHA_BETA_PRUNING
      }
    }

    if (MoveList[MoveNumber].Move != BestMove.Move && !(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      QuietMoveList[QuietMoveCount++] = MoveList[MoveNumber].Move;
    }
  } // for

  if (LegalMoveCount == 0) { // Ходов нет
    if (InCheck) { // Шах
      BestScore = -INF + Ply; // Возвращаем оценку текущей позиции: мат (с поправкой на глубину)
    }
    else {
      BestScore = 0; // Возвращаем оценку текущей позиции: пат (ничья)
    }
  }
  #if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
  else if (BestMove.Move) {
    if (!(BestMove.Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      #ifdef MOVES_SORT_HEURISTIC
      UpdateHeuristic(Board, BestMove.Move, QuietMoveList, QuietMoveCount, BONUS(Depth));
      #endif // MOVES_SORT_HEURISTIC

      #ifdef KILLER_MOVE
      UpdateKiller(Board, BestMove.Move, Ply);
      #endif // KILLER_MOVE
    }
  }
  #endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  if (BestScore >= Beta) {
    HashFlag = HASH_BETA;
  }
  else if (IsPrincipal && BestMove.Move) {
    HashFlag = HASH_EXACT;
  }
  else {
    HashFlag = HASH_ALPHA;
  }

  SaveHash(HashItemPointer, Board->Hash, Depth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);
  #endif // HASH_SCORE || HASH_MOVE

  return BestScore;
}
#endif // !ABDADA

#ifdef ABDADA
BOOL IsSearching(int MoveHash)
{
  int Index = (MoveHash & (ABDADA_HASH_TABLE_SIZE - 1));

  for (int Way = 0; Way < ABDADA_HASH_WAYS; ++Way) {
    if (SearchingMovesHashTable[Index][Way] == MoveHash) {
//      printf(".");

      return TRUE;
    }
  }

//  printf("!");

  return FALSE;
}

void StartingSearch(int MoveHash)
{
  int Index = (MoveHash & (ABDADA_HASH_TABLE_SIZE - 1));

  for (int Way = 0; Way < ABDADA_HASH_WAYS; ++Way) {
    if (SearchingMovesHashTable[Index][Way] == 0) {
//      printf("0");

      SearchingMovesHashTable[Index][Way] = MoveHash;

      return;
    }

    if (SearchingMovesHashTable[Index][Way] == MoveHash) {
//      printf("1");

      return;
    }
  }

//  printf("x");

  SearchingMovesHashTable[Index][0] = MoveHash;
}

void StopingSearch(int MoveHash)
{
  int Index = (MoveHash & (ABDADA_HASH_TABLE_SIZE - 1));

  for (int Way = 0; Way < ABDADA_HASH_WAYS; ++Way) {
    if (SearchingMovesHashTable[Index][Way] == MoveHash) {
      SearchingMovesHashTable[Index][Way] = 0;
    }
  }
}

int ABDADA_Search(BoardItem *Board, int Alpha, int Beta, int Depth, const int Ply, MoveItem *BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL IsNull) // Поиск хода и возврат оценки
{
  int GenMoveCount = 0; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  int QuietMoveCount = 0;
  int QuietMoveList[MAX_GEN_MOVES];

  int DeferMoveCount = 0;
  MoveItem DeferMoveList[MAX_GEN_MOVES];
  int DeferMoveNumber[MAX_GEN_MOVES];

  #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
  int SEE_Value;
  #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

  int LegalMoveCount = 0;

  MoveItem TempBestMoves[MAX_PLY]; // Список лучших ходов

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  HashItem *HashItemPointer = &HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)]; // Указатель на элемент хеш-таблицы для текущей позиции

  int HashScore = 0;
  int HashStaticScore = 0;
  int HashMove = 0;
  int HashDepth = 0;
  int HashFlag = 0;
  #endif // HASH_SCORE || HASH_MOVE

  BOOL GiveCheck;

  int NewDepth;

  int Score;
  int BestScore;
  int StaticScore;

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};

  int CurrentMoveHash;

  #ifdef NULL_MOVE_PRUNING
  int NullMoveReduction;
  #endif // NULL_MOVE_PRUNING

  #if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
  int LateMoveReduction;

  BOOL Improving = FALSE;
  #endif // NEGA_SCOUT && LATE_MOVE_REDUCTION

  #ifdef DEBUG_HASH
  U64 SaveBoardHash = Board->Hash;

  SetBoardHash(Board); // Устанавливаем хеш текущей позиции

  if (Board->Hash != SaveBoardHash) {
    printf("-- Board hash error: SaveBoardHash %llu BoardHash %llu\n", SaveBoardHash, Board->Hash);
  }
  #endif // DEBUG_HASH

  #ifdef QUIESCENCE
  if (Depth <= 0) { // Достигли глубины просмотра
    return QuiescenceSearch(Board, Alpha, Beta, 0, Ply, BestMoves, IsPrincipal, InCheck);
  }
  #endif // QUIESCENCE

  if (
    !(Board->Nodes & (U64)4095) && // Количество просмотренных позиций кратно 4096
    Clock() >= TimeStop // и время истекло
  ) {
    StopSearch = TRUE;

    return 0;
  }

  Alpha = MAX(Alpha, -INF + Ply);
  Beta = MIN(Beta, INF - Ply + 1);

  if (Alpha >= Beta) {
    return Alpha;
  }

  if (Ply > 0 && (PositionRepeat1(Board) || Board->FiftyMove >= 100)) { // Не корень дерева и повтор позиции
    return 0;
  }

  if (Ply >= MAX_PLY) { // Достигли максимальной глубины просмотра
    return Evaluate(Board); // Возвращаем текущую оценку позиции для текущего цвета хода
  }

  #ifndef QUIESCENCE
  if (Depth <= 0) { // Достигли глубины просмотра
    return Evaluate(Board); // Возвращаем текущую оценку позиции для текущего цвета хода
  }
  #endif // !QUIESCENCE

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

  if (HashFlag) {
    #ifdef DEBUG_STATISTIC
    ++Board->HashCount;
    #endif // DEBUG_STATISTIC

    #ifdef HASH_SCORE
    if (!IsPrincipal && HashDepth >= Depth) { // Глубина узла в хеш-таблице больше или равна текущей глубине
      if (
        (HashFlag == HASH_BETA && HashScore >= Beta) ||
        (HashFlag == HASH_ALPHA && HashScore <= Alpha) ||
        (HashFlag == HASH_EXACT)
      ) {
        #if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
        if (HashMove && HashScore >= Beta) {
          if (!Board->Pieces[HashMove & 63] && (PIECE(Board->Pieces[HashMove >> 6]) != PAWN || ((HashMove & 63) != Board->PassantSquare && ROW(HashMove & 63) != 0 && ROW(HashMove & 63) != 7))) {
            #ifdef MOVES_SORT_HEURISTIC
            UpdateHeuristic(Board, HashMove, NULL, 0, BONUS(Depth));
            #endif // MOVES_SORT_HEURISTIC

            #ifdef KILLER_MOVE
            UpdateKiller(Board, HashMove, Ply);
            #endif // KILLER_MOVE
          }
        }
        #endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

        return HashScore;
      } // if
    } // if
    #endif // HASH_SCORE
  } // if
  #endif // HASH_SCORE || HASH_MOVE

  if (InCheck) { // Шах
    Board->StaticScore = BestScore = StaticScore = -INF + Ply;
  }
  else { // Не шах
    #if defined(HASH_SCORE) || defined(HASH_MOVE)
    if (HashFlag) {
      Board->StaticScore = BestScore = StaticScore = HashStaticScore;
    }
    else {
    #endif // HASH_SCORE || HASH_MOVE
      Board->StaticScore = BestScore = StaticScore = Evaluate(Board); // Текущая оценка позиции для текущего цвета хода

    #if defined(HASH_SCORE) || defined(HASH_MOVE)
      SaveHash(HashItemPointer, Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
    }

    if (
      (HashFlag == HASH_BETA && HashScore > StaticScore) ||
      (HashFlag == HASH_ALPHA && HashScore < StaticScore) ||
      (HashFlag == HASH_EXACT)
    ) {
      BestScore = HashScore;
    }
    #endif // HASH_SCORE || HASH_MOVE

    #if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
    if (Ply >= 2 && StaticScore >= Board->MoveTable[Board->HalfMoveNumber - 2].StaticScore) {
      Improving = TRUE;
    }
    #endif // NEGA_SCOUT && LATE_MOVE_REDUCTION
  }

  #ifdef NULL_MOVE_PRUNING
  if (
    !IsPrincipal &&
    !IsNull &&
    !InCheck &&
    Depth > 1 &&
    BestScore >= Beta &&
    NonPawnMaterial(Board)
  ) {
    NullMoveReduction = 3 + (Depth / 6);

    MakeNullMove(Board);

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    ++Board->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(Board, Board->CurrentColor);

    // Поиск с нулевым окном
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -ABDADA_Search(Board, -Beta, -Beta + 1, Depth - 1 - NullMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, TRUE); // Получаем оценку текущей позиции

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
  } // if
  #endif // NULL_MOVE_PRUNING

  #if defined(HASH_MOVE) && defined(IID)
  if (Depth >= 5 && !HashMove) {
    #ifdef DEBUG_IID
    printf("-- IID: Depth %d\n", Depth);
    #endif // DEBUG_IID

    // Поиск с полным окном на сокращенную глубину
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    ABDADA_Search(Board, Alpha, Beta, Depth - 4, Ply, TempBestMoves, IsPrincipal, InCheck, IsNull); // Получаем оценку текущей позиции

    if (StopSearch) {
      return 0;
    }

    LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

    #ifdef DEBUG_IID
    if (HashFlag && HashMove) {
      printf("-- IID: Depth %d Move %s%s\n", Depth, BoardName[HashMove >> 6], BoardName[HashMove & 63]);
    }
    #endif // DEBUG_IID
  }
  #endif // HASH_MOVE && IID

  BestScore = -INF + Ply;

  GenerateAllMoves(Board, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)

  #ifdef PVS
  SetPvsMoveSortValue(Board, Ply, MoveList, GenMoveCount);
  #endif // PVS

  #ifdef HASH_MOVE
  SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);
  #endif // HASH_MOVE

  #ifdef KILLER_MOVE
  SetKillerMoveSortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE

  #ifdef KILLER_MOVE_2
  SetKillerMove2SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE_2

  for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
    #ifdef HASH_SCORE
    if (DeferMoveCount > 0 && Depth >= ABDADA_CUTOFF_CHECK_MIN_DEPTH) {
      LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

      if (HashFlag) {
        #ifdef DEBUG_STATISTIC
        ++Board->HashCount;
        #endif // DEBUG_STATISTIC

        if (!IsPrincipal && HashDepth >= Depth) { // Глубина узла в хеш-таблице больше или равна текущей глубине
          if (
            (HashFlag == HASH_BETA && HashScore >= Beta) ||
            (HashFlag == HASH_ALPHA && HashScore <= Alpha) ||
            (HashFlag == HASH_EXACT)
          ) {
            #if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
            if (HashMove && HashScore >= Beta) {
              if (!Board->Pieces[HashMove & 63] && (PIECE(Board->Pieces[HashMove >> 6]) != PAWN || ((HashMove & 63) != Board->PassantSquare && ROW(HashMove & 63) != 0 && ROW(HashMove & 63) != 7))) {
                #ifdef MOVES_SORT_HEURISTIC
                UpdateHeuristic(Board, HashMove, NULL, 0, BONUS(Depth));
                #endif // MOVES_SORT_HEURISTIC

                #ifdef KILLER_MOVE
                UpdateKiller(Board, HashMove, Ply);
                #endif // KILLER_MOVE
              }
            }
            #endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

//            printf("c");

            return HashScore;
          } // if
        } // if
      } // if
    } // if
    #endif // HASH_SCORE

#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
ABDADA_NextMove:
#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    PrepareNextMove(MoveNumber, MoveList, &GenMoveCount);
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
    if ((MoveList[MoveNumber].Type & MOVE_CAPTURE & ~MOVE_PAWN_PROMOTE) && MoveList[MoveNumber].Move != HashMove && MoveList[MoveNumber].SortValue >= CAPTURE_MOVE_BONUS) {
      SEE_Value = CaptureSEE(Board, (MoveList[MoveNumber].Move >> 6), (MoveList[MoveNumber].Move & 63), MoveList[MoveNumber].Type);

      if (SEE_Value < 0) { // Bad capture move
        MoveList[MoveNumber].SortValue = SEE_Value - CAPTURE_MOVE_BONUS;

        goto ABDADA_NextMove;
      }
    }
    #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    MakeMove(Board, MoveList[MoveNumber]); // Делаем ход

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    if (IsInCheck(Board, (Board->CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
      UnmakeMove(Board); // Восстанавливаем ход

      continue;
    }

    // Ход возможен (с учётом шаха после хода)

    ++LegalMoveCount;

    CurrentMoveHash = Board->Hash >> 32;
    CurrentMoveHash ^= (MoveList[MoveNumber].Move * 1664525) + 1013904223; // https://en.wikipedia.org/wiki/Linear_congruential_generator

    if (LegalMoveCount > 1 && Depth >= ABDADA_DEFER_MIN_DEPTH && IsSearching(CurrentMoveHash)) {
      DeferMoveList[DeferMoveCount] = MoveList[MoveNumber];
      DeferMoveNumber[DeferMoveCount] = LegalMoveCount;

      ++DeferMoveCount;

      UnmakeMove(Board); // Восстанавливаем ход

      continue;
    }

    if (omp_get_thread_num() == 0) { // Master thread
      if (UciMode && Ply == 0) {
        #pragma omp critical
        {
          printf("info depth %d currmovenumber %d currmove %s%s", Depth, LegalMoveCount, BoardName[MoveList[MoveNumber].Move >> 6], BoardName[MoveList[MoveNumber].Move & 63]);

          if (MoveList[MoveNumber].PromotePiece) {
            printf("%c", PiecesCharBlack[MoveList[MoveNumber].PromotePiece]);
          }

          printf(" nodes %llu\n", Board->Nodes);
        }
      }
    }

    ++Board->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(Board, Board->CurrentColor);

    #ifdef CHECK_EXTENSION
    if (GiveCheck) {
      NewDepth = Depth;
    }
    else {
    #endif // CHECK_EXTENSION
      NewDepth = Depth - 1;
    #ifdef CHECK_EXTENSION
    }
    #endif // CHECK_EXTENSION

    #ifdef NEGA_SCOUT
    if (IsPrincipal && LegalMoveCount == 1) { // Первый ход
    #endif // NEGA_SCOUT
      // Поиск с полным окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -ABDADA_Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    #ifdef NEGA_SCOUT
    }
    else { // Оставшиеся ходы
      #ifdef LATE_MOVE_REDUCTION
      LateMoveReduction = 0;

      if (!InCheck && !GiveCheck && Depth >= 3 && MoveList[MoveNumber].SortValue >= -MAX_HEURISTIC_SCORE && MoveList[MoveNumber].SortValue <= MAX_HEURISTIC_SCORE) {
        LateMoveReduction = LateMoveReductionTable[MIN(Depth, 63)][MIN(LegalMoveCount, 63)];

        if (!Improving) {
          ++LateMoveReduction;
        }

        if (LateMoveReduction > 0 && IsPrincipal) {
          --LateMoveReduction;
        }

        if (LateMoveReduction >= NewDepth) {
          LateMoveReduction = NewDepth - 1;
        }
        else if (LateMoveReduction < 0) {
          LateMoveReduction = 0;
        }
      }

      if (Depth >= ABDADA_DEFER_MIN_DEPTH) {
        StartingSearch(CurrentMoveHash);
      }

      // Поиск с нулевым окном на сокращенную глубину
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -ABDADA_Search(Board, -Alpha - 1, -Alpha, NewDepth - LateMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции

      if (Depth >= ABDADA_DEFER_MIN_DEPTH) {
        StopingSearch(CurrentMoveHash);
      }

      if (LateMoveReduction > 0 && Score > Alpha) { // Полученная оценка (с нулевым окном на сокращенную глубину) больше alpha
        // Поиск с нулевым окном
        TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

        Score = -ABDADA_Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции
      }
      #else
      if (Depth >= ABDADA_DEFER_MIN_DEPTH) {
        StartingSearch(CurrentMoveHash);
      }

      // Поиск с нулевым окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -ABDADA_Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции

      if (Depth >= ABDADA_DEFER_MIN_DEPTH) {
        StopingSearch(CurrentMoveHash);
      }
      #endif // LATE_MOVE_REDUCTION

      if (IsPrincipal && Score > Alpha && (Ply == 0 || Score < Beta)) { // Полученная оценка текущей позиции (с нулевым окном) в диапазоне от alpha до beta
        // Поиск с полным окном
        TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

        Score = -ABDADA_Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
      }
    }
    #endif // NEGA_SCOUT

    UnmakeMove(Board); // Восстанавливаем ход

    if (StopSearch) {
      return 0;
    }

    if (Score > BestScore) {
      BestScore = Score;

      if (BestScore > Alpha) {
        BestMove = MoveList[MoveNumber];

        if (IsPrincipal) {
          SaveBestMoves(BestMoves, BestMove, TempBestMoves); // Сохраняем лучший ход
        }

        if (IsPrincipal && BestScore < Beta) {
          Alpha = BestScore;
        }
        #ifdef ALPHA_BETA_PRUNING
        else {
          #ifdef DEBUG_STATISTIC
          ++Board->CutoffCount;
          #endif // DEBUG_STATISTIC

          goto ABDADA_Done;
        }
        #endif // ALPHA_BETA_PRUNING
      }
    }

    if (MoveList[MoveNumber].Move != BestMove.Move && !(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      QuietMoveList[QuietMoveCount++] = MoveList[MoveNumber].Move;
    }
  } // for

  for (int MoveNumber = 0; MoveNumber < DeferMoveCount; ++MoveNumber) { // Просматриваем список отложенных ходов
    MakeMove(Board, DeferMoveList[MoveNumber]); // Делаем ход

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    if (omp_get_thread_num() == 0) { // Master thread
      if (UciMode && Ply == 0) {
        #pragma omp critical
        {
          printf("info depth %d currmovenumber %d currmove %s%s", Depth, DeferMoveNumber[MoveNumber], BoardName[DeferMoveList[MoveNumber].Move >> 6], BoardName[DeferMoveList[MoveNumber].Move & 63]);

          if (DeferMoveList[MoveNumber].PromotePiece) {
            printf("%c", PiecesCharBlack[DeferMoveList[MoveNumber].PromotePiece]);
          }

          printf(" nodes %llu\n", Board->Nodes);
        }
      }
    }

    ++Board->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(Board, Board->CurrentColor);

    #ifdef CHECK_EXTENSION
    if (GiveCheck) {
      NewDepth = Depth;
    }
    else {
    #endif // CHECK_EXTENSION
      NewDepth = Depth - 1;
    #ifdef CHECK_EXTENSION
    }
    #endif // CHECK_EXTENSION

    #ifdef LATE_MOVE_REDUCTION
    LateMoveReduction = 0;

    if (!InCheck && !GiveCheck && Depth >= 3 && DeferMoveList[MoveNumber].SortValue >= -MAX_HEURISTIC_SCORE && DeferMoveList[MoveNumber].SortValue <= MAX_HEURISTIC_SCORE) {
      LateMoveReduction = LateMoveReductionTable[MIN(Depth, 63)][MIN(DeferMoveNumber[MoveNumber], 63)];

      if (!Improving) {
        ++LateMoveReduction;
      }

      if (LateMoveReduction > 0 && IsPrincipal) {
        --LateMoveReduction;
      }

      if (LateMoveReduction >= NewDepth) {
        LateMoveReduction = NewDepth - 1;
      }
      else if (LateMoveReduction < 0) {
        LateMoveReduction = 0;
      }
    }

    // Поиск с нулевым окном на сокращенную глубину
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -ABDADA_Search(Board, -Alpha - 1, -Alpha, NewDepth - LateMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции

    if (LateMoveReduction > 0 && Score > Alpha) { // Полученная оценка (с нулевым окном на сокращенную глубину) больше alpha
      // Поиск с нулевым окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -ABDADA_Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    }
    #else
    // Поиск с нулевым окном
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -ABDADA_Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    #endif // LATE_MOVE_REDUCTION

    if (IsPrincipal && Score > Alpha && (Ply == 0 || Score < Beta)) { // Полученная оценка текущей позиции (с нулевым окном) в диапазоне от alpha до beta
      // Поиск с полным окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -ABDADA_Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    }

    UnmakeMove(Board); // Восстанавливаем ход

    if (StopSearch) {
      return 0;
    }

    if (Score > BestScore) {
      BestScore = Score;

      if (BestScore > Alpha) {
        BestMove = DeferMoveList[MoveNumber];

        if (IsPrincipal) {
          SaveBestMoves(BestMoves, BestMove, TempBestMoves); // Сохраняем лучший ход
        }

        if (IsPrincipal && BestScore < Beta) {
          Alpha = BestScore;
        }
        #ifdef ALPHA_BETA_PRUNING
        else {
          #ifdef DEBUG_STATISTIC
          ++Board->CutoffCount;
          #endif // DEBUG_STATISTIC

          break;
        }
        #endif // ALPHA_BETA_PRUNING
      }
    }

    if (DeferMoveList[MoveNumber].Move != BestMove.Move && !(DeferMoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      QuietMoveList[QuietMoveCount++] = DeferMoveList[MoveNumber].Move;
    }
  } // for

ABDADA_Done:

  if (LegalMoveCount == 0) { // Ходов нет
    if (InCheck) { // Шах
      BestScore = -INF + Ply; // Возвращаем оценку текущей позиции: мат (с поправкой на глубину)
    }
    else {
      BestScore = 0; // Возвращаем оценку текущей позиции: пат (ничья)
    }
  }
  #if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
  else if (BestMove.Move) {
    if (!(BestMove.Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      #ifdef MOVES_SORT_HEURISTIC
      UpdateHeuristic(Board, BestMove.Move, QuietMoveList, QuietMoveCount, BONUS(Depth));
      #endif // MOVES_SORT_HEURISTIC

      #ifdef KILLER_MOVE
      UpdateKiller(Board, BestMove.Move, Ply);
      #endif // KILLER_MOVE
    }
  }
  #endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  if (BestScore >= Beta) {
    HashFlag = HASH_BETA;
  }
  else if (IsPrincipal && BestMove.Move) {
    HashFlag = HASH_EXACT;
  }
  else {
    HashFlag = HASH_ALPHA;
  }

  SaveHash(HashItemPointer, Board->Hash, Depth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);
  #endif // HASH_SCORE || HASH_MOVE

  return BestScore;
}
#endif // ABDADA

#ifdef PV_SPLITTING
int PVSplitting_Search(BoardItem *Board, int Alpha, int Beta, const int Depth, const int Ply, MoveItem *BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL IsNull) // Поиск хода и возврат оценки
{
  int GenMoveCount = 0; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  int QuietMoveCount = 0;
  int QuietMoveList[MAX_GEN_MOVES];

  #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
  int SEE_Value;
  #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

  volatile int LegalMoveCount = 0;

  MoveItem TempBestMoves[MAX_PLY]; // Список лучших ходов

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  HashItem *HashItemPointer = &HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)]; // Указатель на элемент хеш-таблицы для текущей позиции

  int HashScore = 0;
  int HashStaticScore = 0;
  int HashMove = 0;
  int HashDepth = 0;
  int HashFlag = 0;
  #endif // HASH_SCORE || HASH_MOVE

  BOOL GiveCheck;

  int NewDepth;

  int Score;
  int BestScore;
  int StaticScore;

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};

  int MoveNumber0;

  BOOL Cutoff = FALSE;

  BoardItem BoardCopy;
  BoardItem *ThreadBoard;

  #if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
  int LateMoveReduction;

  BOOL Improving = FALSE;
  #endif // NEGA_SCOUT && LATE_MOVE_REDUCTION

  #ifdef DEBUG_HASH
  U64 SaveBoardHash = Board->Hash;

  SetBoardHash(Board); // Устанавливаем хеш текущей позиции

  if (Board->Hash != SaveBoardHash) {
    printf("-- Board hash error: SaveBoardHash %llu BoardHash %llu\n", SaveBoardHash, Board->Hash);
  }
  #endif // DEBUG_HASH

  #ifdef QUIESCENCE
  if (Depth <= 0) { // Достигли глубины просмотра
    return QuiescenceSearch(Board, Alpha, Beta, 0, Ply, BestMoves, IsPrincipal, InCheck);
  }
  #endif // QUIESCENCE

  if (
    !(Board->Nodes & (U64)4095) && // Количество просмотренных позиций кратно 4096
    Clock() >= TimeStop // и время истекло
  ) {
    StopSearch = TRUE;

    return 0;
  }

  Alpha = MAX(Alpha, -INF + Ply);
  Beta = MIN(Beta, INF - Ply + 1);

  if (Alpha >= Beta) {
    return Alpha;
  }

  if (Ply > 0 && (PositionRepeat1(Board) || Board->FiftyMove >= 100)) { // Не корень дерева и повтор позиции
    return 0;
  }

  if (Ply >= MAX_PLY) { // Достигли максимальной глубины просмотра
    return Evaluate(Board); // Возвращаем текущую оценку позиции для текущего цвета хода
  }

  #ifndef QUIESCENCE
  if (Depth <= 0) { // Достигли глубины просмотра
    return Evaluate(Board); // Возвращаем текущую оценку позиции для текущего цвета хода
  }
  #endif // !QUIESCENCE

  #ifdef HASH_MOVE
  LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

  #ifdef DEBUG_STATISTIC
  if (HashFlag) {
    ++Board->HashCount;
  }
  #endif // DEBUG_STATISTIC
  #endif // HASH_MOVE

  if (InCheck) { // Шах
    Board->StaticScore = StaticScore = -INF + Ply;
  }
  else { // Не шах
    #if defined(HASH_SCORE) || defined(HASH_MOVE)
    if (HashFlag) {
      Board->StaticScore = StaticScore = HashStaticScore;
    }
    else {
    #endif // HASH_SCORE || HASH_MOVE
      Board->StaticScore = StaticScore = Evaluate(Board); // Текущая оценка позиции для текущего цвета хода

    #if defined(HASH_SCORE) || defined(HASH_MOVE)
      SaveHash(HashItemPointer, Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
    }

    if (
      (HashFlag == HASH_BETA && HashScore > StaticScore) ||
      (HashFlag == HASH_ALPHA && HashScore < StaticScore) ||
      (HashFlag == HASH_EXACT)
    ) {
      BestScore = HashScore;
    }
    #endif // HASH_SCORE || HASH_MOVE

    #if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
    if (Ply >= 2 && StaticScore >= Board->MoveTable[Board->HalfMoveNumber - 2].StaticScore) {
      Improving = TRUE;
    }
    #endif // NEGA_SCOUT && LATE_MOVE_REDUCTION
  }

  #if defined(HASH_MOVE) && defined(IID)
  if (Depth >= 5 && !HashMove) {
    #ifdef DEBUG_IID
    printf("-- IID: Depth %d\n", Depth);
    #endif // DEBUG_IID

    // Поиск с полным окном на сокращенную глубину
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Search(Board, Alpha, Beta, Depth - 4, Ply, TempBestMoves, IsPrincipal, InCheck, IsNull); // Получаем оценку текущей позиции

    if (StopSearch) {
      return 0;
    }

    LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

    #ifdef DEBUG_IID
    if (HashFlag && HashMove) {
      printf("-- IID: Depth %d Move %s%s\n", Depth, BoardName[HashMove >> 6], BoardName[HashMove & 63]);
    }
    #endif // DEBUG_IID
  }
  #endif // HASH_MOVE && IID

  BestScore = -INF + Ply;

  GenerateAllMoves(Board, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)

  #ifdef PVS
  SetPvsMoveSortValue(Board, Ply, MoveList, GenMoveCount);
  #endif // PVS

  #ifdef HASH_MOVE
  SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);
  #endif // HASH_MOVE

  #ifdef KILLER_MOVE
  SetKillerMoveSortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE

  #ifdef KILLER_MOVE_2
  SetKillerMove2SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE_2

  for (MoveNumber0 = 0; MoveNumber0 < GenMoveCount; ++MoveNumber0) { // Просматриваем список возможных ходов
#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
PV_NextMove0:
#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
    PrepareNextMove(MoveNumber0, MoveList, &GenMoveCount);
    #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

    #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
    if ((MoveList[MoveNumber0].Type & MOVE_CAPTURE & ~MOVE_PAWN_PROMOTE) && MoveList[MoveNumber0].Move != HashMove && MoveList[MoveNumber0].SortValue >= CAPTURE_MOVE_BONUS) {
      SEE_Value = CaptureSEE(Board, (MoveList[MoveNumber0].Move >> 6), (MoveList[MoveNumber0].Move & 63), MoveList[MoveNumber0].Type);

      if (SEE_Value < 0) { // Bad capture move
        MoveList[MoveNumber0].SortValue = SEE_Value - CAPTURE_MOVE_BONUS;

        goto PV_NextMove0;
      }
    }
    #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    MakeMove(Board, MoveList[MoveNumber0]); // Делаем ход

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    if (IsInCheck(Board, (Board->CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
      UnmakeMove(Board); // Восстанавливаем ход

      continue;
    }

    // Ход возможен (с учётом шаха после хода)

    ++LegalMoveCount;

    if (UciMode && Ply == 0) {
      printf("info depth %d currmovenumber %d currmove %s%s", Depth, LegalMoveCount, BoardName[MoveList[MoveNumber0].Move >> 6], BoardName[MoveList[MoveNumber0].Move & 63]);

      if (MoveList[MoveNumber0].PromotePiece) {
        printf("%c", PiecesCharBlack[MoveList[MoveNumber0].PromotePiece]);
      }

      printf(" nodes %llu\n", Board->Nodes);
    }

    ++Board->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(Board, Board->CurrentColor);

    #ifdef CHECK_EXTENSION
    if (GiveCheck) {
      NewDepth = Depth;
    }
    else {
    #endif // CHECK_EXTENSION
      NewDepth = Depth - 1;
    #ifdef CHECK_EXTENSION
    }
    #endif // CHECK_EXTENSION

    // Поиск с полным окном
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -PVSplitting_Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции

    UnmakeMove(Board); // Восстанавливаем ход

    if (StopSearch) {
      return 0;
    }

    if (Score > BestScore) {
      BestScore = Score;

      if (BestScore > Alpha) {
        BestMove = MoveList[MoveNumber0];

        SaveBestMoves(BestMoves, BestMove, TempBestMoves); // Сохраняем лучший ход

        if (BestScore < Beta) {
          Alpha = BestScore;
        }
        #ifdef ALPHA_BETA_PRUNING
        else {
          #ifdef DEBUG_STATISTIC
          ++Board->CutoffCount;
          #endif // DEBUG_STATISTIC

          goto PV_Done;
        }
        #endif // ALPHA_BETA_PRUNING
      }
    }

    if (MoveList[MoveNumber0].Move != BestMove.Move && !(MoveList[MoveNumber0].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      QuietMoveList[QuietMoveCount++] = MoveList[MoveNumber0].Move;
    }

    ++MoveNumber0;

    break;
  } // for

  #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
  for (int MoveNumber = MoveNumber0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
PV_NextMove:
#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    PrepareNextMove(MoveNumber, MoveList, &GenMoveCount);

    #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
    if ((MoveList[MoveNumber].Type & MOVE_CAPTURE & ~MOVE_PAWN_PROMOTE) && MoveList[MoveNumber].Move != HashMove && MoveList[MoveNumber].SortValue >= CAPTURE_MOVE_BONUS) {
      SEE_Value = CaptureSEE(Board, (MoveList[MoveNumber].Move >> 6), (MoveList[MoveNumber].Move & 63), MoveList[MoveNumber].Type);

      if (SEE_Value < 0) { // Bad capture move
        MoveList[MoveNumber].SortValue = SEE_Value - CAPTURE_MOVE_BONUS;

        goto PV_NextMove;
      }
    }
    #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST
  }
  #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

  #pragma omp parallel for private(BoardCopy, ThreadBoard, GiveCheck, NewDepth, TempBestMoves, Score) schedule(dynamic, 1)
  for (int MoveNumber = MoveNumber0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
    if (StopSearch || Cutoff) {
      continue;
    }

    BoardCopy = *Board;
    ThreadBoard = &BoardCopy;

    ThreadBoard->Nodes = (U64)0;

    #ifdef DEBUG_STATISTIC
    ThreadBoard->HashCount = (U64)0;
    ThreadBoard->EvaluateCount = (U64)0;
    ThreadBoard->CutoffCount = (U64)0;
    #endif // DEBUG_STATISTIC

    ThreadBoard->SelDepth = 0;

    MakeMove(ThreadBoard, MoveList[MoveNumber]); // Делаем ход

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[ThreadBoard->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    if (IsInCheck(ThreadBoard, (ThreadBoard->CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
      UnmakeMove(ThreadBoard); // Восстанавливаем ход

      continue;
    }

    // Ход возможен (с учётом шаха после хода)

    #pragma omp critical
    {
      ++LegalMoveCount;
    }

    if (UciMode && Ply == 0) {
      #pragma omp critical
      {
        printf("info depth %d currmovenumber %d currmove %s%s", Depth, LegalMoveCount, BoardName[MoveList[MoveNumber].Move >> 6], BoardName[MoveList[MoveNumber].Move & 63]);

        if (MoveList[MoveNumber].PromotePiece) {
          printf("%c", PiecesCharBlack[MoveList[MoveNumber].PromotePiece]);
        }

        printf(" nodes %llu\n", Board->Nodes);
      }
    }

    ++ThreadBoard->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(ThreadBoard, ThreadBoard->CurrentColor);

    #ifdef CHECK_EXTENSION
    if (GiveCheck) {
      NewDepth = Depth;
    }
    else {
    #endif // CHECK_EXTENSION
      NewDepth = Depth - 1;
    #ifdef CHECK_EXTENSION
    }
    #endif // CHECK_EXTENSION

    #ifdef LATE_MOVE_REDUCTION
    LateMoveReduction = 0;

    if (!InCheck && !GiveCheck && Depth >= 3 && MoveList[MoveNumber].SortValue >= -MAX_HEURISTIC_SCORE && MoveList[MoveNumber].SortValue <= MAX_HEURISTIC_SCORE) {
      LateMoveReduction = LateMoveReductionTable[MIN(Depth, 63)][MIN(LegalMoveCount, 63)];

      if (!Improving) {
        ++LateMoveReduction;
      }

      if (LateMoveReduction > 0) {
        --LateMoveReduction;
      }

      if (LateMoveReduction >= NewDepth) {
        LateMoveReduction = NewDepth - 1;
      }
      else if (LateMoveReduction < 0) {
        LateMoveReduction = 0;
      }
    }

    // Поиск с нулевым окном на сокращенную глубину
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -Search(ThreadBoard, -Alpha - 1, -Alpha, NewDepth - LateMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции

    if (LateMoveReduction > 0 && Score > Alpha) { // Полученная оценка (с нулевым окном на сокращенную глубину) больше alpha
      // Поиск с нулевым окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -Search(ThreadBoard, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    }
    #else
    // Поиск с нулевым окном
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Score = -Search(ThreadBoard, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    #endif // LATE_MOVE_REDUCTION

    if (Score > Alpha && (Ply == 0 || Score < Beta)) { // Полученная оценка текущей позиции (с нулевым окном) в диапазоне от alpha до beta
      // Поиск с полным окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -Search(ThreadBoard, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    }

    UnmakeMove(ThreadBoard); // Восстанавливаем ход

    #pragma omp critical
    {
      Board->Nodes += ThreadBoard->Nodes;

      #ifdef DEBUG_STATISTIC
      Board->HashCount += ThreadBoard->HashCount;
      Board->EvaluateCount += ThreadBoard->EvaluateCount;
      Board->CutoffCount += ThreadBoard->CutoffCount;
      #endif // DEBUG_STATISTIC

      if (ThreadBoard->SelDepth > Board->SelDepth) {
        Board->SelDepth = ThreadBoard->SelDepth;
      }
    }

    if (StopSearch) {
      continue;
    }

    #pragma omp critical
    if (Score > BestScore) {
      BestScore = Score;

      if (BestScore > Alpha) {
        BestMove = MoveList[MoveNumber];

        SaveBestMoves(BestMoves, BestMove, TempBestMoves); // Сохраняем лучший ход

        if (BestScore < Beta) {
          Alpha = BestScore;
        }
        #ifdef ALPHA_BETA_PRUNING
        else {
          #ifdef DEBUG_STATISTIC
          ++Board->CutoffCount;
          #endif // DEBUG_STATISTIC

          Cutoff = TRUE;
        }
        #endif // ALPHA_BETA_PRUNING
      }
    }

    if (!Cutoff && MoveList[MoveNumber].Move != BestMove.Move && !(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      QuietMoveList[QuietMoveCount++] = MoveList[MoveNumber].Move;
    }
  } // for

  if (StopSearch) {
    return 0;
  }

PV_Done:

  if (LegalMoveCount == 0) { // Ходов нет
    if (InCheck) { // Шах
      BestScore = -INF + Ply; // Возвращаем оценку текущей позиции: мат (с поправкой на глубину)
    }
    else {
      BestScore = 0; // Возвращаем оценку текущей позиции: пат (ничья)
    }
  }
  #if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
  else if (BestMove.Move) {
    if (!(BestMove.Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      #ifdef MOVES_SORT_HEURISTIC
      UpdateHeuristic(Board, BestMove.Move, QuietMoveList, QuietMoveCount, BONUS(Depth));
      #endif // MOVES_SORT_HEURISTIC

      #ifdef KILLER_MOVE
      UpdateKiller(Board, BestMove.Move, Ply);
      #endif // KILLER_MOVE
    }
  }
  #endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  if (BestScore >= Beta) {
    HashFlag = HASH_BETA;
  }
  else if (BestMove.Move) {
    HashFlag = HASH_EXACT;
  }
  else {
    HashFlag = HASH_ALPHA;
  }

  SaveHash(HashItemPointer, Board->Hash, Depth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);
  #endif // HASH_SCORE || HASH_MOVE

  return BestScore;
}
#endif // PV_SPLITTING

#ifdef ROOT_SPLITTING
int RootSplitting_Search(BoardItem *Board, int Alpha, int Beta, const int Depth, const int Ply, MoveItem *BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL IsNull) // Поиск хода и возврат оценки
{
  int GenMoveCount = 0; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  int QuietMoveCount = 0;
  int QuietMoveList[MAX_GEN_MOVES];

  #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
  int SEE_Value;
  #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

  volatile int LegalMoveCount = 0;

  MoveItem TempBestMoves[MAX_PLY]; // Список лучших ходов

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  HashItem *HashItemPointer = &HashTable[Board->Hash & (HASH_TABLE_SIZE - 1)]; // Указатель на элемент хеш-таблицы для текущей позиции

  int HashScore = 0;
  int HashStaticScore = 0;
  int HashMove = 0;
  int HashDepth = 0;
  int HashFlag = 0;
  #endif // HASH_SCORE || HASH_MOVE

  BOOL GiveCheck;

  int NewDepth;

  int Score;
  int BestScore;
  int StaticScore;

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};

  BOOL Cutoff = FALSE;

  BoardItem BoardCopy;
  BoardItem *ThreadBoard;

  #ifdef DEBUG_HASH
  U64 SaveBoardHash = Board->Hash;

  SetBoardHash(Board); // Устанавливаем хеш текущей позиции

  if (Board->Hash != SaveBoardHash) {
    printf("-- Board hash error: SaveBoardHash %llu BoardHash %llu\n", SaveBoardHash, Board->Hash);
  }
  #endif // DEBUG_HASH

  Alpha = MAX(Alpha, -INF + Ply);
  Beta = MIN(Beta, INF - Ply + 1);

  if (Alpha >= Beta) {
    return Alpha;
  }

  #ifdef HASH_MOVE
  LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

  #ifdef DEBUG_STATISTIC
  if (HashFlag) {
    ++Board->HashCount;
  }
  #endif // DEBUG_STATISTIC
  #endif // HASH_MOVE

  if (InCheck) { // Шах
    Board->StaticScore = StaticScore = -INF + Ply;
  }
  else { // Не шах
    #if defined(HASH_SCORE) || defined(HASH_MOVE)
    if (HashFlag) {
      Board->StaticScore = StaticScore = HashStaticScore;
    }
    else {
    #endif // HASH_SCORE || HASH_MOVE
      Board->StaticScore = StaticScore = Evaluate(Board); // Текущая оценка позиции для текущего цвета хода

    #if defined(HASH_SCORE) || defined(HASH_MOVE)
      SaveHash(HashItemPointer, Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
    }

    if (
      (HashFlag == HASH_BETA && HashScore > StaticScore) ||
      (HashFlag == HASH_ALPHA && HashScore < StaticScore) ||
      (HashFlag == HASH_EXACT)
    ) {
      BestScore = HashScore;
    }
    #endif // HASH_SCORE || HASH_MOVE
  }

  #if defined(HASH_MOVE) && defined(IID)
  if (Depth >= 5 && !HashMove) {
    #ifdef DEBUG_IID
    printf("-- IID: Depth %d\n", Depth);
    #endif // DEBUG_IID

    // Поиск с полным окном на сокращенную глубину
    TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

    Search(Board, Alpha, Beta, Depth - 4, Ply, TempBestMoves, IsPrincipal, InCheck, IsNull); // Получаем оценку текущей позиции

    if (StopSearch) {
      return 0;
    }

    LoadHash(HashItemPointer, Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

    #ifdef DEBUG_IID
    if (HashFlag && HashMove) {
      printf("-- IID: Depth %d Move %s%s\n", Depth, BoardName[HashMove >> 6], BoardName[HashMove & 63]);
    }
    #endif // DEBUG_IID
  }
  #endif // HASH_MOVE && IID

  BestScore = -INF + Ply;

  GenerateAllMoves(Board, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)

  #ifdef PVS
  SetPvsMoveSortValue(Board, Ply, MoveList, GenMoveCount);
  #endif // PVS

  #ifdef HASH_MOVE
  SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);
  #endif // HASH_MOVE

  #ifdef KILLER_MOVE
  SetKillerMoveSortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE

  #ifdef KILLER_MOVE_2
  SetKillerMove2SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
  #endif // KILLER_MOVE_2

  #if defined(MOVES_SORT_SEE) || defined(MOVES_SORT_MVV_LVA) || defined(MOVES_SORT_HEURISTIC) || defined(MOVES_SORT_SQUARE_SCORE)
  for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
#if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
Root_NextMove:
#endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST

    PrepareNextMove(MoveNumber, MoveList, &GenMoveCount);

    #if defined(MOVES_SORT_MVV_LVA) && defined(BAD_CAPTURE_LAST)
    if ((MoveList[MoveNumber].Type & MOVE_CAPTURE & ~MOVE_PAWN_PROMOTE) && MoveList[MoveNumber].Move != HashMove && MoveList[MoveNumber].SortValue >= CAPTURE_MOVE_BONUS) {
      SEE_Value = CaptureSEE(Board, (MoveList[MoveNumber].Move >> 6), (MoveList[MoveNumber].Move & 63), MoveList[MoveNumber].Type);

      if (SEE_Value < 0) { // Bad capture move
        MoveList[MoveNumber].SortValue = SEE_Value - CAPTURE_MOVE_BONUS;

        goto Root_NextMove;
      }
    }
    #endif // MOVES_SORT_MVV_LVA && BAD_CAPTURE_LAST
  }
  #endif // MOVES_SORT_SEE || MOVES_SORT_MVV_LVA || MOVES_SORT_HEURISTIC || MOVES_SORT_SQUARE_SCORE

  #pragma omp parallel for private(BoardCopy, ThreadBoard, GiveCheck, NewDepth, TempBestMoves, Score) schedule(dynamic, 1)
  for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
    if (StopSearch || Cutoff) {
      continue;
    }

    BoardCopy = *Board;
    ThreadBoard = &BoardCopy;

    ThreadBoard->Nodes = (U64)0;

    #ifdef DEBUG_STATISTIC
    ThreadBoard->HashCount = (U64)0;
    ThreadBoard->EvaluateCount = (U64)0;
    ThreadBoard->CutoffCount = (U64)0;
    #endif // DEBUG_STATISTIC

    ThreadBoard->SelDepth = 0;

    MakeMove(ThreadBoard, MoveList[MoveNumber]); // Делаем ход

    #if defined(HASH_PREFETCH) && (defined(HASH_SCORE) || defined(HASH_MOVE))
    _mm_prefetch((char *)&HashTable[ThreadBoard->Hash & (HASH_TABLE_SIZE - 1)], _MM_HINT_T0);
    #endif // HASH_PREFETCH && (HASH_SCORE || HASH_MOVE)

    if (IsInCheck(ThreadBoard, (ThreadBoard->CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
      UnmakeMove(ThreadBoard); // Восстанавливаем ход

      continue;
    }

    // Ход возможен (с учётом шаха после хода)

    #pragma omp critical
    {
      ++LegalMoveCount;
    }

    if (UciMode) {
      #pragma omp critical
      {
        printf("info depth %d currmovenumber %d currmove %s%s", Depth, LegalMoveCount, BoardName[MoveList[MoveNumber].Move >> 6], BoardName[MoveList[MoveNumber].Move & 63]);

        if (MoveList[MoveNumber].PromotePiece) {
          printf("%c", PiecesCharBlack[MoveList[MoveNumber].PromotePiece]);
        }

        printf(" nodes %llu\n", Board->Nodes);
      }
    }

    ++ThreadBoard->Nodes; // Увеличиваем число просмотренных позиций

    GiveCheck = IsInCheck(ThreadBoard, ThreadBoard->CurrentColor);

    #ifdef CHECK_EXTENSION
    if (GiveCheck) {
      NewDepth = Depth;
    }
    else {
    #endif // CHECK_EXTENSION
      NewDepth = Depth - 1;
    #ifdef CHECK_EXTENSION
    }
    #endif // CHECK_EXTENSION

    #ifdef NEGA_SCOUT
    if (LegalMoveCount == 1) { // Первый ход
    #endif // NEGA_SCOUT
      // Поиск с полным окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -Search(ThreadBoard, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
    #ifdef NEGA_SCOUT
    }
    else { // Оставшиеся ходы
      // Поиск с нулевым окном
      TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      Score = -Search(ThreadBoard, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, IsNull); // Получаем оценку текущей позиции

      if (Score > Alpha) { // Полученная оценка текущей позиции (с нулевым окном) в диапазоне от alpha до beta
        // Поиск с полным окном
        TempBestMoves[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

        Score = -Search(ThreadBoard, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, IsNull); // Получаем оценку текущей позиции
      }
    }
    #endif // NEGA_SCOUT

    UnmakeMove(ThreadBoard); // Восстанавливаем ход

    #pragma omp critical
    {
      Board->Nodes += ThreadBoard->Nodes;

      #ifdef DEBUG_STATISTIC
      Board->HashCount += ThreadBoard->HashCount;
      Board->EvaluateCount += ThreadBoard->EvaluateCount;
      Board->CutoffCount += ThreadBoard->CutoffCount;
      #endif // DEBUG_STATISTIC

      if (ThreadBoard->SelDepth > Board->SelDepth) {
        Board->SelDepth = ThreadBoard->SelDepth;
      }
    }

    if (StopSearch) {
      continue;
    }

    #pragma omp critical
    if (Score > BestScore) {
      BestScore = Score;

      if (BestScore > Alpha) {
        BestMove = MoveList[MoveNumber];

        SaveBestMoves(BestMoves, BestMove, TempBestMoves); // Сохраняем лучший ход

        if (BestScore < Beta) {
          Alpha = BestScore;
        }
        #ifdef ALPHA_BETA_PRUNING
        else {
          #ifdef DEBUG_STATISTIC
          ++Board->CutoffCount;
          #endif // DEBUG_STATISTIC

          Cutoff = TRUE;
        }
        #endif // ALPHA_BETA_PRUNING
      }
    }

    if (!Cutoff && MoveList[MoveNumber].Move != BestMove.Move && !(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      QuietMoveList[QuietMoveCount++] = MoveList[MoveNumber].Move;
    }
  } // for

  if (StopSearch) {
    return 0;
  }

  if (LegalMoveCount == 0) { // Ходов нет
    if (InCheck) { // Шах
      BestScore = -INF + Ply; // Возвращаем оценку текущей позиции: мат (с поправкой на глубину)
    }
    else {
      BestScore = 0; // Возвращаем оценку текущей позиции: пат (ничья)
    }
  }
  #if defined(MOVES_SORT_HEURISTIC) || defined(KILLER_MOVE)
  else if (BestMove.Move) {
    if (!(BestMove.Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) {
      #ifdef MOVES_SORT_HEURISTIC
      UpdateHeuristic(Board, BestMove.Move, QuietMoveList, QuietMoveCount, BONUS(Depth));
      #endif // MOVES_SORT_HEURISTIC

      #ifdef KILLER_MOVE
      UpdateKiller(Board, BestMove.Move, Ply);
      #endif // KILLER_MOVE
    }
  }
  #endif // MOVES_SORT_HEURISTIC || KILLER_MOVE

  #if defined(HASH_SCORE) || defined(HASH_MOVE)
  if (BestScore >= Beta) {
    HashFlag = HASH_BETA;
  }
  else if (BestMove.Move) {
    HashFlag = HASH_EXACT;
  }
  else {
    HashFlag = HASH_ALPHA;
  }

  SaveHash(HashItemPointer, Board->Hash, Depth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);
  #endif // HASH_SCORE || HASH_MOVE

  return BestScore;
}
#endif // ROOT_SPLITTING

BOOL PrintResult(const BOOL InCheck, const MoveItem BestMove, const MoveItem PonderMove, const int BestScore)
{
  if (UciMode) {
    printf("info nodes %llu", CurrentBoard.Nodes);

    if (TotalTime > 0) {
      printf(" nps %d", (int)(1000 * CurrentBoard.Nodes / TotalTime));
    }

    printf(" hashfull %d\n", FullHash());

    if (BestMove.Move) { // Есть ход
      printf("bestmove %s%s", BoardName[BestMove.Move >> 6], BoardName[BestMove.Move & 63]);

      if (BestMove.PromotePiece) {
        printf("%c", PiecesCharBlack[BestMove.PromotePiece]);
      }

      if (PonderMove.Move) {
        printf(" ponder %s%s", BoardName[PonderMove.Move >> 6], BoardName[PonderMove.Move & 63]);

        if (PonderMove.PromotePiece) {
          printf("%c", PiecesCharBlack[PonderMove.PromotePiece]);
        }
      }
    }
    else { // Нет хода
      printf("bestmove (none)");
    }

    printf("\n");

    return FALSE;
  }
  else {
    if (!BestMove.Move) { // Нет хода
      if (InCheck) { // Мат
        printf("Checkmate!\n");

        if (CurrentBoard.CurrentColor == WHITE) { // Ход белых
          printf("{0-1} Black wins!\n\n");
        }
        else { // Ход чёрных
          printf("{1-0} White wins!\n\n");
        }
      }
      else { // Пат (ничья)
        printf("{1/2-1/2} Stalemate!\n\n");
      }

      return FALSE; // Возвращаем флаг: возврат в основное меню
    }

    MakeMove(&CurrentBoard, BestMove); // Делаем ход

    PrintBoard(&CurrentBoard); // Выводим доску и фигуры

    if (CurrentBoard.CurrentColor == WHITE) { // Ход белых
      printf("%d: ... %s%s", (int)(CurrentBoard.HalfMoveNumber / 2), BoardName[BestMove.Move >> 6], BoardName[BestMove.Move & 63]);
    }
    else { // Ход чёрных
      printf("%d: %s%s", (int)(CurrentBoard.HalfMoveNumber / 2) + 1, BoardName[BestMove.Move >> 6], BoardName[BestMove.Move & 63]);
    }

    if (BestMove.PromotePiece) {
      printf("%c", PiecesCharBlack[BestMove.PromotePiece]);
    }

    printf("\n\n");

    printf("Score %.2f Nodes %llu Hashfull %.2f%% Time %.2f", ((double)BestScore / PAWN_SCORE), CurrentBoard.Nodes, ((double)FullHash() / 10), ((double)TotalTime / 1000));

    if (TotalTime > 0) {
      printf(" NPS %d", (int)(1000 * CurrentBoard.Nodes / TotalTime));
    }

    printf("\n\n");

    #ifdef DEBUG_STATISTIC
    printf("Hash count %llu Evaluate count %llu Cutoff count %llu\n\n", CurrentBoard.HashCount, CurrentBoard.EvaluateCount, CurrentBoard.CutoffCount);
    #endif // DEBUG_STATISTIC

    if (BestScore >= INF - 1 || BestScore <= -INF + 1) { // Победа
      printf("Checkmate!\n");

      if (CurrentBoard.CurrentColor == WHITE) { // Ход белых
        printf("{0-1} Black wins!\n\n");
      }
      else { // Ход чёрных
        printf("{1-0} White wins!\n\n");
      }

      return FALSE; // Возвращаем флаг: возврат в основное меню
    }

    if (PositionRepeat2(&CurrentBoard) == 2) {
      printf("{1/2-1/2} Draw by repetition!\n\n");

      return FALSE; // Возвращаем флаг: возврат в основное меню
    }

    if (CurrentBoard.FiftyMove >= 100) {
      printf("{1/2-1/2} Draw by fifty move rule!\n\n");

      return FALSE; // Возвращаем флаг: возврат в основное меню
    }

    return TRUE; // Возвращаем флаг: сделан ход
  }
}

#if !defined(ABDADA) && !defined(LAZY_SMP)
BOOL ComputerMove(void) // Ход компьютера
{
  int Score; // Текущая оцека

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};
  MoveItem PonderMove = (MoveItem){0, 0, 0, 0};

  int BestScore = -INF;

  BOOL InCheck = IsInCheck(&CurrentBoard, CurrentBoard.CurrentColor);

  CurrentBoard.Nodes = (U64)0; // Количество просмотренных позиций

  #ifdef DEBUG_STATISTIC
  CurrentBoard.HashCount = (U64)0; // Количество оценок/ходов полученных их хеша
  CurrentBoard.EvaluateCount = (U64)0;
  CurrentBoard.CutoffCount = (U64)0;
  #endif // DEBUG_STATISTIC

  CurrentBoard.SelDepth = 0;

  memset(CurrentBoard.BestMovesRoot, 0, sizeof(CurrentBoard.BestMovesRoot));

  #ifdef MOVES_SORT_HEURISTIC
  for (int Color = 0; Color < 2; ++Color) {
    for (int Piece = 0; Piece < 6; ++Piece) {
      for (int Square = 0; Square < 64; ++Square) {
        HeuristicTable[Color][Piece][Square] = 0;
      }
    }
  }
  #endif // MOVES_SORT_HEURISTIC

  #ifdef KILLER_MOVE
  for (int Depth = 0; Depth < MAX_PLY; ++Depth) {
    KillerMoveTable[Depth][0] = 0;

    #ifdef KILLER_MOVE_2
    KillerMoveTable[Depth][1] = 0;
    #endif // KILLER_MOVE_2
  }
  #endif // KILLER_MOVE

  TimeStart = Clock(); // Время начала поиска хода
  TimeStop = TimeStart + MaxTimeForMove;

  StopSearch = FALSE;

  // Итеративный поиск
  for (int Depth = 1; Depth <= MaxDepth; ++Depth) {
    #if defined(PVS) || defined(QUIESCENCE_PVS)
    CurrentBoard.FollowPV = TRUE;
    #endif // PVS || QUIESCENCE_PVS

    #ifdef PV_SPLITTING
    Score = PVSplitting_Search(&CurrentBoard, -INF, INF, Depth, 0, CurrentBoard.BestMovesRoot, TRUE, InCheck, FALSE); // Ищем ход и получаем оценку позиции
    #elif defined(ROOT_SPLITTING)
    Score = RootSplitting_Search(&CurrentBoard, -INF, INF, Depth, 0, CurrentBoard.BestMovesRoot, TRUE, InCheck, FALSE); // Ищем ход и получаем оценку позиции
    #else // NONE
    Score = Search(&CurrentBoard, -INF, INF, Depth, 0, CurrentBoard.BestMovesRoot, TRUE, InCheck, FALSE); // Ищем ход и получаем оценку позиции
    #endif // PV_SPLITTING/ROOT_SPLITTING/NONE

    if (StopSearch) {
      break;
    }

    PrintBestMoves(&CurrentBoard, Depth, CurrentBoard.BestMovesRoot, Score); // Выводим список лучших ходов

    BestMove = CurrentBoard.BestMovesRoot[0];
    PonderMove = CurrentBoard.BestMovesRoot[1];

    BestScore = Score;

    if (!BestMove.Move || BestScore <= -INF + Depth || BestScore >= INF - Depth) { // Мат или пат (ничья)
      break;
    }
  } // for

  TimeStop = Clock(); // Время окончания поиска хода
  TotalTime = TimeStop - TimeStart; // Суммарное время поиска хода

  return PrintResult(InCheck, BestMove, PonderMove, BestScore);
}
#elif defined(ABDADA)
BOOL ComputerMove(void) // Ход компьютера
{
  int Score; // Текущая оцека
  int ThreadScore;

  BoardItem ThreadBoard;

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};
  MoveItem PonderMove = (MoveItem){0, 0, 0, 0};

  int BestScore = -INF;

  BOOL InCheck = IsInCheck(&CurrentBoard, CurrentBoard.CurrentColor);

  #if defined(PVS) || defined(QUIESCENCE_PVS)
  CurrentBoard.FollowPV = TRUE;
  #endif // PVS || QUIESCENCE_PVS

  CurrentBoard.Nodes = (U64)0; // Количество просмотренных позиций

  #ifdef DEBUG_STATISTIC
  CurrentBoard.HashCount = (U64)0; // Количество оценок/ходов полученных их хеша
  CurrentBoard.EvaluateCount = (U64)0;
  CurrentBoard.CutoffCount = (U64)0;
  #endif // DEBUG_STATISTIC

  CurrentBoard.SelDepth = 0;

  memset(CurrentBoard.BestMovesRoot, 0, sizeof(CurrentBoard.BestMovesRoot));

  #ifdef MOVES_SORT_HEURISTIC
  for (int Color = 0; Color < 2; ++Color) {
    for (int Piece = 0; Piece < 6; ++Piece) {
      for (int Square = 0; Square < 64; ++Square) {
        HeuristicTable[Color][Piece][Square] = 0;
      }
    }
  }
  #endif // MOVES_SORT_HEURISTIC

  #ifdef KILLER_MOVE
  for (int Depth = 0; Depth < MAX_PLY; ++Depth) {
    KillerMoveTable[Depth][0] = 0;

    #ifdef KILLER_MOVE_2
    KillerMoveTable[Depth][1] = 0;
    #endif // KILLER_MOVE_2
  }
  #endif // KILLER_MOVE

  TimeStart = Clock(); // Время начала поиска хода
  TimeStop = TimeStart + MaxTimeForMove;

  StopSearch = FALSE;

  // Итеративный поиск
  for (int Depth = 1; Depth <= MaxDepth; ++Depth) {
    Score = -INF;

    #pragma omp parallel private(ThreadBoard, ThreadScore)
    {
      ThreadBoard = CurrentBoard;

      ThreadBoard.Nodes = (U64)0; // Количество просмотренных позиций

      #ifdef DEBUG_STATISTIC
      ThreadBoard.HashCount = (U64)0; // Количество оценок/ходов полученных их хеша
      ThreadBoard.EvaluateCount = (U64)0;
      ThreadBoard.CutoffCount = (U64)0;
      #endif // DEBUG_STATISTIC

      ThreadBoard.SelDepth = 0;

      ThreadBoard.BestMovesRoot[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      ThreadScore = ABDADA_Search(&ThreadBoard, -INF, INF, Depth, 0, ThreadBoard.BestMovesRoot, TRUE, InCheck, FALSE); // Ищем ход и получаем оценку позиции

      #pragma omp critical
      {
        CurrentBoard.Nodes += ThreadBoard.Nodes;

        #ifdef DEBUG_STATISTIC
        CurrentBoard.HashCount += ThreadBoard.HashCount;
        CurrentBoard.EvaluateCount += ThreadBoard.EvaluateCount;
        CurrentBoard.CutoffCount += ThreadBoard.CutoffCount;
        #endif // DEBUG_STATISTIC

        if (ThreadBoard.SelDepth > CurrentBoard.SelDepth) {
          CurrentBoard.SelDepth = ThreadBoard.SelDepth;
        }

        if (ThreadScore > Score) {
          Score = ThreadScore;

          for (int MoveNumber = 0; MoveNumber < MAX_PLY; ++MoveNumber) {
            CurrentBoard.BestMovesRoot[MoveNumber] = ThreadBoard.BestMovesRoot[MoveNumber];
          }
        }
      }
    } // pragma omp parallel

    if (StopSearch) {
      break;
    }

    PrintBestMoves(&CurrentBoard, Depth, CurrentBoard.BestMovesRoot, Score); // Выводим список лучших ходов

    BestMove = CurrentBoard.BestMovesRoot[0];
    PonderMove = CurrentBoard.BestMovesRoot[1];

    BestScore = Score;

    if (!BestMove.Move || BestScore <= -INF + Depth || BestScore >= INF - Depth) { // Мат или пат (ничья)
      break;
    }
  } // for

  TimeStop = Clock(); // Время окончания поиска хода
  TotalTime = TimeStop - TimeStart; // Суммарное время поиска хода

  return PrintResult(InCheck, BestMove, PonderMove, BestScore);
}
#else // LAZY_SMP
BOOL ComputerMove(void) // Ход компьютера
{
  int ThreadScore;

  BoardItem ThreadBoard;

  MoveItem BestMove = (MoveItem){0, 0, 0, 0};
  MoveItem PonderMove = (MoveItem){0, 0, 0, 0};

  int BestScore = -INF;

  BOOL InCheck = IsInCheck(&CurrentBoard, CurrentBoard.CurrentColor);

  int SearchDepthCount;

  volatile int ThreadDepth[MAX_PLY];

  #if defined(PVS) || defined(QUIESCENCE_PVS)
  CurrentBoard.FollowPV = TRUE;
  #endif // PVS || QUIESCENCE_PVS

  CurrentBoard.Nodes = (U64)0; // Количество просмотренных позиций

  #ifdef DEBUG_STATISTIC
  CurrentBoard.HashCount = (U64)0; // Количество оценок/ходов полученных их хеша
  CurrentBoard.EvaluateCount = (U64)0;
  CurrentBoard.CutoffCount = (U64)0;
  #endif // DEBUG_STATISTIC

  CurrentBoard.SelDepth = 0;

  memset(CurrentBoard.BestMovesRoot, 0, sizeof(CurrentBoard.BestMovesRoot));

  #ifdef MOVES_SORT_HEURISTIC
  memset(CurrentBoard.HeuristicTable, 0, sizeof(CurrentBoard.HeuristicTable)); // Инициализируем хеш-таблицу
  #endif // MOVES_SORT_HEURISTIC

  #ifdef KILLER_MOVE
  memset(CurrentBoard.KillerMoveTable, 0, sizeof(CurrentBoard.KillerMoveTable));
  #endif // KILLER_MOVE

  for (int Depth = 0; Depth < MAX_PLY; ++Depth) {
    ThreadDepth[Depth] = 0;
  }

  TimeStart = Clock(); // Время начала поиска хода
  TimeStop = TimeStart + MaxTimeForMove;

  StopSearch = FALSE;

  // Итеративный поиск
  #pragma omp parallel private(SearchDepthCount, ThreadBoard, ThreadScore)
  {
    ThreadBoard = CurrentBoard;

    for (int Depth = 1; Depth <= MaxDepth; ++Depth) {
      if (StopSearch) {
        continue;
      }

      #pragma omp critical
      {
        SearchDepthCount = ++ThreadDepth[Depth];
      }

      if (omp_get_thread_num() > 0) { // Helper thread
        if (Depth > 1 && Depth < MaxDepth && SearchDepthCount > MAX((MaxThreads + 1) / 2, 2)) {
          continue;
        }
      }
/*
      #pragma omp critical
      {
        printf("-- Depth %d Thread number %d\n", Depth, omp_get_thread_num());
      }
*/
      ThreadBoard.Nodes = (U64)0; // Количество просмотренных позиций

      #ifdef DEBUG_STATISTIC
      ThreadBoard.HashCount = (U64)0; // Количество оценок/ходов полученных их хеша
      ThreadBoard.EvaluateCount = (U64)0;
      ThreadBoard.CutoffCount = (U64)0;
      #endif // DEBUG_STATISTIC

      ThreadBoard.SelDepth = 0;

      ThreadBoard.BestMovesRoot[0] = (MoveItem){0, 0, 0, 0}; // Флаг конца списка лучших ходов

      ThreadScore = Search(&ThreadBoard, -INF, INF, Depth, 0, ThreadBoard.BestMovesRoot, TRUE, InCheck, FALSE); // Ищем ход и получаем оценку позиции

      #pragma omp critical
      {
        CurrentBoard.Nodes += ThreadBoard.Nodes;

        #ifdef DEBUG_STATISTIC
        CurrentBoard.HashCount += ThreadBoard.HashCount;
        CurrentBoard.EvaluateCount += ThreadBoard.EvaluateCount;
        CurrentBoard.CutoffCount += ThreadBoard.CutoffCount;
        #endif // DEBUG_STATISTIC

        if (ThreadBoard.SelDepth > CurrentBoard.SelDepth) {
          CurrentBoard.SelDepth = ThreadBoard.SelDepth;
        }
      }

      if (StopSearch) {
        continue;
      }

      if (omp_get_thread_num() == 0) { // Master thread
        #pragma omp critical
        {
          for (int MoveNumber = 0; MoveNumber < MAX_PLY; ++MoveNumber) {
            CurrentBoard.BestMovesRoot[MoveNumber] = ThreadBoard.BestMovesRoot[MoveNumber];
          }

          PrintBestMoves(&CurrentBoard, Depth, CurrentBoard.BestMovesRoot, ThreadScore); // Выводим список лучших ходов

          BestMove = CurrentBoard.BestMovesRoot[0];
          PonderMove = CurrentBoard.BestMovesRoot[1];

          BestScore = ThreadScore;

          if (Depth == MaxDepth) {
            StopSearch = TRUE;
          }

          if (!BestMove.Move || BestScore <= -INF + Depth || BestScore >= INF - Depth) { // Мат или пат (ничья)
            StopSearch = TRUE;
          }
        }
      }
    } // for
  } // pragma omp parallel

  TimeStop = Clock(); // Время окончания поиска хода
  TotalTime = TimeStop - TimeStart; // Суммарное время поиска хода

  return PrintResult(InCheck, BestMove, PonderMove, BestScore);
}
#endif // NONE/PV_SPLITTING/ROOT_SPLITTING/ABDADA/LAZY_SMP

void* ComputerMoveThread(void *) // Ход компьютера (UCI)
{
  ComputerMove();

  pthread_exit(NULL);

  return NULL;
}

BOOL HumanMove(void) // Ход человека
{
  char ReadStr[10];
  char MoveStr[10];

  int GenMoveCount = 0; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  GenerateAllMoves(&CurrentBoard, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)

  while (TRUE) {
    ReadStr[0] = '\0';

    printf("Enter move (e2e4, e7e8q, save, exit) %c ", (IsInCheck(&CurrentBoard, CurrentBoard.CurrentColor) ? '!' : '>'));
    scanf("%s", ReadStr);

    if (!strcmp(ReadStr, "exit")) {
      return FALSE; // Возвращаем флаг: возврат в основное меню
    }

    if (!strcmp(ReadStr, "save")) {
      SaveGame(&CurrentBoard); // Сохраняем партию в файле

      continue;
    }

    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
      MoveStr[0] = (char)BoardName[MoveList[MoveNumber].Move >> 6][0];
      MoveStr[1] = (char)BoardName[MoveList[MoveNumber].Move >> 6][1];

      MoveStr[2] = (char)BoardName[MoveList[MoveNumber].Move & 63][0];
      MoveStr[3] = (char)BoardName[MoveList[MoveNumber].Move & 63][1];

      if (MoveList[MoveNumber].PromotePiece) {
        MoveStr[4] = (char)PiecesCharBlack[MoveList[MoveNumber].PromotePiece];
        MoveStr[5] = '\0';
      }
      else {
        MoveStr[4] = '\0';
      }

      if (!strcmp(ReadStr, MoveStr)) {
        MakeMove(&CurrentBoard, MoveList[MoveNumber]); // Делаем ход

        if (IsInCheck(&CurrentBoard, (CurrentBoard.CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
          UnmakeMove(&CurrentBoard); // Восстанавливаем ход

          break;
        }

        // Ход возможен (с учётом шаха после хода)

        PrintBoard(&CurrentBoard); // Выводим доску и фигуры

        if (CurrentBoard.CurrentColor == WHITE) { // Ход белых
          printf("%d: ... %s%s", (int)(CurrentBoard.HalfMoveNumber / 2), BoardName[MoveList[MoveNumber].Move >> 6], BoardName[MoveList[MoveNumber].Move & 63]);
        }
        else { // Ход чёрных
          printf("%d: %s%s", (int)(CurrentBoard.HalfMoveNumber / 2) + 1, BoardName[MoveList[MoveNumber].Move >> 6], BoardName[MoveList[MoveNumber].Move & 63]);
        }

        if (MoveList[MoveNumber].PromotePiece) {
          printf("%c", PiecesCharBlack[MoveList[MoveNumber].PromotePiece]);
        }

        printf("\n\n");

        if (PositionRepeat2(&CurrentBoard) == 2) {
          printf("{1/2-1/2} Draw by repetition!\n\n");

          return FALSE; // Возвращаем флаг: возврат в основное меню
        }

        if (CurrentBoard.FiftyMove >= 100) {
          printf("{1/2-1/2} Draw by fifty move rule!\n\n");

          return FALSE; // Возвращаем флаг: возврат в основное меню
        }

        return TRUE; // Возвращаем флаг: сделан ход
      } // if
    }
  } // while
}

void Game(const int HumanColor, const int ComputerColor) // Играем партию
{
  int MaxInputDepth; // Максимальная (введённая) глубина просмотра, полуходов
  int MaxInputTimeForMove; // Максимальное (введённое) время на ход, секунд
  int MaxInputThreads;

  printf("Max. depth (0-%d): ", MAX_PLY);
  scanf("%d", &MaxInputDepth); // Максимальная (введённая) глубина просмотра, полуходов

  MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_PLY) ? MaxInputDepth : MAX_PLY;

  printf("Max. time for move, sec.: ");
  scanf("%d", &MaxInputTimeForMove); // Максимальное (введённое) время на ход, секунд

  MaxTimeForMove = (MaxInputTimeForMove > 0) ? (U64)MaxInputTimeForMove : (U64)MAX_TIME;
  MaxTimeForMove *= (U64)1000;

  printf("Max. threads (0-%d): ", ComputerMaxThreads);
  scanf("%d", &MaxInputThreads);

  MaxThreads = (MaxInputThreads > 0 && MaxInputThreads < ComputerMaxThreads) ? MaxInputThreads : ComputerMaxThreads;

  omp_set_num_threads(MaxThreads);

  memset(HashTable, 0, sizeof(HashTable)); // Инициализируем хеш-таблицу
  memset(CurrentBoard.MoveTable, 0, sizeof(CurrentBoard.MoveTable));

  PrintBoard(&CurrentBoard); // Выводим доску и фигуры

  while (TRUE) {
    if (CurrentBoard.CurrentColor == HumanColor) {
      if (!HumanMove()) {
        return; // Возврат в основное меню
      }
    }

    if (CurrentBoard.CurrentColor == ComputerColor) {
      if (!ComputerMove()) {
        return; // Возврат в основное меню
      }
    }
  } // while
}

void GameAuto(void) // Играем партию (auto)
{
  int MaxInputDepth; // Максимальная (введённая) глубина просмотра, полуходов
  int MaxInputTimeForMove; // Максимальное (введённое) время на ход, секунд
  int MaxInputThreads;

  printf("Max. depth (0-%d): ", MAX_PLY);
  scanf("%d", &MaxInputDepth); // Максимальная (введённая) глубина просмотра, полуходов

  MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_PLY) ? MaxInputDepth : MAX_PLY;

  printf("Max. time for move, sec.: ");
  scanf("%d", &MaxInputTimeForMove); // Максимальное (введённое) время на ход, секунд

  MaxTimeForMove = (MaxInputTimeForMove > 0) ? (U64)MaxInputTimeForMove : (U64)MAX_TIME;
  MaxTimeForMove *= (U64)1000;

  printf("Max. threads (0-%d): ", ComputerMaxThreads);
  scanf("%d", &MaxInputThreads);

  MaxThreads = (MaxInputThreads > 0 && MaxInputThreads < ComputerMaxThreads) ? MaxInputThreads : ComputerMaxThreads;

  omp_set_num_threads(MaxThreads);

  memset(HashTable, 0, sizeof(HashTable)); // Инициализируем хеш-таблицу
  memset(CurrentBoard.MoveTable, 0, sizeof(CurrentBoard.MoveTable));

  PrintBoard(&CurrentBoard); // Выводим доску и фигуры

  while (TRUE) {
    if (!ComputerMove()) {
      return; // Возврат в основное меню
    }
  }
}

void InitPiecePosition(BoardItem *Board) // Инициализация списка фигур белых/чёрных
{
  int WhitePieceCounter = 0; // Счётчик белых фигур
  int BlackPieceCounter = 0; // Счётчик чёрных фигур

  for (int Piece = 0; Piece < 6; ++Piece) { // Перебираем фигуры
    for (int Square = 0; Square < 64; ++Square) { // Перебираем все клетки доски
      if (OrderPieceList[Piece] == PIECE(Board->Pieces[Square])) { // Фигура в клетке
        if (Board->Pieces[Square] & WHITE) { // Белая
          Board->WhitePieceList[WhitePieceCounter++] = Square; // Заносим номер клетки в список белых фигур
        }
        else { // Чёрная
          Board->BlackPieceList[BlackPieceCounter++] = Square; // Заносим номер клетки в список чёрных фигур
        }
      }
    }
  }

  // Дозополняем список белых фигур флагом отсутствия фигуры
  while (WhitePieceCounter < 16) {
    Board->WhitePieceList[WhitePieceCounter++] = -1; // Флаг отсутствия фигуры
  }

  // Дозополняем список чёрных фигур флагом отсутствия фигуры
  while (BlackPieceCounter < 16) {
    Board->BlackPieceList[BlackPieceCounter++] = -1; // Флаг отсутствия фигуры
  }
}

int SetFen(BoardItem *Board, char *Fen)
{
  int Square;

  int Index = 0;

  char PassantMove[3];
  int MoveNumber;

  // Очищаем доску
  for (Square = 0; Square < 64; ++Square) {
    Board->Pieces[Square] = EMPTY;
  }

  Square = 0;

  // Загружаем фигуры
  while (Fen[Index] != ' ') {
    if (Fen[Index] == 'P') { // Пешка
      Board->Pieces[Square++] = (PAWN | WHITE);
    }
    else if (Fen[Index] == 'p') { // Пешка
      Board->Pieces[Square++] = (PAWN | BLACK);
    }
    else if (Fen[Index] == 'N') { // Конь
      Board->Pieces[Square++] = (KNIGHT | WHITE);
    }
    else if (Fen[Index] == 'n') { // Конь
      Board->Pieces[Square++] = (KNIGHT | BLACK);
    }
    else if (Fen[Index] == 'B') { // Слон
      Board->Pieces[Square++] = (BISHOP | WHITE);
    }
    else if (Fen[Index] == 'b') { // Слон
      Board->Pieces[Square++] = (BISHOP | BLACK);
    }
    else if (Fen[Index] == 'R') { // Ладья
      Board->Pieces[Square++] = (ROOK | WHITE);
    }
    else if (Fen[Index] == 'r') { // Ладья
      Board->Pieces[Square++] = (ROOK | BLACK);
    }
    else if (Fen[Index] == 'Q') { // Ферзь
      Board->Pieces[Square++] = (QUEEN | WHITE);
    }
    else if (Fen[Index] == 'q') { // Ферзь
      Board->Pieces[Square++] = (QUEEN | BLACK);
    }
    else if (Fen[Index] == 'K') { // Король
      Board->Pieces[Square++] = (KING | WHITE);
    }
    else if (Fen[Index] == 'k') { // Король
      Board->Pieces[Square++] = (KING | BLACK);
    }
    else if (Fen[Index] >= '1' && Fen[Index] <= '8') { // Пустые клетки
      Square += Fen[Index] - 48;
    }

    ++Index;
  } // while

  InitPiecePosition(Board); // Инициализация списка фигур белых/чёрных
  InitEvaluation();

  ++Index;

  if (Fen[Index] == 'w') {
    Board->CurrentColor = WHITE; // Текущий цвет хода
  }
  else { // 'b'
    Board->CurrentColor = BLACK; // Текущий цвет хода
  }

  Index += 2;

  Board->CastleFlags = 0;

  if (Fen[Index] == '-') {
    ++Index;
  }
  else {
    while (Fen[Index] != ' ') {
      if (Fen[Index] == 'K') {
        Board->CastleFlags |= CASTLE_WHITE_KING;
      }
      else if (Fen[Index] == 'Q') {
        Board->CastleFlags |= CASTLE_WHITE_QUEEN;
      }
      else if (Fen[Index] == 'k') {
        Board->CastleFlags |= CASTLE_BLACK_KING;
      }
      else if (Fen[Index] == 'q') {
        Board->CastleFlags |= CASTLE_BLACK_QUEEN;
      }

      ++Index;
    }
  }

  ++Index;

  if (Fen[Index] == '-') {
    Board->PassantSquare = -1; // Индекс клетки, через которую прошла пешка

    ++Index;
  }
  else {
    PassantMove[0] = Fen[Index];
    PassantMove[1] = Fen[Index + 1];
    PassantMove[2] = '\0';

    for (Square = 0; Square < 64; ++Square) { // Перебираем все клетки доски
      if (!strcmp(BoardName[Square], PassantMove)) {
        Board->PassantSquare = Square; // Индекс клетки, через которую прошла пешка

        break;
      }
    }

    Index += 2;
  }

  ++Index;

  Board->FiftyMove = atoi(&Fen[Index]);

  while (Fen[Index] != ' ') {
    ++Index;
  }

  ++Index;

  MoveNumber = atoi(&Fen[Index]);

  Board->HalfMoveNumber = (MoveNumber - 1) * 2 + (Board->CurrentColor == WHITE ? 0 : 1);

  while (Fen[Index] != ' ' && Fen[Index] != '\n' && Fen[Index] != '\r' && Fen[Index] != '\0') {
    ++Index;
  }

  if (Fen[Index] == ' ') {
    ++Index;
  }

  SetBoardHash(Board); // Устанавливаем хеш текущей позиции

  return Index;
}

void InitNewGame(BoardItem *Board) // Инициализация новой партии
{
  SetFen(Board, StartFen);
}

void LoadGame(BoardItem *Board) // Загрузка партии из файла
{
  FILE *File;

  char Buf[256];

  File = fopen("chess.fen", "r");

  if (File == NULL) { // File open error
    printf("File 'chess.fen' open error!\n");

    exit(0);
  }

  fgets(Buf, sizeof(Buf), File);

  SetFen(Board, Buf);

  fclose(File);
}

void SaveGame(BoardItem *Board) // Сохранение партии в файле
{
  FILE *File;

  int EmptyCount = 0; // Счётчик пустых клеток

  File = fopen("chess.fen", "w");

  if (File == NULL) { // File open error
    printf("File 'chess.fen' open error!\n");

    return;
  }

  // Сохраняем фигуры
  for (int Square = 0; Square < 64; ++Square) {
    if (Square > 0 && !(Square % 8)) {
      if (EmptyCount > 0) {
        fprintf(File, "%d", EmptyCount);

        EmptyCount = 0;
      }

      fprintf(File, "/");
    }

    if (Board->Pieces[Square] == EMPTY) { // Пустая клетка
      ++EmptyCount;

      continue;
    }

    // Не пустая клетка

    if (EmptyCount > 0) {
      fprintf(File, "%d", EmptyCount);

      EmptyCount = 0;
    }

    if (Board->Pieces[Square] & WHITE) { // Белая фигура
      fprintf(File, "%c", PiecesCharWhite[PIECE(Board->Pieces[Square])]);
    }
    else { // Чёрная фигура
      fprintf(File, "%c", PiecesCharBlack[PIECE(Board->Pieces[Square])]);
    }
  }

  if (EmptyCount > 0) {
    fprintf(File, "%d", EmptyCount);
  }

  // Сохраняем текущий цвет хода
  fprintf(File, " ");

  if (Board->CurrentColor == WHITE) { // Ход белых
    fprintf(File, "w");
  }
  else { // Ход чёрных
    fprintf(File, "b");
  }

  // Сохраняем флаги возможности рокировки белого/чёрного короля влево/вправо
  fprintf(File, " ");

  if (!Board->CastleFlags) {
    fprintf(File, "-");
  }
  else {
    if (Board->CastleFlags & CASTLE_WHITE_KING) {
      fprintf(File, "K");
    }

    if (Board->CastleFlags & CASTLE_WHITE_QUEEN) {
      fprintf(File, "Q");
    }

    if (Board->CastleFlags & CASTLE_BLACK_KING) {
      fprintf(File, "k");
    }

    if (Board->CastleFlags & CASTLE_BLACK_QUEEN) {
      fprintf(File, "q");
    }
  }

  // Сохраняем индекс клетки, через которую прошла пешка
  fprintf(File, " ");

  if (Board->PassantSquare == -1) {
    fprintf(File, "-");
  }
  else {
    fprintf(File, "%s", BoardName[Board->PassantSquare]);
  }

  fprintf(File, " ");

  fprintf(File, "%d", Board->FiftyMove);

  // Сохраняем номер хода в партии
  fprintf(File, " ");

  fprintf(File, "%d", (int)(Board->HalfMoveNumber / 2) + 1); // Сохраняем номер хода в партии

  fclose(File);
}

U64 CountMoves(const int Depth) // Вычисление количества возможных ходов для заданной глубины
{
  int GenMoveCount = 0; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  U64 LegalMoveCounter = (U64)0; // Количество возможных ходов (с учётом шаха после хода)

  if (Depth == 0) { // Достигли глубины просмотра
    return (U64)1;
  }

  GenerateAllMoves(&CurrentBoard, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)

  for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
    MakeMove(&CurrentBoard, MoveList[MoveNumber]); // Делаем ход

    if (IsInCheck(&CurrentBoard, (CurrentBoard.CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
      UnmakeMove(&CurrentBoard); // Восстанавливаем ход

      continue;
    }

    // Ход возможен (с учётом шаха после хода)

    LegalMoveCounter += CountMoves(Depth - 1); // Вычисление количества возможных ходов для заданной глубины

    UnmakeMove(&CurrentBoard); // Восстанавливаем ход
  }

  return LegalMoveCounter;
}

void TestGenerateMoves(void)
{
  int MaxInputDepth; // Максимальная (введённая) глубина просмотра, полуходов

  printf("Max. depth (0-%d): ", MAX_PLY);
  scanf("%d", &MaxInputDepth); // Максимальная (введённая) глубина просмотра, полуходов

  MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_PLY) ? MaxInputDepth : MAX_PLY;

  PrintBoard(&CurrentBoard); // Выводим доску и фигуры

  for (int Depth = 1; Depth <= MaxDepth; ++Depth) {
    printf("%d %llu\n", Depth, CountMoves(Depth)); // Вычисление и вывод количества возможных ходов для заданной глубины
  }
}

void Tests(void)
{
  int MaxInputDepth; // Максимальная (введённая) глубина просмотра, полуходов
  int MaxInputTimeForMove; // Максимальное (введённое) время на ход, секунд
  int MaxInputThreads;

  U64 TotalTestNodes = (U64)0;
  U64 TotalTestTime = (U64)0;

  // https://www.chessprogramming.org/Bratko-Kopec_Test
  char *TestFen[] = {
    "1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - - 0 1 bm Qd1+",
    "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - 0 1 bm d5",
    "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - - 0 1 bm f5",
    "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq - 0 1 bm e6",
    "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - - 0 1 bm Nd5 a4",
    "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - - 0 1 bm g6",
    "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - - 0 1 bm Nf6",
    "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - - 0 1 bm f5",
    "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - - 0 1 bm f5",
    "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - - 0 1 bm Ne5",
    "2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - - 0 1 bm f4",
    "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - - 0 1 bm Bf5",
    "r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - - 0 1 bm b4",
    "rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w - - 0 1 bm Qd2 Qe1",
    "2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - - 0 1 bm Qxg7+",
    "r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq - 0 1 bm Ne4",
    "r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - - 0 1 bm h5",
    "r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - - 0 1 bm Nb3",
    "3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - - 0 1 bm Rxe4",
    "r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - - 0 1 bm g4",
    "3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - - 0 1 bm Nh6",
    "2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - - 0 1 bm Bxe4",
    "r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq - 0 1 bm f6",
    "r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - - 0 1 bm f4"
  };

  printf("Max. depth (0-%d): ", MAX_PLY);
  scanf("%d", &MaxInputDepth); // Максимальная (введённая) глубина просмотра, полуходов

  MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_PLY) ? MaxInputDepth : MAX_PLY;

  printf("Max. time for move, sec.: ");
  scanf("%d", &MaxInputTimeForMove); // Максимальное (введённое) время на ход, секунд

  MaxTimeForMove = (MaxInputTimeForMove > 0) ? (U64)MaxInputTimeForMove : (U64)MAX_TIME;
  MaxTimeForMove *= (U64)1000;

  printf("Max. threads (0-%d): ", ComputerMaxThreads);
  scanf("%d", &MaxInputThreads);

  MaxThreads = (MaxInputThreads > 0 && MaxInputThreads < ComputerMaxThreads) ? MaxInputThreads : ComputerMaxThreads;

  omp_set_num_threads(MaxThreads);

  printf("\n");

  for (int TestNumber = 0; TestNumber < (int)(sizeof(TestFen) / sizeof(TestFen[0])); ++TestNumber) {
    for (int CycleNumber = 0; CycleNumber < TEST_CYCLES; ++CycleNumber) {
      printf("Test %d Cycle %d FEN %s\n", (TestNumber + 1), (CycleNumber + 1), TestFen[TestNumber]);

      SetFen(&CurrentBoard, TestFen[TestNumber]);

      memset(HashTable, 0, sizeof(HashTable)); // Инициализируем хеш-таблицу
      memset(CurrentBoard.MoveTable, 0, sizeof(CurrentBoard.MoveTable));

      PrintBoard(&CurrentBoard); // Выводим доску и фигуры

      ComputerMove();

      TotalTestNodes += CurrentBoard.Nodes;
      TotalTestTime += TotalTime;
    }
  }

  printf("Nodes %llu Time %.2f", TotalTestNodes, ((double)TotalTestTime / 1000));

  if (TotalTestTime > 0) {
    printf(" NPS %d", (int)(1000 * TotalTestNodes / TotalTestTime));
  }

  printf("\n\n");
}

void UCI(void)
{
  char Buf[4096];
  char *Part;

  char ReadStr[10];
  char MoveStr[10];

  int GenMoveCount; // Количество возможных ходов (без учёта шаха после хода)
  MoveItem MoveList[MAX_GEN_MOVES]; // Список возможных ходов (без учёта шаха после хода)

  U64 WTime, BTime;
  U64 WInc, BInc;

  int MovesToGo;

  pthread_t MainSearchThread;

  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("id name %s %s\n", NAME, VERSION);
  printf("id author %s\n", AUTHOR);

  SetFen(&CurrentBoard, StartFen);

  printf("uciok\n");

  while (TRUE) {
    fgets(Buf, sizeof(Buf), stdin);

    Part = Buf;

    if (!strncmp(Part, "isready", 7)) {
      printf("readyok\n");
    }
    else if (!strncmp(Part, "ucinewgame", 8)) {
      MaxThreads = 1; // TODO

      omp_set_num_threads(MaxThreads);

      memset(HashTable, 0, sizeof(HashTable)); // Инициализируем хеш-таблицу
      memset(CurrentBoard.MoveTable, 0, sizeof(CurrentBoard.MoveTable));

      SetFen(&CurrentBoard, StartFen);
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

        while (Part[0] != '\n' && Part[0] != '\r' && Part[0] != '\0') {
          ReadStr[0] = Part[0];
          ReadStr[1] = Part[1];
          ReadStr[2] = Part[2];
          ReadStr[3] = Part[3];
          ReadStr[4] = '\0';

          Part += 4;

          if (
            Part[0] == 'Q' || Part[0] == 'R' || Part[0] == 'B' || Part[0] == 'N' ||
            Part[0] == 'q' || Part[0] == 'r' || Part[0] == 'b' || Part[0] == 'n'
          ) {
            ReadStr[4] = Part[0];
            ReadStr[5] = '\0';

            ++Part;
          }

          if (Part[0] == ' ') {
            ++Part;
          }

          GenMoveCount = 0;

          GenerateAllMoves(&CurrentBoard, MoveList, &GenMoveCount); // Генерируем все возможные ходы (без учёта шаха после хода)

          for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) { // Просматриваем список возможных ходов
            MoveStr[0] = (char)BoardName[MoveList[MoveNumber].Move >> 6][0];
            MoveStr[1] = (char)BoardName[MoveList[MoveNumber].Move >> 6][1];

            MoveStr[2] = (char)BoardName[MoveList[MoveNumber].Move & 63][0];
            MoveStr[3] = (char)BoardName[MoveList[MoveNumber].Move & 63][1];

            if (MoveList[MoveNumber].PromotePiece) {
              MoveStr[4] = (char)PiecesCharBlack[MoveList[MoveNumber].PromotePiece];
              MoveStr[5] = '\0';
            }
            else {
              MoveStr[4] = '\0';
            }

            if (!strcmp(ReadStr, MoveStr)) {
              MakeMove(&CurrentBoard, MoveList[MoveNumber]); // Делаем ход

              if (IsInCheck(&CurrentBoard, (CurrentBoard.CurrentColor ^ (WHITE | BLACK)))) { // Шах после хода
                UnmakeMove(&CurrentBoard); // Восстанавливаем ход

                break;
              }

              break;
            }
          }
        } // while
      } // if
    }
    else if (!strncmp(Part, "go", 2)) {
      Part += 3;

      WTime = (U64)0;
      BTime = (U64)0;

      WInc = (U64)0;
      BInc = (U64)0;

      MovesToGo = 0;

      MaxDepth = 0;
      MaxTimeForMove = (U64)0;

      while (Part[0] != '\n' && Part[0] != '\r' && Part[0] != '\0') {
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
        else if (!strncmp(Part, "movetime", 8)) {
          Part += 9;

          MaxTimeForMove = (U64)atoi(Part);
        }
        else if (!strncmp(Part, "infinite", 8)) {
          Part += 9;

          MaxTimeForMove = (U64)MAX_TIME;
          MaxTimeForMove *= (U64)1000;
        }

        ++Part;
      } // while

      if (MovesToGo == 0 || MovesToGo > MOVES_TO_GO) {
        MovesToGo = MOVES_TO_GO;
      }

      if (MaxDepth == 0 || MaxDepth > MAX_PLY) {
        MaxDepth = MAX_PLY;
      }

      if (MaxTimeForMove == (U64)0) {
        if (CurrentBoard.CurrentColor == WHITE && WTime > (U64)0) { // Ход белых
          MaxTimeForMove = (WTime / (U64)MovesToGo) + WInc - (U64)REDUCE_TIME;
        }
        else if (CurrentBoard.CurrentColor == BLACK && BTime > (U64)0) { // Ход чёрных
          MaxTimeForMove = (BTime / (U64)MovesToGo) + BInc - (U64)REDUCE_TIME;
        }
        else {
          MaxTimeForMove = (U64)MAX_TIME;
          MaxTimeForMove *= (U64)1000;
        }
      }

      pthread_create(&MainSearchThread, NULL, ComputerMoveThread, NULL);
    }
    else if (!strncmp(Part, "stop", 4)) {
      StopSearch = TRUE;

      pthread_join(MainSearchThread, NULL);
    }
    else if (!strncmp(Part, "quit", 4)) {
      StopSearch = TRUE;

      pthread_join(MainSearchThread, NULL);

      return;
    }
  } // while
}

int main(int, char **)
{
  char Buf[10];

  int Choice; // Выбранный пункт меню

  printf("%s %s\n", NAME, VERSION);
  printf("Copyright (C) %s %s\n", YEAR, AUTHOR);

  omp_set_dynamic(0);

  ComputerMaxThreads = omp_get_max_threads();

//  printf("HashDataS = %zd HashDataU = %zd HashItem = %zd\n", sizeof(HashDataS), sizeof(HashDataU), sizeof(HashItem));

  #if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
  for (int Depth = 0; Depth < 64; ++Depth) {
    for (int MoveNumber = 0; MoveNumber < 64; ++MoveNumber) {
      LateMoveReductionTable[Depth][MoveNumber] = (int)MAX(log(Depth + 1) * log(MoveNumber + 1) / 1.70, 1.0); // Hakkapeliitta
    }
  }
  #endif // NEGA_SCOUT && LATE_MOVE_REDUCTION

  InitHash(); // Инициализируем хеш фигур, цвета (чёрного) и взятия на проходе

  printf("\n");

  printf("Use UCI commands or press Enter to display the menu\n");

  fgets(Buf, sizeof(Buf), stdin);

  if (!strncmp(Buf, "uci", 3)) {
    UciMode = TRUE;

    UCI();

    return 0;
  }

  UciMode = FALSE;

  while (TRUE) {
    printf("\nMenu:\n");
    printf("1: New white game\n");
    printf("2: New black game\n");
    printf("3: New auto game\n");
    printf("4: Load game from file and white game\n");
    printf("5: Load game from file and black game\n");
    printf("6: Load game from file and auto game\n");
    printf("7: Load game from file and test moves\n");
    printf("8: Tests\n");
    printf("9: Exit\n");

    printf("Choice: ");
    scanf("%d", &Choice);

    switch (Choice) {
      case 1:
        InitNewGame(&CurrentBoard); // Новая партия
        Game(WHITE, BLACK); // Играем партию (белыми)
        break;

      case 2:
        InitNewGame(&CurrentBoard); // Новая партия
        Game(BLACK, WHITE); // Играем партию (чёрными)
        break;

      case 3:
        InitNewGame(&CurrentBoard); // Новая партия
        GameAuto(); // Играем партию (auto)
        break;

      case 4:
        LoadGame(&CurrentBoard); // Загружаем партию из файла
        Game(WHITE, BLACK); // Играем партию (белыми)
        break;

      case 5:
        LoadGame(&CurrentBoard); // Загружаем партию из файла
        Game(BLACK, WHITE); // Играем партию (чёрными)
        break;

      case 6:
        LoadGame(&CurrentBoard); // Загружаем партию из файла
        GameAuto(); // Играем партию (auto)
        break;

      case 7:
        LoadGame(&CurrentBoard); // Загружаем партию из файла
        TestGenerateMoves();
        break;

      case 8:
        Tests();
        break;

      case 9:
        return 0;
    } // switch
  } // while

  return 0;
}