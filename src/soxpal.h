#pragma once

#include <inttypes.h>

namespace soxpal
{
	// 0 ... 12000 dBFS
	void dbfsToRgb(uint8_t bgr[3], uint16_t dbfs);
}
