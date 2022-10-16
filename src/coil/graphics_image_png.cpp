#include "graphics_image_png.hpp"
#include <sstream>
#include <cstring>
#include <png.h>

namespace Coil
{
  void LoadPngImage(Buffer const& file, GraphicsImageFormat& format, std::vector<uint8_t>& data)
  {
    struct Loader
    {
      static void Error(png_structp png, png_const_charp errorMsg)
      {
        std::ostringstream& errorStream = *(std::ostringstream*)png_get_error_ptr(png);
        errorStream << "ERROR: " << errorMsg;
      }

      static void Warning(png_structp png, png_const_charp warningMsg)
      {
        std::ostringstream& errorStream = *(std::ostringstream*)png_get_error_ptr(png);
        errorStream << "WARNING: " << warningMsg;
      }

      static void Read(png_structp png, png_bytep data, png_size_t length)
      {
        Loader& loader = *(Loader*)png_get_io_ptr(png);
        if(loader.position + length > loader.size)
        {
          png_error(png, "not enough data to read");
          return;
        }
        memcpy(data, loader.data + loader.position, length);
        loader.position += length;
      }

      uint8_t const* data;
      size_t size;
      size_t position;
    };

    // check signature
    if(file.size < 8 || png_sig_cmp((png_const_bytep)file.data, 0, 8) != 0)
      throw Exception("wrong PNG signature");

    std::ostringstream errorStream;
    Loader loader =
    {
      .data = (uint8_t const*)file.data,
      .size = file.size,
      .position = 8,
    };

    // create read struct
    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, &errorStream, &Loader::Error, &Loader::Warning);
    if(!pngPtr)
      throw Exception("failed to create PNG read struct");
    // create info struct
    png_infop infoPtr = png_create_info_struct(pngPtr);
    if(!infoPtr)
    {
      png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
      throw Exception("failed to create PNG info struct");
    }

    // set error block
    if(setjmp(png_jmpbuf(pngPtr)))
    {
      // control gets here in case of error
      png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);
      throw Exception("failed to read PNG: ") << errorStream.str();
    }

    // set read function
    png_set_read_fn(pngPtr, &loader, &Loader::Read);
    png_set_sig_bytes(pngPtr, 8);

    // get image info
    png_read_info(pngPtr, infoPtr);
    png_uint_32 width = png_get_image_width(pngPtr, infoPtr);
    png_uint_32 height = png_get_image_height(pngPtr, infoPtr);
    png_byte bitDepth = png_get_bit_depth(pngPtr, infoPtr);
    png_byte colorType = png_get_color_type(pngPtr, infoPtr);

    // set format
    format =
    {
      .format = PixelFormats::uintRGB24S,
      .width = (int32_t)width,
      .height = (int32_t)height,
      .depth = 0,
      .mips = 1,
      .count = 0,
    };

    // set transformation options
    // convert palette images to RGB
    if(colorType == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(pngPtr);
    // convert grayscale with < 8 bit to 8 bit
    if((colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) && bitDepth < 8)
      png_set_expand_gray_1_2_4_to_8(pngPtr);
    // convert grayscale to RGB
    if(colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(pngPtr);
    // convert 16 bit depth to 8 bit
    if(bitDepth == 16)
      png_set_scale_16(pngPtr);
    // if there's alpha
    switch(colorType)
    {
    case PNG_COLOR_TYPE_GRAY_ALPHA:
    case PNG_COLOR_TYPE_RGB_ALPHA:
      // set format
      format.format = PixelFormats::uintRGBA32S;
      // set sRGB alpha mode
      png_set_alpha_mode(pngPtr, PNG_ALPHA_PNG, PNG_DEFAULT_sRGB);
      break;
    }

    // update info to account for conversions
    png_read_update_info(pngPtr, infoPtr);

    GraphicsImageMetrics metrics = format.GetMetrics();
    // allocate memory
    data.resize(format.width * format.height * metrics.pixelSize);
    // get image data
    {
      std::vector<png_bytep> imageRows(height);
      // get row pitch
      size_t pitch = format.width * metrics.pixelSize;
      for(png_uint_32 i = 0; i < height; ++i)
        imageRows[i] = (png_bytep)data.data() + i * pitch;
      png_read_image(pngPtr, imageRows.data());
    }
    // free struct
    png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);
  }
}
