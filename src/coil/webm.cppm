module;

#include <webm/webm_parser.h>
#include <optional>
#include <unordered_map>

export module coil.core.media.webm;

import coil.core.base;

export namespace Coil
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
    WebmTrackDecodeStream(InputStream& inputStream, MediaType neededTrackType, size_t indexOfTrackWithNeededType = 0)
    : _inputStream(inputStream), _neededTrackType(neededTrackType), _indexOfTrackWithNeededType(indexOfTrackWithNeededType) {}

    // PacketInputStream's methods
    Buffer ReadPacket() override
    {
      _currentPacket.clear();
      if(_eof) return {};
      switch(_parser.Feed(this, this).code)
      {
      case webm::Status::kOkCompleted:
        _eof = true;
        [[fallthrough]];
      case webm::Status::kOkPartial:
        break;
      default:
        throw Exception("decoding WebM track failed");
      }
      return _currentPacket;
    }

  private:
    // webm::Callback's methods

    webm::Status OnTrackEntry(webm::ElementMetadata const& metadata, webm::TrackEntry const& track_entry) override
    {
      MediaType trackType;
      switch(track_entry.track_type.value())
      {
      case webm::TrackType::kVideo:
        trackType = MediaType::Video;
        break;
      case webm::TrackType::kAudio:
        trackType = MediaType::Audio;
        break;
      case webm::TrackType::kSubtitle:
        trackType = MediaType::Subtitle;
        break;
      default:
        trackType = MediaType::Unknown;
        break;
      }

      if(trackType == _neededTrackType && _tracksOfNeededTypeCount++ == _indexOfTrackWithNeededType)
      {
        // found the track
        _trackNumber = track_entry.track_number.value();
      }

      return webm::Status(webm::Status::kOkCompleted);
    }

    webm::Status OnSimpleBlockBegin(webm::ElementMetadata const& metadata, webm::SimpleBlock const& simple_block, webm::Action* action) override
    {
      _currentTrackNumber = simple_block.track_number;
      *action = webm::Action::kRead;
      return webm::Status(webm::Status::kOkCompleted);
    }

    webm::Status OnBlockBegin(webm::ElementMetadata const& metadata, webm::Block const& block, webm::Action* action) override
    {
      _currentTrackNumber = block.track_number;
      *action = webm::Action::kRead;
      return webm::Status(webm::Status::kOkCompleted);
    }

    webm::Status OnFrame(webm::FrameMetadata const& metadata, webm::Reader* reader, uint64_t* bytes_remaining) override
    {
      // delay processing frame if there's current packet
      if(!_currentPacket.empty()) return webm::Status(webm::Status::kOkPartial);

      bool isCorrectTrack = (_trackNumber.has_value() && _trackNumber.value() == _currentTrackNumber);
      if(isCorrectTrack)
      {
        _currentPacket.resize(*bytes_remaining);
      }

      webm::Status status(webm::Status::kOkCompleted);

      // read or skip the packet
      size_t position = 0;
      do
      {
        uint64_t size;
        status = isCorrectTrack
          ? reader->Read(*bytes_remaining, (uint8_t*)_currentPacket.data() + position, &size)
          : reader->Skip(*bytes_remaining, &size)
        ;
        position += size;
        *bytes_remaining -= size;
      }
      while(status.code == webm::Status::kOkPartial);

      if(isCorrectTrack)
      {
        _currentPacket.resize(position);
      }

      return status;
    }

    // webm::Reader's methods

    webm::Status Read(size_t num_to_read, uint8_t* buffer, uint64_t* num_actually_read) override
    {
      size_t size = _inputStream.Read(Buffer(buffer, num_to_read));
      if(num_actually_read) *num_actually_read = size;
      if(size <= 0) return webm::Status(webm::Status::kEndOfFile);
      _readerPosition += size;
      return webm::Status(size < num_to_read ? webm::Status::kOkPartial : webm::Status::kOkCompleted);
    }

    webm::Status Skip(uint64_t num_to_skip, uint64_t* num_actually_skipped) override
    {
      size_t size = _inputStream.Skip(num_to_skip);
      if(num_actually_skipped) *num_actually_skipped = size;
      if(size <= 0) return webm::Status(webm::Status::kEndOfFile);
      _readerPosition += size;
      return webm::Status(size < num_to_skip ? webm::Status::kOkPartial : webm::Status::kOkCompleted);
    }

    uint64_t Position() const override
    {
      return _readerPosition;
    }

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
    WebmTrackDecodeStreamSource(InputStreamSource& inputStreamSource, MediaType trackType)
    : _inputStreamSource(inputStreamSource), _trackType(trackType) {}

    // PacketInputStreamSource's methods

    WebmTrackDecodeStream& CreateStream(Book& book) override
    {
      return book.Allocate<WebmTrackDecodeStream>(_inputStreamSource.CreateStream(book), _trackType);
    }

  private:
    InputStreamSource& _inputStreamSource;
    MediaType const _trackType;
  };

  class WebmTrackAssetLoader
  {
  public:
    template <typename Asset, typename AssetContext>
    requires std::convertible_to<WebmTrackDecodeStreamSource*, Asset>
    Asset LoadAsset(Book& book, AssetContext& assetContext) const
    {
      return &book.Allocate<WebmTrackDecodeStreamSource>(
        *assetContext.template LoadAssetParam<InputStreamSource*>(book, "source"),
        assetContext.template GetFromStringParam<MediaType>("type")
      );
    }

    static constexpr std::string_view assetLoaderName = "webm_track";
  };
  static_assert(IsAssetLoader<WebmTrackAssetLoader>);

  template <> MediaType FromString(std::string_view str)
  {
    static std::unordered_map<std::string_view, MediaType> const values =
    {{
      { "Video", MediaType::Video },
      { "Audio", MediaType::Audio },
      { "Subtitle", MediaType::Subtitle },
    }};
    auto i = values.find(str);
    if(i == values.end())
      return MediaType::Unknown;
    return i->second;
  }
}
