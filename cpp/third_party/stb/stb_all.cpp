// We need to have this inside a .cpp file, because tundra does not compile .h files.

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
