# RukChessHistory
Historical versions of RukChess.

## Chess 0.1
Version: 06.03.2017  
Compiler: GCC 14.2.0 (MSYS2)

## Chess 0.2
Version: 12.12.2017  
Compiler: GCC 14.2.0 (MSYS2)

## Chess 0.3
Version: 12.12.2017  
Compiler: GCC 14.2.0 (MSYS2)

## Chess 1.0
Version: 20.01.2019  
Compiler: GCC 14.2.0 (MSYS2)

### Board Representation
- [8x8 Board](https://www.chessprogramming.org/8x8_Board)
- [10x12 Board](https://www.chessprogramming.org/10x12_Board) for move generation
- [Piece-Lists](https://www.chessprogramming.org/Piece-Lists)

### Search
- [Iterative Deepening](https://www.chessprogramming.org/Iterative_Deepening)
- [Parallel Search](https://www.chessprogramming.org/Parallel_Search) (one of)
  - [PV-splitting](https://www.chessprogramming.org/Parallel_Search#Principal_Variation_Splitting_.28PVS.29) (disabled)
  - Root-splitting (disabled)
  - [ABDADA](https://www.chessprogramming.org/ABDADA) (disabled)
  - [Lazy SMP](https://www.chessprogramming.org/Lazy_SMP) (default)
- [Principal Variation Search](https://www.chessprogramming.org/Principal_Variation_Search)
- [Transposition Table](https://www.chessprogramming.org/Transposition_Table)
  - [Shared Hash Table](https://www.chessprogramming.org/Shared_Hash_Table)
- [Move Ordering](https://www.chessprogramming.org/Move_Ordering)
  - PV move (disabled)
  - Hash move
  - Pawn promote moves (QRBN)
  - Capture moves
    - [MVV-LVA](https://www.chessprogramming.org/MVV-LVA) (default) or [Static Exchange Evaluation](https://www.chessprogramming.org/Static_Exchange_Evaluation) (disabled)
    - Bad capture last ([Static Exchange Evaluation](https://www.chessprogramming.org/Static_Exchange_Evaluation))
  - Quiet moves
    - Killer moves x2 ([Killer Heuristic](https://www.chessprogramming.org/Killer_Heuristic))
    - [History Heuristic](https://www.chessprogramming.org/History_Heuristic) (default) or [Piece-Square Tables](https://www.chessprogramming.org/Piece-Square_Tables) (disabled)
  - [Internal Iterative Deepening](https://www.chessprogramming.org/Internal_Iterative_Deepening)
- [Selectivity](https://www.chessprogramming.org/Selectivity)
  - [Extensions](https://www.chessprogramming.org/Extensions)
    - [Check Extensions](https://www.chessprogramming.org/Check_Extensions)
  - [Pruning](https://www.chessprogramming.org/Pruning)
    - [Mate Distance Pruning](https://www.chessprogramming.org/Mate_Distance_Pruning)
    - [Null Move Pruning](https://www.chessprogramming.org/Null_Move_Pruning)
  - [Reductions](https://www.chessprogramming.org/Reductions)
    - [Late Move Reductions](https://www.chessprogramming.org/Late_Move_Reductions) (disabled)
  - [Quiescence Search](https://www.chessprogramming.org/Quiescence_Search) with check
    - [Mate Distance Pruning](https://www.chessprogramming.org/Mate_Distance_Pruning)
    - Bad capture pruning ([Static Exchange Evaluation](https://www.chessprogramming.org/Static_Exchange_Evaluation)) (disabled)

### Evolution
- [Simplified Evaluation Function](https://www.chessprogramming.org/Simplified_Evaluation_Function)

### Interface
- [Command Line Interface](https://www.chessprogramming.org/CLI)
- [Universal Chess Interface](https://www.chessprogramming.org/UCI)

## Chess 2.0
Version: 20.12.2021  
Compiler: Microsoft Visual Studio 2022