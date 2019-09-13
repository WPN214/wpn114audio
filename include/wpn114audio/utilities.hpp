#pragma once

#define CSTR(_qstring) _qstring.toStdString().c_str()

enum class Interpolation { Linear = 0, Sin4 = 1 };

#define wpnwrap(_v, _limit) if (_v >= _limit) _v -= _limit
#define lininterp(_x,_a,_b) _a+_x*(_b-_a)
#define sininterp(_x,_a,_b) _a+ sin(x*(_b-_a)*(sample_t)M_PI_2)
