//Richard Farr (rfarr6)

#ifndef FIXED_MATH_H
#define FIXED_MATH_H

#define FLOAT_SIZE 16

#define INT2FIXED(n) ((n) << FLOAT_SIZE)
#define FIXED2INT(n) ((n) >> FLOAT_SIZE)
#define FIXED_ADD(x, y) ((x) + (y))
#define FIXED_SUBT(x, y) ((x) - (y))
#define FIXED_MULT(x, y) ((fixed32)(((int64)(x) * (int64)(y)) >> FLOAT_SIZE))
#define FIXED_DIV(x, y) ((fixed32)(((int64)(x) / (int64)(y)) << FLOAT_SIZE))

typedef long long int64;
typedef int fixed32;

#endif

