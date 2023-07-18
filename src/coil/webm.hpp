#pragma once

#include "tasks.hpp"
#include <webm/webm_parser.h>

namespace Coil
{
  // TODO: move to separate library
  enum class MediaType : uint8_t
  {
    Unknown,
    Video,
    Audio,
    Subtitle,
  };

  class WebmTrackDecodeStream final : public PacketInputStream, private webm::Callback, private webm::Reader
  {
  public:
    WebmTrackDecodeStream(InputStream& inputStream, MediaType neededTrackType, size_t indexOfTrackWithNeededType = 0);

    // PacketInputStream's methods
    Buffer ReadPacket() override;

  private:
    // webm::Callback's methods
    webm::Status OnTrackEntry(webm::ElementMetadata const& metadata, webm::TrackEntry const& track_entry) override;
    webm::Status OnSimpleBlockBegin(webm::ElementMetadata const& metadata, webm::SimpleBlock const& simple_block, webm::Action* action) override;
    webm::Status OnBlockBegin(webm::ElementMetadata const& metadata, webm::Block const& block, webm::Action* action) override;
    webm::Status OnFrame(webm::FrameMetadata const& metadata, webm::Reader* reader, uint64_t* bytes_remaining) override;

    // webm::Reader's methods
    webm::Status Read(size_t num_to_read, uint8_t* buffer, uint64_t* num_actually_read) override;
    webm::Status Skip(uint64_t num_to_skip, uint64_t* num_actually_skipped) override;
    uint64_t Position() const override;

    // initial arguments
    InputStream& _inputStream;
    MediaType const _neededTrackType;
    size_t const _indexOfTrackWithNeededType;
    webm::WebmParser _parser;
    // position as a webm::Reader
    size_t _readerPosition = 0;
    // how many tracks of needed type we've seen already
    size_t _tracksOfNeededTypeCount = 0;
    // number of target track
    std::optional<uint64_t> _trackNumber;
    uint64_t _currentTrackNumber = 0;
    // current packet (empty if none)
    std::vector<uint8_t> _currentPacket;
    // whether end of file has been reached
    bool _eof = false;
  };

  class WebmTrackDecodeStreamSource final : public PacketInputStreamSource
  {
  public:
    WebmTrackDecodeStreamSource(InputStreamSource& inputStreamSource, MediaType trackType);

    // PacketInputStreamSource's methods
    WebmTrackDecodeStream& CreateStream(Book& book) override;

  private:
    InputStreamSource& _inputStreamSource;
    MediaType const _trackType;
  };

  class WebmTrackAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<WebmTrackDecodeStreamSource*, Asset>
    Task<Asset> LoadAsset(Book& book, AssetContext& assetContext) const
    {
      co_return &book.Allocate<WebmTrackDecodeStreamSource>(
        *co_await assetContext.template LoadAssetParam<InputStreamSource*>(book, "source"),
        assetContext.template GetFromStringParam<MediaType>("type")
      );
    }

    static constexpr std::string_view assetLoaderName = "webm_track";
  };
  static_assert(IsAssetLoader<WebmTrackAssetLoader>);

  template <>
  struct AssetTraits<WebmTrackDecodeStreamSource*>
  {
    static constexpr std::string_view assetTypeName = "webm_decode_track_stream_source";
  };
  static_assert(IsAsset<WebmTrackDecodeStreamSource*>);

  template <> MediaType FromString(std::string_view str);
}
