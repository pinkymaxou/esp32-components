#ifndef _MISC_FORMULA_H_
#define _MISC_FORMULA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Math.h"

double MISCFA_CircleDiff(double a, double b, double rotation);

int32_t MISCFA_CircleDiffd32(int32_t a, int32_t b, int32_t rotation);

double MISCFA_LinearizeLEDOutput(double dOfOne);

#ifdef __cplusplus
}
#endif

#endif