module;

#include <gav1/decoder.h>
#include <string_view>

export module coil.core.video.av1;

import coil.core.base;
import coil.core.tasks;
import coil.core.video;

export namespace Coil
{
  class Av1DecodeStream final : public VideoStream
  {
  public:
    Av1DecodeStream(PacketInputStream& inputStream)
    : _inputStream(inputStream)
    {
      libgav1::DecoderSettings const settings =
      {
        .frame_parallel = false,
        .blocking_dequeue = false,
        .callback_private_data = this,
      };
      if(_decoder.Init(&settings) != libgav1::kStatusOk)
        throw Exception("initializing AV1 decoder failed");
    }

    // VideoStream's methods
    VideoFrame ReadFrame() override
    {
      for(;;)
      {
        libgav1::DecoderBuffer const* pDecoderBuffer = nullptr;
        switch(auto status = _decoder.DequeueFrame(&pDecoderBuffer))
        {
        case libgav1::kStatusOk:
          {
            VideoFrame frame =
            {
              .format = GetFormat(pDecoderBuffer->image_format),
              .colorRange = GetColorRange(pDecoderBuffer->color_range),
            };

            // check format (quite limited support for now)
            if(!(
              frame.format != VideoFrame::Format::Unknown &&
              frame.colorRange != VideoFrame::ColorRange::Unknown &&
              pDecoderBuffer->bitdepth == 8
            ))
            {
              throw Exception("AV1 decoded frame format is not supported");
            }

            for(size_t i = 0; i < 3; ++i)
            {
              frame.planes[i] =
              {
                .data = pDecoderBuffer->plane[i],
                .width = pDecoderBuffer->displayed_width[i],
                .height = pDecoderBuffer->displayed_height[i],
                .pitch = pDecoderBuffer->stride[i],
              };
            }
            return frame;
          }
        case libgav1::kStatusTryAgain:
        case libgav1::kStatusNothingToDequeue:
          break;
        default:
          throw Exception() << "AV1 frame dequeue error: " << libgav1::GetErrorString(status);
        }

        auto buffer = _inputStream.ReadPacket();
        if(!buffer) return {};

        if(auto status = _decoder.EnqueueFrame((uint8_t const*)buffer.data, buffer.size, 0, nullptr); status != libgav1::kStatusOk)
          throw Exception() << "AV1 frame enqueue error: " << libgav1::GetErrorString(status);
      }
    }

  private:
    static VideoFrame::Format GetFormat(libgav1::ImageFormat format)
    {
      switch(format)
      {
      case libgav1::kImageFormatYuv420:
        return VideoFrame::Format::YUV420;
      default:
        return VideoFrame::Format::Unknown;
      }
    }

    static VideoFrame::ColorRange GetColorRange(libgav1::ColorRange colorRange)
    {
      switch(colorRange)
      {
      case libgav1::kColorRangeStudio:
        return VideoFrame::ColorRange::Narrow;
      case libgav1::kColorRangeFull:
        return VideoFrame::ColorRange::Full;
      default:
        return VideoFrame::ColorRange::Unknown;
      }
    }

    PacketInputStream& _inputStream;
    libgav1::Decoder _decoder;
  };

  class Av1DecodeStreamSource final : public VideoStreamSource
  {
  public:
    Av1DecodeStreamSource(PacketInputStreamSource& inputStreamSource)
    : _inputStreamSource(inputStreamSource) {}

    Av1DecodeStream& CreateStream(Book& book) override
    {
      return book.Allocate<Av1DecodeStream>(_inputStreamSource.CreateStream(book));
    }

  private:
    PacketInputStreamSource& _inputStreamSource;
  };

  class Av1AssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<Av1DecodeStreamSource*, Asset>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      co_return &book.Allocate<Av1DecodeStreamSource>(
        *co_await assetContext.template LoadAssetParam<PacketInputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "av1";
  };
  static_assert(IsAssetLoader<Av1AssetLoader>);
}
