#include "ogg.hpp"

namespace Coil
{
  OggDecodeStream::OggDecodeStream(InputStream& inputStream)
  : _inputStream(inputStream)
  {
    ogg_sync_init(&_syncState);
  }

  OggDecodeStream::~OggDecodeStream()
  {
    ogg_stream_clear(&_streamState);
    ogg_sync_clear(&_syncState);
  }

  Buffer OggDecodeStream::ReadPacket()
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

  OggDecodeStreamSource::OggDecodeStreamSource(InputStreamSource& inputStreamSource)
  : _inputStreamSource(inputStreamSource) {}

  OggDecodeStream& OggDecodeStreamSource::CreateStream(Book& book)
  {
    return book.Allocate<OggDecodeStream>(_inputStreamSource.CreateStream(book));
  }
}
