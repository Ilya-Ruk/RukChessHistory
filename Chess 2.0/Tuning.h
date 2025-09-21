// Tuning.h

#pragma once

#ifndef TUNING_H
#define TUNING_H

#include "Def.h"

#if defined(TUNING) && defined(TOGA_EVALUATION_FUNCTION)

void Pgn2Fen(void);

void FindBestK(void);

void InitTuningParams(void);
void LoadTuningParams(void);
void SaveTuningParams(void);

void TuningLocalSearch(void);

#endif // TUNING && TOGA_EVALUATION_FUNCTION

#endif // !TUNING_H