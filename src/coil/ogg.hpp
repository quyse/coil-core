#pragma once

#include "base.hpp"
#include <ogg/ogg.h>

namespace Coil
{
  class OggDecodeStream
  {
  public:
    OggDecodeStream(InputStream& inputStream);
    ~OggDecodeStream();

    // read one packet from stream, empty buffer if EOF
    // buffer is valid only until next read
    Buffer ReadPacket();

  private:
    InputStream& _inputStream;
    ogg_sync_state _syncState = {};
    ogg_stream_state _streamState = {};
    bool _streamInitialized = false;
  };
}
