#pragma once

#include "tasks.hpp"
#include <ogg/ogg.h>

namespace Coil
{
  class OggDecodeStream final : public PacketInputStream
  {
  public:
    OggDecodeStream(InputStream& inputStream);
    ~OggDecodeStream();

    // PacketInputStream's methods
    Buffer ReadPacket() override;

  private:
    InputStream& _inputStream;
    ogg_sync_state _syncState = {};
    ogg_stream_state _streamState = {};
    bool _streamInitialized = false;
  };

  class OggDecodeStreamSource final : public PacketInputStreamSource
  {
  public:
    OggDecodeStreamSource(InputStreamSource& _inputStreamSource);

    // PacketInputStreamSource's methods
    OggDecodeStream& CreateStream(Book& book) override;

  private:
    InputStreamSource& _inputStreamSource;
  };

  class OggAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<OggDecodeStreamSource*, Asset>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      co_return &book.Allocate<OggDecodeStreamSource>(
        *co_await assetContext.template LoadAssetParam<InputStreamSource*>(book, "source")
      );
    }

    static constexpr std::string_view assetLoaderName = "ogg";
  };
  static_assert(IsAssetLoader<OggAssetLoader>);
}
