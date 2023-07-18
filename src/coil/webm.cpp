#include "webm.hpp"
#include <unordered_map>

namespace Coil
{
  WebmTrackDecodeStream::WebmTrackDecodeStream(InputStream& inputStream, MediaType neededTrackType, size_t indexOfTrackWithNeededType)
  : _inputStream(inputStream), _neededTrackType(neededTrackType), _indexOfTrackWithNeededType(indexOfTrackWithNeededType) {}

  Buffer WebmTrackDecodeStream::ReadPacket()
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

  // WebmTrackDecodeStream's webm::Callback's methods
  webm::Status WebmTrackDecodeStream::OnTrackEntry(webm::ElementMetadata const& metadata, webm::TrackEntry const& track_entry)
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
  webm::Status WebmTrackDecodeStream::OnSimpleBlockBegin(webm::ElementMetadata const& metadata, webm::SimpleBlock const& simple_block, webm::Action* action)
  {
    _currentTrackNumber = simple_block.track_number;
    *action = webm::Action::kRead;
    return webm::Status(webm::Status::kOkCompleted);
  }
  webm::Status WebmTrackDecodeStream::OnBlockBegin(webm::ElementMetadata const& metadata, webm::Block const& block, webm::Action* action)
  {
    _currentTrackNumber = block.track_number;
    *action = webm::Action::kRead;
    return webm::Status(webm::Status::kOkCompleted);
  }
  webm::Status WebmTrackDecodeStream::OnFrame(webm::FrameMetadata const& metadata, webm::Reader* reader, uint64_t* bytes_remaining)
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

  // WebmTrackDecodeStream's webm::Reader's methods
  webm::Status WebmTrackDecodeStream::Read(size_t num_to_read, uint8_t* buffer, uint64_t* num_actually_read)
  {
    size_t size = _inputStream.Read(Buffer(buffer, num_to_read));
    if(num_actually_read) *num_actually_read = size;
    if(size <= 0) return webm::Status(webm::Status::kEndOfFile);
    _readerPosition += size;
    return webm::Status(size < num_to_read ? webm::Status::kOkPartial : webm::Status::kOkCompleted);
  }
  webm::Status WebmTrackDecodeStream::Skip(uint64_t num_to_skip, uint64_t* num_actually_skipped)
  {
    size_t size = _inputStream.Skip(num_to_skip);
    if(num_actually_skipped) *num_actually_skipped = size;
    if(size <= 0) return webm::Status(webm::Status::kEndOfFile);
    _readerPosition += size;
    return webm::Status(size < num_to_skip ? webm::Status::kOkPartial : webm::Status::kOkCompleted);
  }
  uint64_t WebmTrackDecodeStream::Position() const
  {
    return _readerPosition;
  }

  WebmTrackDecodeStreamSource::WebmTrackDecodeStreamSource(InputStreamSource& inputStreamSource, MediaType trackType)
  : _inputStreamSource(inputStreamSource), _trackType(trackType) {}

  WebmTrackDecodeStream& WebmTrackDecodeStreamSource::CreateStream(Book& book)
  {
    return book.Allocate<WebmTrackDecodeStream>(_inputStreamSource.CreateStream(book), _trackType);
  }

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
