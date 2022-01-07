#include "SlowTemplate.h"

inline int SlowFunc1() { return FibSlow_t<0,17>::value; }
inline int SlowFunc2() { return FibSlow_t<1,14>::value; }
