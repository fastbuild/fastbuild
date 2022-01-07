#include "SlowTemplate.h"

inline int SlowFunc1() { return FibSlow_t<0,18>::value; }
