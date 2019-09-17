#pragma once
#include <wpn114audio/basic_types.hpp>
#include <cmath>

#define QCSTR(_qstring) _qstring.toStdString().c_str()
#define SMP_PI_2 static_cast<sample_t>(M_PI_2)

namespace wpn114 {
namespace audio  {

enum class Interpolation { Linear = 0, Sin4 = 1 };

template<typename T> inline constexpr void
wrap(T& value, T lim)  { if (value >= lim) value -= lim; }

template<typename T> inline constexpr T
lininterp(T x, T a, T b) { return a+x*(b-a); }

template<typename T> inline constexpr T
sininterp(T x, T a, T b) { return a + std::sin(x*(b-a) * SMP_PI_2); }
}
}

