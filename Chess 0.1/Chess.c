#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BOOL int

#define FALSE 0
#define TRUE  1

#define INF 9999 // Максимальная число для статической оценки короля

#define WHITE -1 // Белые фигуры
#define BLACK 1 // Чёрные фигуры

#define MAX_DEPTH 40 // Максимальная глубина просмотра, полуходов
#define MAX_TIME 86400 // Максимальное время на ход, секунд

typedef BOOL (*fun_2)(int, int); // Прототип функции (указателя на функцию): принимает откуда и куда идёт фигура; возвращает возможность хода (TRUE - ход возможен, FALSE - ход не возможен)

fun_2 BoardPiece[64]; // Список фигур (указателей на функции возможности хода)
int BoardColor[64]; // Список цвета фигур
int WhitePiecePosition[16], BlackPiecePosition[16]; // Список белых/чёрных фигур

BOOL CastlingWhiteLeft, CastlingWhiteRight; // Флаги возможности рокировки белого короля влево/вправо
BOOL CastlingBlackLeft, CastlingBlackRight; // Флаги возможности рокировки чёрного короля влево/вправо

int BestMovesMain[MAX_DEPTH]; // Список лучших ходов

int BestMoveMain; // Лучший ход
int BestEvaluationMain; // Оценка найденного (лучшего) хода

int CurrentColor; // Текущий цвет хода (-1 - белые; 1 - чёрные)
int CurrentEvaluation; // Текущая оценка позиции
int CurrentDepth; // Текущая глубина просмотра, полуходов

int PositionCounter; // Количество просмотренных позиций

// Прототипы функций
BOOL CheckField(int Position); // Проверка атаки клетки
BOOL CheckKing(); // Проверка на шах
int FindMove(int Alpha, int Beta, int Ply, int *BestMoves, int FirstMove, BOOL PV); // Поиск хода и возврат оценки
void SaveGame(); // Сохранение партии в файле

int MoveNumber; // Номер хода в партии

int MaxDepth; // Максимальная глубина просмотра, полуходов
int MaxTimeForMove; // Максимальное время на ход, секунд

int InputFrom, InputTo; // Ход (откуда/куда)

time_t TimeStart, TimeStop; // Время начала/окончания поиска хода
int TotalTime; // Суммарное время поиска хода

int PassantIndex; // Индекс клетки, через которую прошла пешка

int KingBoardScoreOpening[64] = {
	0,		0,		-4,		-10,	-10,	-4,		0,		0,
	-4,		-4,		-8,		-12,	-12,	-8,		-4,		-4,
	-12,	-16,	-20,	-20,	-20,	-20,	-16,	-12,
	-16,	-20,	-24,	-24,	-24,	-24,	-20,	-16,
	-16,	-20,	-24,	-24,	-24,	-24,	-20,	-16,
	-12,	-16,	-20,	-20,	-20,	-20,	-16,	-12,
	-4,		-4,		-8,		-12,	-12,	-8,		-4,		-4,
	0,		0,		-4,		-10,	-10,	-4,		0,		0
};
/*
int KingBoardScoreEnding[64] = {
	0,		6,		12,		18,		18,		12,		6,		0,
	6,		12,		18,		24,		24,		18,		12,		6,
	12,		18,		24,		30,		30,		24,		18,		12,
	18,		24,		30,		36,		36,		30,		24,		18,
	18,		24,		30,		36,		36,		30,		24,		18,
	12,		18,		24,		30,		30,		24,		18,		12,
	6,		12,		18,		24,		24,		18,		12,		6,
	0,		6,		12,		18,		18,		12,		6,		0
};
*/
int BishopBoardScore[64] = {
	14,		14,		14,		14,		14,		14,		14,		14,
	14,		22,		18,		18,		18,		18,		22,		14,
	14,		18,		22,		22,		22,		22,		18,		14,
	14,		18,		22,		22,		22,		22,		18,		14,
	14,		18,		22,		22,		22,		22,		18,		14,
	14,		18,		22,		22,		22,		22,		18,		14,
	14,		22,		18,		18,		18,		18,		22,		14,
	14,		14,		14,		14,		14,		14,		14,		14
};

int KnightBoardScore[64] = {
	0,		4,		8,		10,		10,		8,		4,		0,
	4,		8,		16,		20,		20,		16,		8,		4,
	8,		16,		24,		28,		28,		24,		16,		8,
	10,		20,		28,		32,		32,		28,		20,		10,
	10,		20,		28,		32,		32,		28,		20,		10,
	8,		16,		24,		28,		28,		24,		16,		8,
	4,		8,		16,		20,		20,		16,		8,		4,
	0,		4,		8,		10,		10,		8,		4,		0
};

int PawnWhiteBoardScore[64] = {
	0,		0,		0,		0,		0,		0,		0,		0,
	12,		16,		24,		32,		32,		24,		16,		12,
	12,		16,		24,		32,		32,		24,		16,		12,
	8,		12,		16,		24,		24,		16,		12,		8,
	6,		8,		12,		16,		16,		12,		8,		6,
	6,		8,		2,		10,		10,		2,		8,		6,
	4,		4,		4,		0,		0,		4,		4,		4,
	0,		0,		0,		0,		0,		0,		0,		0
};

int PawnBlackBoardScore[64] = {
	0,		0,		0,		0,		0,		0,		0,		0,
	4,		4,		4,		0,		0,		4,		4,		4,
	6,		8,		2,		10,		10,		2,		8,		6,
	6,		8,		12,		16,		16,		12,		8,		6,
	8,		12,		16,		24,		24,		16,		12,		8,
	12,		16,		24,		32,		32,		24,		16,		12,
	12,		16,		24,		32,		32,		24,		16,		12,
	0,		0,		0,		0,		0,		0,		0,		0
};

int PawnToQueenWhiteBoardScore[8] = {0, 140, 30, 20, 15, 10, 5, 0};

int PawnToQueenBlackBoardScore[8] = {0, 5, 10, 15, 20, 30, 140, 0};

char BoardName[][64] = {
	"a8",	"b8",	"c8",	"d8",	"e8",	"f8",	"g8",	"h8",
	"a7",	"b7",	"c7",	"d7",	"e7",	"f7",	"g7",	"h7",
	"a6",	"b6",	"c6",	"d6",	"e6",	"f6",	"g6",	"h6",
	"a5",	"b5",	"c5",	"d5",	"e5",	"f5",	"g5",	"h5",
	"a4",	"b4",	"c4",	"d4",	"e4",	"f4",	"g4",	"h4",
	"a3",	"b3",	"c3",	"d3",	"e3",	"f3",	"g3",	"h3",
	"a2",	"b2",	"c2",	"d2",	"e2",	"f2",	"g2",	"h2",
	"a1",	"b1",	"c1",	"d1",	"e1",	"f1",	"g1",	"h1"
};

BOOL CheckPieceLine(int From, int To, int dH, int dV) // Проверка возможности хода дальнобойной фигурой (слон, ладья, ферзь)
{
	int Delta = 0; // Шаг смещения фигуры

	if (dV < 0)
	{
		Delta = -8;
	}
	else if (dV > 0)
	{
		Delta = 8;
	}

	if (dH < 0)
	{
		Delta--;
	}
	else if (dH > 0)
	{
		Delta++;
	}

	// Проверка клеток по пути движения фигуры
	for (int i = (From + Delta); i != To; i += Delta)
	{
		if (BoardPiece[i]) // В клетке есть фигура
		{
			return FALSE; // Ход не возможен
		}
	} // for

	return TRUE; // Ход возможен
}

BOOL Pawn(int From, int To) // Пешка
{
	int ColFrom = From & 7; // Колонка (откуда)
	int LineFrom = From >> 3; // Строчка (откуда)

	int ColTo = To & 7; // Колонка (куда)
	int LineTo = To >> 3; // Строчка (куда)

	int dH = ColTo - ColFrom; // Смещение фигуры по горизонтали доски
	int dV = LineTo - LineFrom; // Смещение фигуры по вертикали доски

	if (!dH) // Ход
	{
		if (dV == CurrentColor) // Ход
		{
			if (!BoardColor[To]) // Пустая клетка (куда)
			{
				return TRUE; // Ход возможен
			}

			return FALSE; // Ход не возможен
		}
		else if (dV == (2 * CurrentColor) && LineFrom == (CurrentColor == WHITE ? 6 : 1)) // Первый ход
		{
			if (!BoardColor[From + 8 * CurrentColor] && !BoardColor[To]) // Пустые клетки (между и куда)
			{
				return TRUE; // Ход возможен
			}

			return FALSE; // Ход не возможен
		}
	}
	else if (abs(dH) == 1 && dV == CurrentColor) // Взятие
	{
		if (To == PassantIndex) // Взятие на проходе
		{
			return TRUE; // Ход (взятие на проходе) возможен
		}
		else if (BoardColor[To] == -CurrentColor) // Фигура противника
		{
			return TRUE; // Ход (взятие) возможен
		}

		return FALSE; // Ход (взятие или взятие на проходе) не возможен
	}

	return FALSE; // Ход не возможен
}

BOOL Knight(int From, int To) // Конь
{
	int dH = (To & 7) - (From & 7); // Смещение фигуры по горизонтали доски
	int dV = (To >> 3) - (From >> 3); // Смещение фигуры по вертикали доски

	if (!(abs(dH) * abs(dV) - 2))
	{
		return TRUE; // Ход возможен
	}

	return FALSE; // Ход не возможен
}

BOOL Bishop(int From, int To) // Слон
{
	int dH = (To & 7) - (From & 7); // Смещение фигуры по горизонтали доски
	int dV = (To >> 3) - (From >> 3); // Смещение фигуры по вертикали доски

	if (!(abs(dH) - abs(dV)) && CheckPieceLine(From, To, dH, dV))
	{
		return TRUE; // Ход возможен
	}

	return FALSE; // Ход не возможен
}

BOOL Rook(int From, int To) // Ладья
{
	int dH = (To & 7) - (From & 7); // Смещение фигуры по горизонтали доски
	int dV = (To >> 3) - (From >> 3); // Смещение фигуры по вертикали доски

	if (!(dH * dV) && CheckPieceLine(From, To, dH, dV))
	{
		return TRUE; // Ход возможен
	}

	return FALSE; // Ход не возможен
}

BOOL Queen(int From, int To) // Ферзь
{
	int dH = (To & 7) - (From & 7); // Смещение фигуры по горизонтали доски
	int dV = (To >> 3) - (From >> 3); // Смещение фигуры по вертикали доски

	if ((!(dH * dV) || !(abs(dH) - abs(dV))) && CheckPieceLine(From, To, dH, dV))
	{
		return TRUE; // Ход возможен
	}

	return FALSE; // Ход не возможен
}

BOOL King(int From, int To) // Король
{
	int ColFrom = From & 7; // Колонка (откуда)
	int LineFrom = From >> 3; // Строчка (откуда)

	int ColTo = To & 7; // Колонка (куда)
	int LineTo = To >> 3; // Строчка (куда)

	int dH = ColTo - ColFrom; // Смещение фигуры по горизонтали доски
	int dV = LineTo - LineFrom; // Смещение фигуры по вертикали доски

	if ((!dH && abs(dV) == 1) || (abs(dH) == 1 && !dV) || (abs(dH) == 1 && abs(dV) == 1)) // Ход
	{
		return TRUE; // Ход возможен
	}
	else if (ColFrom == 4 && abs(dH) == 2 && !dV && !CheckKing()) // Ход (рокировка)
	{
		if (ColTo == 6 && !BoardPiece[From + 1] && !BoardPiece[From + 2] && BoardPiece[From + 3] == Rook && BoardColor[From + 3] == CurrentColor && (CurrentColor == WHITE ? CastlingWhiteRight : CastlingBlackRight) && !CheckField(From + 1) && !CheckField(From + 2)) // Короткая рокировка О-О
		{
			return TRUE; // Ход (рокировка) возможен
		}
		else if (ColTo == 2 && !BoardPiece[From - 1] && !BoardPiece[From - 2] && !BoardPiece[From - 3] && BoardPiece[From - 4] == Rook && BoardColor[From - 4] == CurrentColor && (CurrentColor == WHITE ? CastlingWhiteLeft : CastlingBlackLeft) && !CheckField(From - 1) && !CheckField(From - 2)) // Длинная рокировка О-О-О
		{
			return TRUE; // Ход (рокировка) возможен
		}
	}

	return FALSE; // Ход не возможен
}
/*
int PieceMobility(int Position) // Вычисление оценки подвижности фигуры
{
	int Mobility = 0; // Оценка подвижности фигуры

	for (int j = 0; j < 64; j++) // Перебираем все клетки доски (куда)
	{
		if (Position == j || BoardPiece[j] == King)
		{
			continue;
		}

    if (BoardPiece[Position](Position, j)) // Ход возможен (без учёта шаха после хода)
    {
      if (BoardColor[j] == -CurrentColor) // Фигура противника
      {
        Mobility += 5; // Атака
      }
      else if (BoardColor[j] == 0) // Пустая клетка
      {
        Mobility += 1; // Мобильность
      }
    } // if
	} // for

	return Mobility; // Возвращаем оценку подвижности фигуры
}
*/
int PieceEvaluation(int Position) // Вычисление оценки фигуры
{
	BOOL PawnToQueen = TRUE; // Флаг проходной пешки
	int	PieceScore = 0; // Оценка фигуры

	if (BoardPiece[Position] == King) // Король
	{
		PieceScore = INF + KingBoardScoreOpening[Position];
	}
	else if (BoardPiece[Position] == Queen) // Ферзь
	{
		PieceScore = 900;
	}
	else if (BoardPiece[Position] == Rook) // Ладья
	{
		PieceScore = 500;
	}
	else if (BoardPiece[Position] == Bishop) // Слон
	{
		PieceScore = 300 + BishopBoardScore[Position];
	}
	else if (BoardPiece[Position] == Knight) // Конь
	{
		PieceScore = 300 + KnightBoardScore[Position];
	}
	else if (BoardPiece[Position] == Pawn) // Пешка
	{
		PieceScore = 100;

		for (int i = (Position + 8 * CurrentColor); (i >= 0 && i < 64); i += (8 * CurrentColor))
		{
			if (BoardPiece[i] == Pawn)
			{
				PawnToQueen = FALSE;

				break;
			}
		} // for

		if (BoardColor[Position] == WHITE) // Белая пешка
		{
			if (PawnToQueen) // Проходная пешка
			{
				PieceScore += PawnToQueenWhiteBoardScore[Position >> 3];
			}
			else // Обычная пешка
			{
				PieceScore += PawnWhiteBoardScore[Position];
			}
		}
		else // Чёрная пешка
		{
			if (PawnToQueen) // Проходная пешка
			{
				PieceScore += PawnToQueenBlackBoardScore[Position >> 3];
			}
			else // Обычная пешка
			{
				PieceScore += PawnBlackBoardScore[Position];
			}
		}
	}

//  PieceScore += PieceMobility(Position);

	return PieceScore; // Возвращаем оценку фигуры
}

int PositionEvaluation() // Вычисление оценки текущей позиции
{
	int SaveColor = CurrentColor; // Сохраняем цвет хода
	int PositionScore = 0; // Оценка текущей позиции

	CurrentColor = WHITE;

	for (int i = 0; i < 16; i++) // Перебираем список белых фигур
	{
		if (WhitePiecePosition[i] == -1) // Нет фигуры
		{
			continue;
		}

		PositionScore += PieceEvaluation(WhitePiecePosition[i]);
	} // for

	CurrentColor = BLACK;

	for (int i = 0; i < 16; i++) // Перебираем список чёрных фигур
	{
		if (BlackPiecePosition[i] == -1) // Нет фигуры
		{
			continue;
		}

		PositionScore -= PieceEvaluation(BlackPiecePosition[i]);
	} // for

	CurrentColor = SaveColor; // Восстанавливаем цвет хода

	return PositionScore; // Возвращаем оценку текущей позиции
}

void MakeMove(int From, int To) // Сделать ход
{
	int ColFrom = From & 7; // Колонка (откуда)
	int LineFrom = From >> 3; // Строчка (откуда)

	int ColTo = To & 7; // Колонка (куда)
	int LineTo = To >> 3; // Строчка (куда)

	int *FromPiecePosition, *ToPiecePosition; // Указатели на списки фигур (откуда/куда)

	int EatPawnIndex; // Индекс клетки, где стоит пешка после прохода клетки

	if (To != PassantIndex || BoardPiece[From] != Pawn)
	{
		PassantIndex = -1; // Индекс клетки, через которую прошла пешка
	}

	if (CurrentColor == WHITE) // Ход белых
	{
		FromPiecePosition = WhitePiecePosition;
		ToPiecePosition = BlackPiecePosition;

		if (BoardPiece[From] == Pawn && LineFrom == 6 && LineTo == 4)
		{
			PassantIndex = From - 8; // Индекс клетки, через которую прошла пешка
		}

		CurrentEvaluation -= PieceEvaluation(From);

		if (To == PassantIndex) // Взятие на проходе
		{
			EatPawnIndex = To + 8; // Индекс клетки, где стоит пешка после прохода клетки

			CurrentEvaluation += PieceEvaluation(EatPawnIndex);
		}
		else if (BoardPiece[To]) // Взятие
		{
			CurrentEvaluation += PieceEvaluation(To);
		}
	}
	else // Ход чёрных
	{
		FromPiecePosition = BlackPiecePosition;
		ToPiecePosition = WhitePiecePosition;

		if (BoardPiece[From] == Pawn && LineFrom == 1 && LineTo == 3)
		{
			PassantIndex = From + 8; // Индекс клетки, через которую прошла пешка
		}

		CurrentEvaluation += PieceEvaluation(From);

		if (To == PassantIndex) // Взятие на проходе
		{
			EatPawnIndex = To - 8; // Индекс клетки, где стоит пешка после прохода клетки

			CurrentEvaluation -= PieceEvaluation(EatPawnIndex);
		}
		else if (BoardPiece[To]) // Взятие
		{
			CurrentEvaluation -= PieceEvaluation(To);
		}
	}

	for (int i = 0; i < 16; i++) // Перебираем список фигур (откуда)
	{
		if (FromPiecePosition[i] == From) // Нашли фигуру
		{
			FromPiecePosition[i] = To; // Переместили фигуру

			if (To == PassantIndex) // Взятие на проходе
			{
				for (int j = 0; j < 16; j++) // Перебираем список фигур (куда)
				{
					if (ToPiecePosition[j] == EatPawnIndex) // Нашли фигуру
					{
						ToPiecePosition[j] = -1; // Флаг отсутствия фигуры

						break;
					}
				} // for
			}
			else if (BoardPiece[To]) // Взятие
			{
				for (int j = 0; j < 16; j++) // Перебираем список фигур (куда)
				{
					if (ToPiecePosition[j] == To) // Нашли фигуру
					{
						ToPiecePosition[j] = -1; // Флаг отсутствия фигуры

						break;
					}
				} // for
			}

			break;
		} // if
	} // for

	switch (From)
	{
		case 0: // Левая чёрная ладья
			CastlingBlackLeft = FALSE;
			break;

		case 7: // Правая чёрная ладья
			CastlingBlackRight = FALSE;
			break;

		case 56: // Левая белая ладья
			CastlingWhiteLeft = FALSE;
			break;

		case 63: // Правая белая ладья
			CastlingWhiteRight = FALSE;
			break;

		case 4: // Чёрный король
			CastlingBlackLeft = FALSE;
			CastlingBlackRight = FALSE;
			break;

		case 60: // Белый король
			CastlingWhiteLeft = FALSE;
			CastlingWhiteRight = FALSE;
	} // switch

	BoardPiece[To] = BoardPiece[From]; // Копируем фигуру
	BoardColor[To] = BoardColor[From]; // Копируем цвет фигуры

	if (To == PassantIndex) // Взятие на проходе
	{
		BoardPiece[EatPawnIndex] = 0; // Очищаем клетку (фигура)
		BoardColor[EatPawnIndex] = 0; // Очищаем клетку (цвет)

		PassantIndex = -1; // Индекс клетки, через которую прошла пешка
	}

	if (BoardPiece[To] == Pawn && (LineTo == (BoardColor[To] == WHITE ? 0 : 7))) // Пешка становится ферзем
	{
		BoardPiece[To] = Queen; // Pawn -> Queen
	}

	if (BoardPiece[To] == King) // Король
	{
		if (ColFrom == 4 && ColTo == 6) // Короткая рокировка О-О
		{
			MakeMove(To + 1, To - 1); // Перемещение ладьи
		}
		else if (ColFrom == 6 && ColTo == 4) // Возвращение короткой рокировки О-О
		{
			MakeMove(From - 1, From + 1); // Возврат ладьи
		}
		else if (ColFrom == 4 && ColTo == 2) // Длинная рокировка О-О-О
		{
			MakeMove(To - 2, To + 1); // Перемещение ладьи
		}
		else if (ColFrom == 2 && ColTo == 4) // Возвращение длинной рокировки О-О-О
		{
			MakeMove(From + 1, From - 2); // Возврат ладьи
		}
	} // if

	BoardPiece[From] = 0; // Очищаем клетку (фигура)
	BoardColor[From] = 0; // Очищаем клетку (цвет)

	if (CurrentColor == WHITE) // Ход белых
	{
		CurrentEvaluation += PieceEvaluation(To);
	}
	else // Ход чёрных
	{
		CurrentEvaluation -= PieceEvaluation(To);
	}
}

BOOL CheckField(int Position) // Проверка атаки клетки
{
	int *FromPiecePosition; // Указатель на список фигур (откуда)

	if (CurrentColor == WHITE) // Ход белых
	{
		FromPiecePosition = BlackPiecePosition; // Список чёрных фигур
	}
	else // Ход чёрных
	{
		FromPiecePosition = WhitePiecePosition; // Список белых фигур
	}

	CurrentColor = -CurrentColor; // Меняем цвет хода

	for (int i = 0; i < 16; i++) // Перебираем список фигур (откуда)
	{
		if (FromPiecePosition[i] == -1) // Нет фигуры
		{
			continue;
		}

		if (BoardPiece[FromPiecePosition[i]](FromPiecePosition[i], Position)) // Ход возможен (без учёта шаха после хода)
		{
			CurrentColor = -CurrentColor; // Восстанавливаем цвет хода

			return TRUE; // Клетка атакована
		}
	} // for

	CurrentColor = -CurrentColor; // Восстанавливаем цвет хода

	return FALSE; // Клетка не атакована
}

BOOL CheckKing() // Проверка на шах
{
	int KingPosition; // Позиция короля

	if (CurrentColor == WHITE) // Ход белых
	{
		KingPosition = WhitePiecePosition[0]; // Позиция белого короля
	}
	else // Ход чёрных
	{
		KingPosition = BlackPiecePosition[0]; // Позиция чёрного короля
	}

	return CheckField(KingPosition);
}

void PrintBoard() // Выводим доску и фигуры
{
	printf("\n");
	printf("     a   b   c   d   e   f   g   h\n");

	for (int i = 0; i < 64; i++) // Перебираем все клетки доски
	{
		if (!(i % 8))
		{
			printf("   +---+---+---+---+---+---+---+---+\n");
			printf(" %d |", (8 - i / 8));
		}

		if (BoardPiece[i] == Rook) // Ладья
		{
			printf(" %c |", 98 + BoardColor[i] * 16);
		}
		else if (BoardPiece[i] == Bishop) // Слон
		{
			printf(" %c |", 82 + BoardColor[i] * 16);
		}
		else if (BoardPiece[i] == Knight) // Конь
		{
			printf(" %c |", 94 + BoardColor[i] * 16);
		}
		else if (BoardPiece[i] == Queen) // Ферзь
		{
			printf(" %c |", 97 + BoardColor[i] * 16);
		}
		else if (BoardPiece[i] == King) // Король
		{
			printf(" %c |", 91 + BoardColor[i] * 16);
		}
		else if (BoardPiece[i] == Pawn) // Пешка
		{
			printf(" %c |", 96 + BoardColor[i] * 16);
		}
		else if (!BoardPiece[i]) // Пустая клетка
		{
			printf("   |");
		}

		if (!((i + 1) % 8))
		{
			printf(" %d", (8 - i / 8));
			printf("\n");
		}
	} // for

	printf("   +---+---+---+---+---+---+---+---+\n");
	printf("     a   b   c   d   e   f   g   h\n\n");
}

void PrintBestMoves() // Выводим список лучших ходов
{
	printf("Depth %d PV", CurrentDepth);

	for (int i = 0; (i < MAX_DEPTH && BestMovesMain[i]); i++)
	{
		printf(" %s-%s", BoardName[BestMovesMain[i] >> 6], BoardName[BestMovesMain[i] & 63]);
	} // for

	printf("\n");
}

void SaveBestMoves(int *BestMoves, int BestMove, int *TempBestMoves) // Сохраняем лучший ход
{
	int i;

	BestMoves[0] = BestMove; // Лучший ход в начало списка

	// Переносим лучшие ходы с нижнего уровня
	for (i = 0; (i < (MAX_DEPTH - 2) && TempBestMoves[i]); i++)
	{
		BestMoves[i + 1] = TempBestMoves[i];
	} // for

	BestMoves[i + 1] = 0; // Флаг конца списка лучших ходов
}

int TestMove(int From, int To, int *ToPiecePosition, int *Alpha, int *Beta, int *Ply, int *TempBestMoves, BOOL FullSearch, BOOL FollowPV) // Ход (если возможен), оценка текущей позиции, возврат хода
{
	fun_2 SavePieceFrom = BoardPiece[From]; // Сохраняем фигуру в клетке (откуда)
	fun_2 SavePieceTo = BoardPiece[To]; // Сохраняем фигуру в клетке (куда)
	int SavePieceColorTo = BoardColor[To]; // Сохраняем цвет фигуры в клетке (куда)
	int SavePieceToIndex = -1; // Сохраняем индекс (по списку фигур) взятой фигуры
	int SavePassantIndex = PassantIndex; // Сохраняем индекс клетки, через которую прошла пешка
	int EatPawnIndex; // Индекс клетки, где стоит пешка после прохода клетки
	int SaveEvaluation = CurrentEvaluation; // Сохраняем текущую оценку позиции
  BOOL CK; // Флаг шаха (после хода)
	int TempScore; // Текущая оцека

	// Сохраняем флаги возможности левой/правой рокировки белого/чёрного короля
	BOOL SaveCastlingWhiteLeft = CastlingWhiteLeft;
	BOOL SaveCastlingWhiteRight = CastlingWhiteRight;
	BOOL SaveCastlingBlackLeft = CastlingBlackLeft;
	BOOL SaveCastlingBlackRight = CastlingBlackRight;

	TempBestMoves[0] = 0; // Флаг конца списка лучших ходов

	if (To == SavePassantIndex) // Взятие на проходе
	{
		if (CurrentColor == WHITE) // Ход белых
		{
			EatPawnIndex = To + 8; // Индекс клетки, где стоит пешка после прохода клетки
		}
		else // Ход чёрных
		{
			EatPawnIndex = To - 8; // Индекс клетки, где стоит пешка после прохода клетки
		}

		SavePieceTo = BoardPiece[EatPawnIndex]; // Сохраняем фигуру в клетке (куда)
		SavePieceColorTo = BoardColor[EatPawnIndex]; // Сохраняем цвет фигуры в клетке (куда)

		for (int j = 0; j < 16; j++) // Перебираем список фигур (куда)
		{
			if (ToPiecePosition[j] == EatPawnIndex) // Нашли фигуру
			{
				SavePieceToIndex = j; // Сохраняем индекс (по списку фигур) взятой на проходе фигуры

				break;
			}
		} // for
	}
	else if (SavePieceTo) // Взятие
	{
		for (int j = 0; j < 16; j++) // Перебираем список фигур (куда)
		{
			if (ToPiecePosition[j] == To) // Нашли фигуру
			{
				SavePieceToIndex = j; // Сохраняем индекс (по списку фигур) взятой фигуры

				break;
			}
		} // for
	}

	MakeMove(From, To); // Делаем ход

	CK = CheckKing(); // Проверка на шах

	if (!CK) // Нет шаха (после хода)
	{
		CurrentColor = -CurrentColor; // Меняем цвет хода

    PositionCounter++; // Увеличиваем число просмотренных позиций

		if (!FullSearch) // Сокращенный поиск (с нулевым окном)
		{
			TempScore = -FindMove(-(*Alpha + 1), -(*Alpha), (*Ply + 1), TempBestMoves, 0, FALSE); // Получаем оценку текущей позиции (с нулевым окном)
		}

		if (FullSearch || (TempScore > *Alpha && TempScore < *Beta)) // Полный поиск (без нулевого окна) или полученная оценка (с нулевым окном) в диапазоне от alpha до beta
		{
			TempScore = -FindMove(-(*Beta), -(*Alpha), (*Ply + 1), TempBestMoves, ((FollowPV && BestMovesMain[*Ply + 1]) ? BestMovesMain[*Ply + 1] : 0), TRUE); // Получаем оценку текущей позиции (с полным окном)
		}

		CurrentColor = -CurrentColor; // Восстанавливаем цвет хода
	} // if

	MakeMove(To, From); // Восстанавливаем ход

	BoardPiece[From] = SavePieceFrom; // Восстанавливаем фигуру в клетке (откуда): Queen -> Pawn

	if (To == SavePassantIndex) // Взятие на проходе
	{
		BoardPiece[EatPawnIndex] = SavePieceTo; // Восстанавливаем фигуру в клетке (куда)
		BoardColor[EatPawnIndex] = SavePieceColorTo; // Восстанавливаем цвет фигуры в клетке (куда)

		ToPiecePosition[SavePieceToIndex] = EatPawnIndex; // Восстанавливаем взятую на проходе фигуру
	}
	else if (SavePieceTo) // Взятие
	{
		BoardPiece[To] = SavePieceTo; // Восстанавливаем фигуру в клетке (куда)
		BoardColor[To] = SavePieceColorTo; // Восстанавливаем цвет фигуры в клетке (куда)

		ToPiecePosition[SavePieceToIndex] = To; // Восстанавливаем взятую фигуру
	}

	CurrentEvaluation = SaveEvaluation; // Восстанавливаем текущую оценку позиции

	// Восстанавливаем флаги возможности левой/правой рокировки белого/чёрного короля
	CastlingWhiteLeft = SaveCastlingWhiteLeft;
	CastlingWhiteRight = SaveCastlingWhiteRight;
	CastlingBlackLeft = SaveCastlingBlackLeft;
	CastlingBlackRight = SaveCastlingBlackRight;

	PassantIndex = SavePassantIndex; // Восстанавливаем индекс клетки, через которую прошла пешка

	if (CK) // Шах (после хода)
	{
		return 131072; // Возвращаем флаг: ход не возможен (шах после хода)
	}
	else // Нет шаха (после хода)
	{
		return TempScore; // Возвращаем текущую оценку
	}
}

int FindMove(int Alpha, int Beta, int Ply, int *BestMoves, int FirstMove, BOOL PV) // Поиск хода и возврат оценки
{
	BOOL CK = CheckKing(); // Флаг шаха
  BOOL QS = (Ply >= CurrentDepth);
	BOOL MoveFound = FALSE; // Флаг найденного хода
	int FirstMoveFrom = FirstMove >> 6; // Первый ход (откуда)
	int FirstMoveTo = FirstMove & 63; // Первый ход (куда)
	int *FromPiecePosition, *ToPiecePosition; // Указатели на списки фигур (откуда/куда)
	int TempBestMoves[MAX_DEPTH]; // Список лучших ходов
	int TempScore; // Текущая оцека
	int CurrentEvaluationForColor; // Текущая оценка позиции для текущего цвета хода
	int CurrentTime = (int)difftime(time(NULL), TimeStart); // Текущее время (с начала поиска хода)

	if (CurrentDepth > 1 && CurrentTime >= MaxTimeForMove) // Текущая глубина просмотра больше одного полухода и время истекло
	{
		return 65536; // Возвращаем флаг: время истекло
	}

	if (CurrentColor == WHITE) // Ход белых
	{
		CurrentEvaluationForColor = CurrentEvaluation;

		FromPiecePosition = WhitePiecePosition;
		ToPiecePosition = BlackPiecePosition;
	}
	else // Ход чёрных
	{
		CurrentEvaluationForColor = -CurrentEvaluation;

		FromPiecePosition = BlackPiecePosition;
		ToPiecePosition = WhitePiecePosition;
	}

	if (QS && !CK) // Достигли глубины просмотра и не шах
	{
		if (CurrentEvaluationForColor > Alpha) // Текущая оценка позиции для текущего цвета хода больше Alpha
		{
			Alpha = CurrentEvaluationForColor; // Обновляем Alpha

			if (Alpha >= Beta) // Отсечение
			{
				return Alpha; // Возвращаем Alpha
			}
		}
	}

	if (FirstMove && (!QS || CK || ToPiecePosition[FirstMoveTo] != -1 || (BoardPiece[FromPiecePosition[FirstMoveFrom]] == Pawn && FirstMoveTo == PassantIndex))) // Есть первый (лучший) ход
	{
		TempScore = TestMove(FirstMoveFrom, FirstMoveTo, ToPiecePosition, &Alpha, &Beta, &Ply, TempBestMoves, TRUE, TRUE); // Ход, оценка текущей позиции, возврат хода

		if (TempScore == -65536) // Время истекло
		{
			return 65536; // Возвращаем флаг: время истекло
		}

		MoveFound = TRUE; // Флаг найденного хода

		if (TempScore > Alpha) // Нашли лучший ход на текущем уровне
		{
			Alpha = TempScore; // Обновляем оценку лучшего хода

			if (Alpha >= Beta) // Отсечение
			{
				return Alpha; // Возвращаем Alpha
			}

			SaveBestMoves(BestMoves, FirstMove, TempBestMoves); // Сохраняем лучший ход
		}
	} // if

	// Просматриваем взятия
	for (int i = 0; i < 16; i++) // Перебираем список фигур (откуда)
	{
		if (FromPiecePosition[i] == -1) // Нет фигуры
		{
			continue;
		}

		if (BoardPiece[FromPiecePosition[i]] == Pawn && PassantIndex != -1) // Взятие на проходе
		{
      if (FromPiecePosition[i] != FirstMoveFrom || PassantIndex != FirstMoveTo) // Не совпадает с первым ходом
      {
        if (BoardPiece[FromPiecePosition[i]](FromPiecePosition[i], PassantIndex)) // Взятие на проходе возможно (без учёта шаха после хода)
        {
          TempScore = TestMove(FromPiecePosition[i], PassantIndex, ToPiecePosition, &Alpha, &Beta, &Ply, TempBestMoves, (PV && !MoveFound), FALSE); // Ход (если возможен), оценка текущей позиции, возврат хода

          if (TempScore == -65536) // Время истекло
          {
            return 65536; // Возвращаем флаг: время истекло
          }

          if (TempScore != 131072) // Ход возможен
          {
            MoveFound = TRUE; // Флаг найденного хода

            if (TempScore > Alpha) // Нашли лучший ход на текущем уровне
            {
              Alpha = TempScore; // Обновляем оценку лучшего хода

              if (Alpha >= Beta) // Отсечение
              {
                return Alpha; // Возвращаем Alpha
              }

              SaveBestMoves(BestMoves, ((FromPiecePosition[i] << 6) + PassantIndex), TempBestMoves); // Сохраняем лучший ход
            }
          } // if
        } // if
      } // if
		} // if

		for (int j = 15; j > 0; j--) // Перебираем список фигур (куда), кроме короля
		{
			if (ToPiecePosition[j] == -1 || (FromPiecePosition[i] == FirstMoveFrom && ToPiecePosition[j] == FirstMoveTo)) // Нет фигуры или совпадает с первым ходом
			{
				continue;
			}

			if (BoardPiece[FromPiecePosition[i]](FromPiecePosition[i], ToPiecePosition[j])) // Взятие возможно (без учёта шаха после хода)
			{
				TempScore = TestMove(FromPiecePosition[i], ToPiecePosition[j], ToPiecePosition, &Alpha, &Beta, &Ply, TempBestMoves, (PV && !MoveFound), FALSE); // Ход (если возможен), оценка текущей позиции, возврат хода

				if (TempScore == -65536) // Время истекло
				{
					return 65536; // Возвращаем флаг: время истекло
				}

				if (TempScore == 131072) // Ход не возможен (шах после хода)
				{
					continue;
				}

				MoveFound = TRUE; // Флаг найденного хода

				if (TempScore > Alpha) // Нашли лучший ход на текущем уровне
				{
					Alpha = TempScore; // Обновляем оценку лучшего хода

					if (Alpha >= Beta) // Отсечение
					{
						return Alpha; // Возвращаем Alpha
					}

					SaveBestMoves(BestMoves, ((FromPiecePosition[i] << 6) + ToPiecePosition[j]), TempBestMoves); // Сохраняем лучший ход
				}
			} // if
		} // for
	} // for

	// Просматриваем перемещения
	if (!QS || CK)
	{
		for (int i = 15; i >= 0; i--) // Перебираем список фигур (откуда)
		{
			if (FromPiecePosition[i] == -1) // Нет фигуры
			{
				continue;
			}

			for (int j = 0; j < 64; j++) // Перебираем все клетки доски (куда)
			{
				if (BoardColor[j] || (BoardPiece[FromPiecePosition[i]] == Pawn && j == PassantIndex) || (FromPiecePosition[i] == FirstMoveFrom && j == FirstMoveTo)) // Не пустая клетка или совпадает со взятием на проходе или совпадает с первым ходом
				{
					continue;
				}

				if (BoardPiece[FromPiecePosition[i]](FromPiecePosition[i], j)) // Ход возможен (без учёта шаха после хода)
				{
					TempScore = TestMove(FromPiecePosition[i], j, ToPiecePosition, &Alpha, &Beta, &Ply, TempBestMoves, (PV && !MoveFound), FALSE); // Ход (если возможен), оценка текущей позиции, возврат хода

					if (TempScore == -65536) // Время истекло
					{
						return 65536; // Возвращаем флаг: время истекло
					}

					if (TempScore == 131072) // Ход не возможен (шах после хода)
					{
						continue;
					}

					MoveFound = TRUE; // Флаг найденного хода

					if (TempScore > Alpha) // Нашли лучший ход на текущем уровне
					{
						Alpha = TempScore; // Обновляем оценку лучшего хода

						if (Alpha >= Beta) // Отсечение
						{
							return Alpha; // Возвращаем Alpha
						}

						SaveBestMoves(BestMoves, ((FromPiecePosition[i] << 6) + j), TempBestMoves); // Сохраняем лучший ход
					}
				} // if
			} // for
		} // for
	}

	if (!MoveFound) // Ходов нет
	{
    if (CK) // Шах
    {
      return -INF + Ply; // Возвращаем оценку текущей позиции: мат
    }

    if (!QS)
    {
      return 0; // Возвращаем оценку текущей позиции: пат (ничья)
    }
	}

	return Alpha; // Возвращаем Alpha
}

BOOL CheckMove(int From, int To) // Проверка возможности хода фигурой (с учётом шаха после хода)
{
	fun_2 SavePieceFrom = BoardPiece[From]; // Сохраняем фигуру в клетке (откуда)
	fun_2 SavePieceTo = BoardPiece[To]; // Сохраняем фигуру в клетке (куда)
	int SavePieceColorTo = BoardColor[To]; // Сохраняем цвет фигуры в клетке (куда)
	int SavePieceToIndex = -1; // Сохранение индекса (по списку фигур) взятой фигуры
	int *ToPiecePosition; // Указатель на список фигур (куда)
	int SaveEvaluation = CurrentEvaluation; // Сохраняем текущую оценку позиции
	int SavePassantIndex = PassantIndex; // Сохраняем индекс клетки, через которую прошла пешка
	int EatPawnIndex; // Сохраняем индекс клетки, где стоит пешка после прохода клетки

	// Сохраняем флаги возможности левой/правой рокировки белого/чёрного короля
	BOOL SaveCastlingWhiteLeft = CastlingWhiteLeft;
	BOOL SaveCastlingWhiteRight = CastlingWhiteRight;
	BOOL SaveCastlingBlackLeft = CastlingBlackLeft;
	BOOL SaveCastlingBlackRight = CastlingBlackRight;

	BOOL CM = BoardPiece[From](From, To); // Проверка возможности хода фигурой (без учёта шаха после хода)

	if (!CM) // Ход не возможен
	{
		return FALSE; // Ход не возможен
	}

	if (CurrentColor == WHITE) // Ход белых
	{
		ToPiecePosition = BlackPiecePosition;
	}
	else // Ход чёрных
	{
		ToPiecePosition = WhitePiecePosition;
	}

	if (To == SavePassantIndex) // Взятие на проходе
	{
		if (CurrentColor == WHITE) // Ход белых
		{
			EatPawnIndex = To + 8; // Индекс клетки, где стоит пешка после прохода клетки
		}
		else // Ход чёрных
		{
			EatPawnIndex = To - 8; // Индекс клетки, где стоит пешка после прохода клетки
		}

		SavePieceTo = BoardPiece[EatPawnIndex];
		SavePieceColorTo = BoardColor[EatPawnIndex];

		for (int j = 0; j < 16; j++) // Перебираем список фигур (куда)
		{
			if (ToPiecePosition[j] == EatPawnIndex) // Нашли фигуру
			{
				SavePieceToIndex = j; // Сохраняем индекс (по списку фигур) взятой на проходе фигуры

				break;
			}
		} // for
	}
	else if (SavePieceTo) // Взятие
	{
		for (int j = 0; j < 16; j++) // Перебираем список фигур (куда)
		{
			if (ToPiecePosition[j] == To) // Нашли фигуру
			{
				SavePieceToIndex = j; // Сохраняем индекс (по списку фигур) взятой фигуры

				break;
			}
		} // for
	}

	MakeMove(From, To); // Делаем ход

	CM = !CheckKing(); // Проверяем на шах

	MakeMove(To, From); // Восстанавливаем ход

	BoardPiece[From] = SavePieceFrom; // Восстанавливаем фигуру в клетке (откуда): Queen -> Pawn

	if (To == SavePassantIndex) // Взятие на проходе
	{
		BoardPiece[EatPawnIndex] = SavePieceTo; // Восстанавливаем фигуру в клетке (куда)
		BoardColor[EatPawnIndex] = SavePieceColorTo; // Восстанавливаем цвет фигуры в клетке (куда)

		ToPiecePosition[SavePieceToIndex] = EatPawnIndex; // Восстанавливаем взятую на проходе фигуру
	}
	else if (SavePieceTo) // Взятие
	{
		BoardPiece[To] = SavePieceTo; // Восстанавливаем фигуру в клетке (куда)
		BoardColor[To] = SavePieceColorTo; // Восстанавливаем цвет фигуры в клетке (куда)

		ToPiecePosition[SavePieceToIndex] = To; // Восстанавливаем взятую фигуру
	}

	CurrentEvaluation = SaveEvaluation; // Восстанавливаем текущую оценку позиции

	// Восстанавливаем флаги возможности левой/правой рокировки белого/чёрного короля
	CastlingWhiteLeft = SaveCastlingWhiteLeft;
	CastlingWhiteRight = SaveCastlingWhiteRight;
	CastlingBlackLeft = SaveCastlingBlackLeft;
	CastlingBlackRight = SaveCastlingBlackRight;

	PassantIndex = SavePassantIndex; // Восстанавливаем индекс клетки, через которую прошла пешка

	return CM; // Ход возможен/не возможен
}

void GetParametrs() // Ввод параметров поиска ходов
{
  int MaxInputDepth;
  int MaxInputTimeForMove;

	printf("Max. depth: ");
	scanf("%d", &MaxInputDepth); // Максимальная (введённая) глубина просмотра, полуходов

	MaxDepth = (MaxInputDepth > 0 && MaxInputDepth < MAX_DEPTH) ? MaxInputDepth : MAX_DEPTH;

	printf("Max. time for move, sec.: ");
	scanf("%d", &MaxInputTimeForMove); // Максимальное (введённое) время на ход, секунд

	MaxTimeForMove = (MaxInputTimeForMove > 0 && MaxInputTimeForMove < MAX_TIME) ? MaxInputTimeForMove : MAX_TIME;
}

int MoveToInt(char * Str) // Преобразуем ход из строки в число
{
	for (int i = 0; i < 64; i++) // Перебираем все клетки доски
	{
		if (!strcmp(BoardName[i], Str))
		{
			return i; // Возвращаем номер клетки доски
		}
	} // for

	return -1; // Флаг ошибки
}

BOOL ComputerMove()
{
  int Score = 0;

	PositionCounter = 0; // Количество просмотренных позиций

	TimeStart = time(NULL); // Время начала поиска хода

  BestMovesMain[0] = 0;

	// Итеративный поиск
	for (CurrentDepth = 1; CurrentDepth <= MaxDepth; CurrentDepth++)
	{
		Score = FindMove(-INF, INF, 0, BestMovesMain, BestMovesMain[0], TRUE); // Ищем ход и получаем оценку позиции

		if (Score == 65536) // Время истекло
		{
			break;
		}

    BestMoveMain = BestMovesMain[0];
    BestEvaluationMain = Score;

    PrintBestMoves();

		if (!BestMoveMain || BestEvaluationMain <= -INF + CurrentDepth || BestEvaluationMain >= INF - CurrentDepth) // Мат или пат (ничья)
		{
			break;
		}
	} // for

	TimeStop = time(NULL); // Время окончания поиска хода

	TotalTime = (int)difftime(TimeStop, TimeStart); // Суммарное время поиска хода

	if (!BestMoveMain) // Нет хода
	{
		if (CheckKing()) // Мат
		{
			printf("Checkmate!\n");

			if (CurrentColor == WHITE)
			{
				printf("Black wins!\n\n");
			}
			else // BLACK
			{
				printf("White wins!\n\n");
			}
		}
		else // Пат (ничья)
		{
			printf("Stalemate!\n\n");
		}

		return FALSE; // Возвращаем флаг: возврат в основное меню
	} // if

	MakeMove(BestMoveMain >> 6, BestMoveMain & 63); // Делаем ход

  if (BestEvaluationMain >= INF - 1 || BestEvaluationMain <= -INF + 1) // Победа
	{
		PrintBoard(); // Выводим доску и фигуры

		if (CurrentColor == WHITE)
		{
			printf("%d: %s-%s\n", MoveNumber, BoardName[BestMoveMain >> 6], BoardName[BestMoveMain & 63]);
		}
		else // BLACK
		{
			printf("%d: ... %s-%s\n", MoveNumber, BoardName[BestMoveMain >> 6], BoardName[BestMoveMain & 63]);
		}

		printf("%d %d %d\n", PositionCounter, BestEvaluationMain, TotalTime);

		printf("Checkmate!\n");

		if (CurrentColor == WHITE)
		{
			printf("White wins!\n\n");
		}
		else // BLACK
		{
			printf("Black wins!\n\n");
		}

		return FALSE; // Возвращаем флаг: возврат в основное меню
	} // if

	return TRUE; // Возвращаем флаг: сделан ход
}

BOOL HumanMove()
{
	char ReadStr[5]; // Строка с ходом, например, e2e4, плюс один символ окончания строки '\0'
	char InputFromStr[3], InputToStr[3]; // Строка с ходом (откуда/куда), например, e2/e4, плюс один символ окончания строки '\0'

	PrintBoard(); // Выводим доску и фигуры

	if (BestMoveMain)
	{
		if (CurrentColor == WHITE)
		{
			printf("%d: ... %s-%s\n", (MoveNumber - 1), BoardName[BestMoveMain >> 6], BoardName[BestMoveMain & 63]);
		}
		else // BLACK
		{
			printf("%d: %s-%s\n", MoveNumber, BoardName[BestMoveMain >> 6], BoardName[BestMoveMain & 63]);
		}

		printf("%d %d %d\n", PositionCounter, BestEvaluationMain, TotalTime);
	}

	while (TRUE)
	{
		printf("Enter move (e2e4, save, exit) %c ", CheckKing() ? '!' : '>');
		scanf("%s", ReadStr);

		ReadStr[4] = '\0';

		if (!strcmp(ReadStr, "exit"))
		{
			return FALSE; // Возвращаем флаг: возврат в основное меню
		}

		if (!strcmp(ReadStr, "save"))
		{
			SaveGame(); // Сохраняем партию в файле

			continue;
		}

		strncpy(InputFromStr, ReadStr, 2); // Копируем ход (откуда), например, e2

		InputFromStr[2] = '\0';

		strncpy(InputToStr, (ReadStr + 2), 2); // Копируем ход (куда), например, e4

		InputToStr[2] = '\0';

		InputFrom = MoveToInt(InputFromStr); // Преобразуем ход (откуда) из строки в число
		InputTo = MoveToInt(InputToStr); // Преобразуем ход (куда) из строки в число

		if (InputFrom != -1 && InputTo != -1 && BoardColor[InputFrom] == CurrentColor && BoardColor[InputTo] != CurrentColor && CheckMove(InputFrom, InputTo)) // Корректный ход и ход возможен (с учётом шаха после хода)
		{
			break;
		}
	} // while

	MakeMove(InputFrom, InputTo); // Делаем ход

	PrintBoard(); // Выводим доску и фигуры

	if (CurrentColor == WHITE)
	{
		printf("%d: %s-%s\n\n", MoveNumber, BoardName[InputFrom], BoardName[InputTo]);
	}
	else // BLACK
	{
		printf("%d: ... %s-%s\n\n", MoveNumber, BoardName[InputFrom], BoardName[InputTo]);
	}

	return TRUE; // Возвращаем флаг: сделан ход
}

void Game(int HumanColor, int ComputerColor)
{
	GetParametrs(); // Ввод параметров поиска ходов

	InputFrom = 0;
	InputTo = 0;

	PositionCounter = 0; // Количество просмотренных позиций

	BestMovesMain[0] = 0; // Флаг конца списка лучших ходов

  BestMoveMain = 0;
  BestEvaluationMain = 0; // Оценка чёрных

	TotalTime = 0; // Суммарное время поиска хода

	if (CurrentColor == ComputerColor)
	{
		PrintBoard(); // Выводим доску и фигуры
	}

	while (TRUE)
	{
		if (CurrentColor == HumanColor)
		{
			if (!HumanMove())
			{
				return; // Возврат в основное меню
			}

			if (CurrentColor == BLACK)
			{
				MoveNumber++; // Увеличиваем счетчик ходов в партии
			}

			CurrentColor = -CurrentColor; // Меняем цвет хода
		}

		if (CurrentColor == ComputerColor)
		{
			if (!ComputerMove())
			{
				return; // Возврат в основное меню
			}

			if (CurrentColor == BLACK)
			{
				MoveNumber++; // Увеличиваем счетчик ходов в партии
			}

			CurrentColor = -CurrentColor; // Меняем цвет хода
		}
	} // while
}

void GameAuto() // Играем партию (auto)
{
	GetParametrs(); // Ввод параметров поиска ходов

	PrintBoard(); // Выводим доску и фигуры

	BestMovesMain[0] = 0; // Флаг конца списка лучших ходов

  BestMoveMain = 0;
  BestEvaluationMain = 0; // Оценка чёрных

	while (TRUE)
	{
		if (!ComputerMove())
		{
			return; // Возврат в основное меню
		}

		PrintBoard(); // Выводим доску и фигуры

		printf("%d: %s-%s\n", MoveNumber, BoardName[BestMoveMain >> 6], BoardName[BestMoveMain & 63]);
		printf("%d %d %d\n\n", PositionCounter, BestEvaluationMain, TotalTime);

		CurrentColor = -CurrentColor; // Меняем цвет хода

		if (!ComputerMove())
		{
			return; // Возврат в основное меню
		}

		PrintBoard(); // Выводим доску и фигуры

		printf("%d: ... %s-%s\n", MoveNumber, BoardName[BestMoveMain >> 6], BoardName[BestMoveMain & 63]);
		printf("%d %d %d\n\n", PositionCounter, BestEvaluationMain, TotalTime);

		MoveNumber++; // Увеличиваем счетчик ходов в партии

		CurrentColor = -CurrentColor; // Меняем цвет хода
	} // while
}

void InitPiecePosition() // Инициализация списка фигур белых/чёрных
{
	int WhitePieceCounter = 0, BlackPieceCounter = 0; // Счётчик белых/чёрных фигур
	fun_2 Pieces[6] = {King, Pawn, Knight, Bishop, Rook, Queen}; // Список фигур: король всегда первый, остальные фигуры по возрастанию веса

	for (int i = 0; i < 6; i++) // Перебираем фигуры
	{
		for (int j = 0; j < 64; j++) // Перебираем все клетки доски
		{
			if (Pieces[i] == BoardPiece[j]) // Фигура в клетке
			{
				if (BoardColor[j] == WHITE) // Белая
				{
					WhitePiecePosition[WhitePieceCounter] = j; // Заносим номер клетки в список белых фигур

					WhitePieceCounter++; // Увеличиваем счётчик белых фигур
				}
				else // Чёрная
				{
					BlackPiecePosition[BlackPieceCounter] = j; // Заносим номер клетки в список чёрных фигур

					BlackPieceCounter++; // Увеличиваем счётчик чёрных фигур
				}
			}
		} // for
	} // for

	// Дозополняем список белых фигур флагом отсутствия фигуры
	while (WhitePieceCounter < 16)
	{
		WhitePiecePosition[WhitePieceCounter] = -1; // Флаг отсутствия фигуры

		WhitePieceCounter++; // Увеличиваем счётчик белых фигур
	} // while

	// Дозополняем список чёрных фигур флагом отсутствия фигуры
	while (BlackPieceCounter < 16)
	{
		BlackPiecePosition[BlackPieceCounter] = -1; // Флаг отсутствия фигуры

		BlackPieceCounter++; // Увеличиваем счётчик чёрных фигур
	} // while
}

void InitNewGame() // Инициализация новой партии
{
	CurrentColor = WHITE; // Текущий цвет хода

	// Инициализация доски: фигуры
	BoardPiece[0] = BoardPiece[7] = BoardPiece[56] = BoardPiece[63] = Rook; // Ладьи
	BoardPiece[1] = BoardPiece[6] = BoardPiece[57] = BoardPiece[62] = Knight; // Кони
	BoardPiece[2] = BoardPiece[5] = BoardPiece[58] = BoardPiece[61] = Bishop; // Слоны
	BoardPiece[3] = BoardPiece[59] = Queen; // Ферзи
	BoardPiece[4] = BoardPiece[60] = King; // Короли

	for (int i = 8; i < 16; i++) // Пешки
	{
		BoardPiece[i] = Pawn; // Пешки (чёрные)
		BoardPiece[i + 40] = Pawn; // Пешки (белые)
	} // for

	// Инициализация доски: цвета
	for (int i = 0; i < 16; i++) // Заполняем цвета фигур
	{
		BoardColor[i] = BLACK; // Чёрные
		BoardColor[i + 48] = WHITE; // Белые
	} // for

	// Инициализация доски: пустые клетки
	for (int i = 16; i < 48; i++)
	{
		BoardPiece[i] = 0; // Пустые клетки (фигуры)
		BoardColor[i] = 0; // Пустые клетки (цвета)
	} // for

	InitPiecePosition(); // Инициализация списка фигур белых/чёрных

	CurrentEvaluation = PositionEvaluation(); // Инициализация оценки текущей позиции

	// Инициализация флагов рокировки
	CastlingWhiteLeft = TRUE;
	CastlingWhiteRight = TRUE;
	CastlingBlackLeft = TRUE;
	CastlingBlackRight = TRUE;

	PassantIndex = -1; // Индекс клетки, через которую прошла пешка

	MoveNumber = 1; // Номер хода в партии
}

void LoadGame() // Загрузка партии из файла
{
	int i = 0;
	FILE *File;
	char ReadChar;
	char Str[3];

	File = fopen("chess.fen", "r");

	if (File == NULL) // File open error
	{
		printf("File 'chess.fen' open error!\n");

		exit(0);
	}

	// Загружаем фигуры
	while ((ReadChar = fgetc(File)) && ReadChar != ' ')
	{
		if (ReadChar == '/')
		{
			continue;
		}

		if (ReadChar >= '1' && ReadChar <= '8') // Пустые клетки
		{
			for (int j = 0; j < (ReadChar - 48); j++)
			{
				BoardPiece[i] = 0; // Пустые клетки (фигуры)
				BoardColor[i] = 0; // Пустые клетки (цвета)

				i++;
			} // for

			continue;
		}

		if (ReadChar == 'R' || ReadChar == 'r') // Ладья
		{
			BoardPiece[i] = Rook;
			BoardColor[i] = (ReadChar - 98) / 16;
		}
		else if (ReadChar == 'B' || ReadChar == 'b') // Слон
		{
			BoardPiece[i] = Bishop;
			BoardColor[i] = (ReadChar - 82) / 16;
		}
		else if (ReadChar == 'N' || ReadChar == 'n') // Конь
		{
			BoardPiece[i] = Knight;
			BoardColor[i] = (ReadChar - 94) / 16;
		}
		else if (ReadChar == 'Q' || ReadChar == 'q') // Ферзь
		{
			BoardPiece[i] = Queen;
			BoardColor[i] = (ReadChar - 97) / 16;
		}
		else if (ReadChar == 'K' || ReadChar == 'k') // Король
		{
			BoardPiece[i] = King;
			BoardColor[i] = (ReadChar - 91) / 16;
		}
		else if (ReadChar == 'P' || ReadChar == 'p') // Пешка
		{
			BoardPiece[i] = Pawn;
			BoardColor[i] = (ReadChar - 96) / 16;
		}

		i++;
	} // while

	InitPiecePosition(); // Инициализация списка фигур белых/чёрных

	CurrentEvaluation = PositionEvaluation(); // Инициализация оценки текущей позиции

	// Загружаем текущий цвет хода
	ReadChar = fgetc(File);

	if (ReadChar == 'w')
	{
		CurrentColor = WHITE; // Текущий цвет хода
	}
	else // 'b'
	{
		CurrentColor = BLACK; // Текущий цвет хода
	}

	// Загружаем флаги рокировки
	CastlingWhiteLeft = FALSE;
	CastlingWhiteRight = FALSE;
	CastlingBlackLeft = FALSE;
	CastlingBlackRight = FALSE;

	fgetc(File); // ' '

	while ((ReadChar = fgetc(File)) && ReadChar != '-' && ReadChar != ' ')
	{
		if (ReadChar == 'K')
		{
			CastlingWhiteRight = TRUE;
		}
		else if (ReadChar == 'Q')
		{
			CastlingWhiteLeft = TRUE;
		}
		else if (ReadChar == 'k')
		{
			CastlingBlackRight = TRUE;
		}
		else if (ReadChar == 'q')
		{
			CastlingBlackLeft = TRUE;
		}
	} // while

	// Загружаем индекс клетки, через которую прошла пешка
	fgetc(File); // ' '

	Str[0] = fgetc(File);

	if (Str[0] == '-')
	{
		PassantIndex = -1; // Индекс клетки, через которую прошла пешка
	}
	else
	{
		Str[1] = fgetc(File);
		Str[2] = '\0';

		PassantIndex = MoveToInt(Str); // Индекс клетки, через которую прошла пешка
	}

	fgetc(File); // ' '

	fgetc(File); // '0'

	// Загружаем номер хода в партии
	fscanf(File, "%d", &MoveNumber);

	fclose(File);
}

void SaveGame() // Сохранение партии в файле
{
	int k = 0; // Счётчик пустых клеток
	FILE *File;

	File = fopen("chess.fen", "w");

	if (File == NULL) // File open error
	{
		printf("File 'chess.fen' open error!\n");

		return;
	}

	// Сохраняем фигуры
	for (int i = 0; i < 64; i++)
	{
		if (i > 0 && !(i % 8))
		{
			if (k > 0)
			{
				fprintf(File, "%d", k);

				k = 0;
			}

			fprintf(File, "/");
		}

		if (BoardPiece[i] == 0) // Пустая клетка
		{
			k++;
		}
		else // Не пустая клетка
		{
			if (k > 0)
			{
				fprintf(File, "%d", k);

				k = 0;
			}
		}

		if (BoardPiece[i] == Rook) // Ладья
		{
			fprintf(File, "%c", (98 + BoardColor[i] * 16));
		}
		else if (BoardPiece[i] == Bishop) // Слон
		{
			fprintf(File, "%c", (82 + BoardColor[i] * 16));
		}
		else if (BoardPiece[i] == Knight) // Конь
		{
			fprintf(File, "%c", (94 + BoardColor[i] * 16));
		}
		else if (BoardPiece[i] == Queen) // Ферзь
		{
			fprintf(File, "%c", (97 + BoardColor[i] * 16));
		}
		else if (BoardPiece[i] == King) // Король
		{
			fprintf(File, "%c", (91 + BoardColor[i] * 16));
		}
		else if (BoardPiece[i] == Pawn) // Пешка
		{
			fprintf(File, "%c", (96 + BoardColor[i] * 16));
		}
	} // for

	if (k > 0)
	{
		fprintf(File, "%d", k);
	}

	// Сохраняем текущий цвет хода
	fprintf(File, " ");

	if (CurrentColor == WHITE)
	{
		fprintf(File, "w");
	}
	else // BLACK
	{
		fprintf(File, "b");
	}

	// Сохраняем флаги рокировки
	fprintf(File, " ");

	if (!CastlingWhiteRight && !CastlingWhiteLeft && !CastlingBlackRight && !CastlingBlackLeft)
	{
		fprintf(File, "-");
	}
	else
	{
		fprintf(File, (CastlingWhiteRight ? "K" : ""));
		fprintf(File, (CastlingWhiteLeft ? "Q" : ""));
		fprintf(File, (CastlingBlackRight ? "k" : ""));
		fprintf(File, (CastlingBlackLeft ? "q" : ""));
	}

	// Сохраняем индекс клетки, через которую прошла пешка
	fprintf(File, " ");

	if (PassantIndex == -1)
	{
		fprintf(File, "-");
	}
	else
	{
		fprintf(File, "%s", BoardName[PassantIndex]);
	}

	fprintf(File, " ");

	fprintf(File, "0");

	// Сохраняем номер хода в партии
	fprintf(File, " ");

	fprintf(File, "%d", MoveNumber); // Сохраняем номер хода в партии

	fclose(File);
}

int main()
{
	int Choice; // Выбранный пункт меню

	while (TRUE)
	{
		printf("\n");
		printf("Menu:\n");
		printf("1: New white game\n");
		printf("2: New black game\n");
		printf("3: New auto game\n");
		printf("4: Load game from file and white game\n");
		printf("5: Load game from file and black game\n");
		printf("6: Load game from file and auto game\n");
		printf("7: Exit\n");

		printf("Choice: ");
		scanf("%d", &Choice);

		switch (Choice)
		{
			case 1:
				InitNewGame(); // Инициализация новой партии
				Game(WHITE, BLACK); // Играем партию (белыми)
				break;

			case 2:
				InitNewGame(); // Инициализация новой партии
				Game(BLACK, WHITE); // Играем партию (чёрными)
				break;

			case 3:
				InitNewGame(); // Инициализация новой партии
				GameAuto(); // Играем партию (auto)
				break;

			case 4:
				LoadGame(); // Загружаем партию из файла
				Game(WHITE, BLACK); // Играем партию (белыми)
				break;

			case 5:
				LoadGame(); // Загружаем партию из файла
				Game(BLACK, WHITE); // Играем партию (чёрными)
				break;

			case 6:
				LoadGame(); // Загружаем партию из файла
				GameAuto(); // Играем партию (auto)
				break;

			case 7:
				exit(0);
		} // switch
	} // while
}