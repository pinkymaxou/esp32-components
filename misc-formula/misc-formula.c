#include "misc-formula.h"

double MISCFA_CircleDiff(double a, double b, double rotation)
{
    const double halfRotation = rotation / 2;
    return fmod((a - b + rotation + halfRotation), rotation) - halfRotation;
}

int32_t MISCFA_CircleDiffd32(int32_t a, int32_t b, int32_t rotation)
{
    const int32_t halfRotation = rotation / 2;
    return fmod((a - b + rotation + halfRotation), rotation) - halfRotation;
}

double MISCFA_LinearizeLEDOutput(double of_one)
{
    return (pow(10.0d, of_one) - 1.0d) / 9.0d;
}