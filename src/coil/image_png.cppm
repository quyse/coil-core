module;

#include <sstream>
#include <vector>
#include <png.h>

export module coil.core.image.png;

import coil.core.base;
import coil.core.data;
import coil.core.image.format;

namespace
{
  struct Helper
  {
    static void Error(png_structp png, png_const_charp errorMsg)
    {
      auto& helper = *(Helper*)png_get_error_ptr(png);
      helper.errorStream << "ERROR: " << errorMsg;
    }

    static void Warning(png_structp png, png_const_charp warningMsg)
    {
      auto& helper = *(Helper*)png_get_error_ptr(png);
      helper.errorStream << "WARNING: " << warningMsg;
    }

    std::ostringstream errorStream;
  };
}

export namespace Coil
{
  ImageBuffer LoadPngImage(Book& book, InputStream& stream)
  {
    class LoadHelper : public Helper
    {
    public:
      LoadHelper(InputStream& stream)
      : _stream(stream) {}

      static void Read(png_structp png, png_bytep data, png_size_t length)
      {
        LoadHelper& helper = *(LoadHelper*)png_get_io_ptr(png);
        if(helper._stream.Read(Buffer(data, length)) < length)
        {
          png_error(png, "not enough data to read");
          return;
        }
      }

    private:
      InputStream& _stream;
    };

    // check signature
    {
      uint8_t signature[8];
      if(stream.Read(Buffer(signature, sizeof(signature))) != sizeof(signature) || png_sig_cmp(signature, 0, 8) != 0)
        throw Exception("wrong PNG signature");
    }

    LoadHelper helper(stream);

    // create read struct
    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, &helper, &LoadHelper::Error, &LoadHelper::Warning);
    if(!pngPtr)
      throw Exception("failed to create PNG read struct");
    // create info struct
    png_infop infoPtr = png_create_info_struct(pngPtr);
    if(!infoPtr)
    {
      png_destroy_read_struct(&pngPtr, nullptr, nullptr);
      throw Exception("failed to create PNG info struct");
    }

    // set error block
    if(setjmp(png_jmpbuf(pngPtr)))
    {
      // control gets here in case of error
      png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
      throw Exception("failed to read PNG: ") << helper.errorStream.str();
    }

    // set read function
    png_set_read_fn(pngPtr, &helper, &LoadHelper::Read);
    png_set_sig_bytes(pngPtr, 8);

    // get image info
    png_read_info(pngPtr, infoPtr);
    png_uint_32 width = png_get_image_width(pngPtr, infoPtr);
    png_uint_32 height = png_get_image_height(pngPtr, infoPtr);
    png_byte bitDepth = png_get_bit_depth(pngPtr, infoPtr);
    png_byte colorType = png_get_color_type(pngPtr, infoPtr);

    // set format
    ImageBuffer imageBuffer =
    {
      .format =
      {
        .format = PixelFormats::uintRGB24S,
        .width = (int32_t)width,
        .height = (int32_t)height,
        .depth = 0,
        .mips = 1,
        .count = 0,
      },
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
      imageBuffer.format.format = PixelFormats::uintRGBA32S;
      // set sRGB alpha mode
      png_set_alpha_mode(pngPtr, PNG_ALPHA_PNG, PNG_DEFAULT_sRGB);
      break;
    }

    // update info to account for conversions
    png_read_update_info(pngPtr, infoPtr);

    auto metrics = imageBuffer.format.GetMetrics();
    // allocate memory
    imageBuffer.buffer = book.Allocate<std::vector<uint8_t>>(imageBuffer.format.width * imageBuffer.format.height * metrics.pixelSize);
    // get image data
    {
      std::vector<png_bytep> imageRows(height);
      // get row pitch
      size_t pitch = imageBuffer.format.width * metrics.pixelSize;
      for(png_uint_32 i = 0; i < height; ++i)
        imageRows[i] = (png_bytep)imageBuffer.buffer.data + i * pitch;
      png_read_image(pngPtr, imageRows.data());
    }
    // free struct
    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

    return std::move(imageBuffer);
  }

  void SavePngImage(OutputStream& stream, ImageBuffer const& imageBuffer)
  {
    class SaveHelper : public Helper
    {
    public:
      SaveHelper(OutputStream& stream)
      : _stream(stream) {}

      static void Write(png_structp png, png_bytep data, png_size_t length)
      {
        auto& helper = *(SaveHelper*)png_get_io_ptr(png);
        helper._stream.Write(Buffer(data, length));
      }

      static void Flush(png_structp png)
      {
      }

    private:
      OutputStream& _stream;
    };

    SaveHelper helper(stream);

    // create write struct
    png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &helper, &SaveHelper::Error, &SaveHelper::Warning);
    if(!pngPtr)
      throw Exception("failed to create PNG write struct");
    // create info struct
    png_infop infoPtr = png_create_info_struct(pngPtr);
    if(!infoPtr)
    {
      png_destroy_write_struct(&pngPtr, nullptr);
      throw Exception("failed to create PNG info struct");
    }

    // set error block
    if(setjmp(png_jmpbuf(pngPtr)))
    {
      // control gets here in case of error
      png_destroy_write_struct(&pngPtr, &infoPtr);
      throw Exception("failed to write PNG: ") << helper.errorStream.str();
    }

    // set write function
    png_set_write_fn(pngPtr, &helper, &SaveHelper::Write, &SaveHelper::Flush);

    auto metrics = imageBuffer.format.GetMetrics();

    uint8_t componentsCount = 0;
    png_byte colorType;
    switch(imageBuffer.format.format.components)
    {
    case PixelFormat::Components::R:
      componentsCount = 1;
      colorType = PNG_COLOR_TYPE_GRAY;
      break;
    case PixelFormat::Components::RG:
      componentsCount = 2;
      colorType = PNG_COLOR_TYPE_RGB;
      break;
    case PixelFormat::Components::RGB:
      componentsCount = 3;
      colorType = PNG_COLOR_TYPE_RGB;
      break;
    case PixelFormat::Components::RGBA:
      componentsCount = 4;
      colorType = PNG_COLOR_TYPE_RGB_ALPHA;
      break;
    }
    size_t pixelBitSize = metrics.pixelSize * 8;
    size_t bitDepth = 0;
    if(componentsCount && pixelBitSize % componentsCount == 0)
    {
      bitDepth = pixelBitSize / componentsCount;
    }

    png_set_IHDR(pngPtr, infoPtr,
      imageBuffer.format.width,
      imageBuffer.format.height,
      bitDepth,
      colorType,
      PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(pngPtr, infoPtr);

    // write image data
    {
      std::vector<png_bytep> imageRows(imageBuffer.format.height);
      // get row pitch
      int32_t pitch = imageBuffer.format.width * metrics.pixelSize;
      for(int32_t i = 0; i < imageBuffer.format.height; ++i)
        imageRows[i] = (uint8_t*)imageBuffer.buffer.data + i * pitch;
      png_write_image(pngPtr, imageRows.data());
    }

    png_write_end(pngPtr, nullptr);

    png_destroy_write_struct(&pngPtr, &infoPtr);
  }

  class PngAssetLoader
  {
  public:
    template <std::same_as<ImageBuffer> Asset, typename AssetContext>
    Asset LoadAsset(Book& book, AssetContext& assetContext) const
    {
      BufferInputStream inputStream = assetContext.template LoadAssetParam<Buffer>(book, "buffer");
      return LoadPngImage(book, inputStream);
    }

    static constexpr std::string_view assetLoaderName = "image_png";
  };
  static_assert(IsAssetLoader<PngAssetLoader>);
}
