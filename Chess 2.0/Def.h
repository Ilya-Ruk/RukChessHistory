// Def.h

#pragma once

#ifndef DEF_H
#define DEF_H

// Evaluation functions

/*
	https://www.chessprogramming.org/Simplified_Evaluation_Function
*/
//#define SIMPLIFIED_EVALUATION_FUNCTION

/*
	https://www.chessprogramming.org/Toga_Log
	https://manualzz.com/doc/6937632/toga-log-user-manual
*/
#define TOGA_EVALUATION_FUNCTION

/*
	https://www.chessprogramming.org/Stockfish_NNUE
	https://github.com/nodchip/Stockfish
	https://github.com/official-stockfish/Stockfish
	https://github.com/syzygy1/Cfish
*/
//#define NNUE_EVALUATION_FUNCTION

#ifdef NNUE_EVALUATION_FUNCTION

#define USE_NNUE_AVX2							// Required NNUE_EVALUATION_FUNCTION
#define USE_NNUE_REFRESH						// Required NNUE_EVALUATION_FUNCTION

#endif // NNUE_EVALUATION_FUNCTION

// Features (enable/disable)

// Debug

//#define DEBUG_BIT_BOARD_INIT
//#define DEBUG_MOVE
//#define DEBUG_HASH
//#define DEBUG_IID
//#define DEBUG_PVS
//#define DEBUG_SINGULAR_EXTENSION
//#define DEBUG_NNUE

//#define DEBUG_STATISTIC

// Common

#define ASPIRATION_WINDOW

#define ALPHA_BETA_PRUNING

//#define MOVES_SORT_SEE
#define MOVES_SORT_MVV_LVA

#define MOVES_SORT_HEURISTIC
//#define MOVES_SORT_SQUARE_SCORE				// Required SIMPLIFIED_EVALUATION_FUNCTION

#define HASH_PREFETCH

// Search

#define MATE_DISTANCE_PRUNING
#define HASH_SCORE
#define REVERSE_FUTILITY_PRUNING
#define RAZORING								// Required QUIESCENCE
#define NULL_MOVE_PRUNING
#define IID
//#define IIR
//#define PVS									// Required MOVES_SORT_...
#define HASH_MOVE								// Required MOVES_SORT_...
#define KILLER_MOVE								// Required MOVES_SORT_... and HASH_MOVE
#define KILLER_MOVE_2							// Required MOVES_SORT_..., HASH_MOVE and KILLER_MOVE
#define BAD_CAPTURE_LAST						// Required MOVES_SORT_MVV_LVA
#define FUTILITY_PRUNING
#define LATE_MOVE_PRUNING
#define SEE_QUIET_MOVE_PRUNING
#define SEE_CAPTURE_MOVE_PRUNING				// Required MOVES_SORT_SEE or BAD_CAPTURE_LAST
#define CHECK_EXTENSION
#define SINGULAR_EXTENSION						// Required HASH_SCORE and HASH_MOVE
#define NEGA_SCOUT
#define LATE_MOVE_REDUCTION						// Required NEGA_SCOUT

// Quiescence search

#define QUIESCENCE

#define QUIESCENCE_MATE_DISTANCE_PRUNING
#define QUIESCENCE_HASH_SCORE
#define QUIESCENCE_CHECK_EXTENSION
//#define QUIESCENCE_GENERATE_ALL_MOVES
//#define QUIESCENCE_PVS						// Required MOVES_SORT_...
#define QUIESCENCE_HASH_MOVE					// Required MOVES_SORT_...
#define QUIESCENCE_SEE_MOVE_PRUNING				// Required MOVES_SORT_SEE or MOVES_SORT_MVV_LVA

// Parallel search

//#define ROOT_SPLITTING
//#define PV_SPLITTING
//#define ABDADA
#define LAZY_SMP

//#define BIND_THREAD							// Required LAZY_SMP or ABDADA
//#define BIND_THREAD_V2						// Required LAZY_SMP or ABDADA (Max. 64 CPUs)

// Heuristic/Killer move tables

#ifndef LAZY_SMP

#define COMMON_HEURISTIC_TABLE					// Required MOVES_SORT_HEURISTIC
#define COMMON_KILLER_MOVE_TABLE				// Required KILLER_MOVE

#endif // !LAZY_SMP

// Tuning

#define TUNING									// Required TOGA_EVALUATION_FUNCTION

// ------------------------------------------------------------------

// Parameter values

// Program name, version and copyright information

#define NAME									"RukChess"
#define VERSION									"2.0"

#define YEAR									"2021"
#define AUTHOR									"Ilya Rukavishnikov"

// Max limits and default values

#define MAX_PLY									  128

#define MAX_GEN_MOVES							  256 // The maximum number of moves in the position
#define MAX_GAME_MOVES							 1024 // The maximum number of moves in the game

#define MAX_FEN_LENGTH							  256

#define ASPIRATION_WINDOW_INIT_DELTA			   17

#define DEFAULT_HASH_TABLE_SIZE					  512 // Mbyte
#define MAX_HASH_TABLE_SIZE						 4096 // Mbyte

#define DEFAULT_THREADS								1
#define MAX_THREADS								  128

// Time management (Xiphos)

#define MAX_TIME								86400 // Maximum time per move, seconds

#define REDUCE_TIME								  150 // msec.

#define REDUCE_TIME_PERCENT						    5 // %
#define MAX_REDUCE_TIME							 1000 // msec.

#define MAX_MOVES_TO_GO							   40 // 25
#define MAX_TIME_MOVES_TO_GO					    3

#define MAX_TIME_STEPS							   10

#define MIN_TIME_RATIO							  0.5
#define MAX_TIME_RATIO							  1.2

#define MIN_SEARCH_DEPTH						    4

#endif // !DEF_H