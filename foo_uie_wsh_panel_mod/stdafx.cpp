#include "stdafx.h"

// Define this to disable VLD
#define NO_VISUAL_LEAK_DETECTOR

#if defined(_DEBUG) && !defined(NO_VISUAL_LEAK_DETECTOR)
#include <vld.h>
#endif
