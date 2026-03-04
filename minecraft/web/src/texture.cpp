#include <fstream>
#include <string>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <png.h>

#include "AppPlatform.h"
#include "NinecraftApp.h"
#include "client/gui/screens/DialogDefinitions.h"
#include "client/renderer/TextureData.h"
#include "platform/input/Keyboard.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"

static void png_funcReadFile(png_structp pngPtr, png_bytep data, png_size_t length) {
  ((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}

TextureData loadTextureHack(const std::string &file, bool folder) {
  TextureData out;

  std::string filename = folder ? "/data/images/" + file : file;
  std::ifstream source(filename.c_str(), std::ios::binary);
  if (source) {
    png_structp pngPtr =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (pngPtr) {
      png_infop infoPtr = png_create_info_struct(pngPtr);
      if (infoPtr) {
        png_set_read_fn(pngPtr, (void *)&source, png_funcReadFile);
        png_read_info(pngPtr, infoPtr);

        int color_type = png_get_color_type(pngPtr, infoPtr);
        int bit_depth = png_get_bit_depth(pngPtr, infoPtr);
        if (bit_depth == 16)
          png_set_strip_16(pngPtr);
        if (color_type == PNG_COLOR_TYPE_PALETTE)
          png_set_palette_to_rgb(pngPtr);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
          png_set_expand_gray_1_2_4_to_8(pngPtr);
        if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
          png_set_tRNS_to_alpha(pngPtr);
        if (color_type == PNG_COLOR_TYPE_RGB ||
            color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_PALETTE)
          png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER);
        if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb(pngPtr);

        png_read_update_info(pngPtr, infoPtr);

        out.w = png_get_image_width(pngPtr, infoPtr);
        out.h = png_get_image_height(pngPtr, infoPtr);
        out.data = new unsigned char[4 * out.w * out.h];
        out.memoryHandledExternally = false;

        png_bytep *rowPtrs = new png_bytep[out.h];
        int rowStrideBytes = 4 * out.w;
        for (int i = 0; i < out.h; i++)
          rowPtrs[i] = (png_bytep)&out.data[i * rowStrideBytes];

        png_read_image(pngPtr, rowPtrs);
        delete[] rowPtrs;
        png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
        source.close();
        return out;
      }
      png_destroy_read_struct(&pngPtr, NULL, NULL);
    }
  }

  out.w = 64;
  out.h = 64;
  out.data = new unsigned char[4 * out.w * out.h];
  out.memoryHandledExternally = false;

  unsigned hash = 2166136261u;
  for (char c : file)
    hash = (hash ^ static_cast<unsigned char>(c)) * 16777619u;

  for (int y = 0; y < out.h; ++y)
    for (int x = 0; x < out.w; ++x) {
      const int i = (y * out.w + x) * 4;
      const bool check = ((x >> 3) ^ (y >> 3)) & 1;
      out.data[i + 0] = check ? static_cast<unsigned char>((hash >> 16) & 0xff) : 30;
      out.data[i + 1] = check ? static_cast<unsigned char>((hash >> 8) & 0xff) : 30;
      out.data[i + 2] = check ? static_cast<unsigned char>(hash & 0xff) : 30;
      out.data[i + 3] = 255;
    }

  return out;
}
