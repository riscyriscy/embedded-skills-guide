#ifndef CLAMP_H
#define CLAMP_H

/* An inline function without static or extern. In C99 and later this is an inline definition only:
   the compiler may expand it, but it does not have to emit a function that other files can call. */
inline int clamp(int value, int high)
{
    return value > high ? high : value;
}

#endif
