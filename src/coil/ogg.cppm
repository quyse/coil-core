module;

#include <ogg/ogg.h>
#include <string_view>

export module coil.core.ogg;

import coil.core.base;

export namespace Coil
{
  class OggDecodeStream final : public PacketInputStream
  {
  public:
    OggDecodeStream(InputStream& inputStream)
    : _inputStream(inputStream)
    {
      ogg_sync_init(&_syncState);
    }

    ~OggDecodeStream()
    {
      ogg_stream_clear(&_streamState);
      ogg_sync_clear(&_syncState);
    }

    // PacketInputStream's methods
    Buffer ReadPacket() override
    {
      // loop until we get a packet
      for(;;)
      {
        // try to get packet
        ogg_packet packet = {};
        if(ogg_stream_packetout(&_streamState, &packet) > 0)
        {
          // skip header packets (with zero granule)
          if(!packet.granulepos)
            continue;
          return { packet.packet, (size_t)packet.bytes };
        }

        // loop until we get a page
        for(;;)
        {
          ogg_page page = {};
          if(ogg_sync_pageout(&_syncState, &page) > 0)
          {
            if(!_streamInitialized)
            {
              ogg_stream_init(&_streamState, ogg_page_serialno(&page));
              _streamInitialized = true;
            }

            if(ogg_stream_pagein(&_streamState, &page) != 0)
              throw Exception("decoding Ogg stream error");
            break;
          }

          // more data needed
          size_t const bufferSize = 4096;
          char* buffer = ogg_sync_buffer(&_syncState, 4096);
          if(!buffer)
            throw Exception("getting Ogg sync buffer failed");
          size_t readSize = _inputStream.Read(Buffer(buffer, bufferSize));
          if(ogg_sync_wrote(&_syncState, readSize) != 0)
            throw Exception("writing Ogg sync buffer failed");

          // if no more data, that's it
          if(!readSize)
            return {};
        }
      }
    }

  private:
    InputStream& _inputStream;
    ogg_sync_state _syncState = {};
    ogg_stream_state _streamState = {};
    bool _streamInitialized = false;
  };

  class OggDecodeStreamSource final : public PacketInputStreamSource
  {
  public:
    OggDecodeStreamSource(InputStreamSource& inputStreamSource)
    : _inputStreamSource(inputStreamSource) {}

    // PacketInputStreamSource's methods
    OggDecodeStream& CreateStream(Book& book) override
    {
      return book.Allocate<OggDecodeStream>(_inputStreamSource.CreateStream(book));
    }

  private:
    InputStreamSource& _inputStreamSource;
  };

  class OggAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<OggDecodeStreamSource*, Asset>
    Asset LoadAsset(Book& book, AssetContext& assetContext) const
    {
      return &book.Allocate<OggDecodeStreamSource>(
        *assetContext.template LoadAssetParam<InputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "ogg";
  };
  static_assert(IsAssetLoader<OggAssetLoader>);
}
