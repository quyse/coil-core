module;

#include <concepts>

export module coil.core.crypto.base;

import coil.core.base;

export namespace Coil
{
  template <typename HashAlgorithm>
  concept IsHashAlgorithm = requires(HashAlgorithm state, Buffer const& buffer)
  {
    { HashAlgorithm{} };
    { state.Feed(buffer) };
    { state.Finish() } -> std::same_as<typename HashAlgorithm::Hash>;
  };

  // one-time hash stream
  template <IsHashAlgorithm HashAlgorithm>
  class HashStream final : public OutputStream
  {
  public:
    void Write(Buffer const& buffer) override
    {
      _state.Feed(buffer);
    }

    typename HashAlgorithm::Hash Finish()
    {
      return _state.Finish();
    }

  private:
    HashAlgorithm _state;
  };

  // shortcut for calculating hash of a buffer
  template <IsHashAlgorithm HashAlgorithm>
  typename HashAlgorithm::Hash CalculateHash(Buffer const& buffer)
  {
    HashStream<HashAlgorithm> stream;
    stream.Write(buffer);
    return stream.Finish();
  }
}
