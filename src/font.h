#pragma once

#include <inttypes.h>

namespace font {

static const unsigned CHAR_WIDTH  = 9;
static const unsigned CHAR_HEIGHT = 16;

void getLine(char line[CHAR_WIDTH], char ch, int lineNo);

} // namespace font
