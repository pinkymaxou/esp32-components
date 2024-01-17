#include "misc-formula.h"

double MISCFA_CircleDiff(double a, double b, double rotation)
{
    const double halfRotation = rotation / 2;
    return fmod((a - b + rotation + halfRotation), rotation) - halfRotation;
}

double MISCFA_LinearizeLEDOutput(double dOfOne)
{
    return (pow(10.0d, dOfOne) - 1.0d) / 9.0d;
}