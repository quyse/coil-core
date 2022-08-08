#pragma once

namespace Coil::Utf8
{
  template <std::forward_iterator ByteIterator>
  class Char32Iterator
  {
  public:
    Char32Iterator(ByteIterator it)
    : _it(it) {}

    char32_t operator*() const
    {
      uint8_t b0 = *_it;
      if((b0 & 0x80) == 0x00) return b0;
      // check for invalid first UTF-8 byte
      if((b0 & 0xc0) == 0x80) return 0xfffd; // return replacement character
      auto it = _it;
      uint8_t b1 = *++it;
      if((b0 & 0xe0) == 0xc0) return ((char32_t)b0 & 0x1f) | ((char32_t)b1 & 0x3f) << 5;
      uint8_t b2 = *++it;
      if((b0 & 0xf0) == 0xe0) return ((char32_t)b0 & 0x0f) | ((char32_t)b1 & 0x3f) << 4 | ((char32_t)b2 & 0x3f) << 10;
      uint8_t b3 = *++it;
      if((b0 & 0xf8) == 0xf0) return ((char32_t)b0 & 0x07) | ((char32_t)b1 & 0x3f) << 3 | ((char32_t)b2 & 0x3f) << 9 | ((char32_t)b3 & 0x3f) << 15;
      return 0xfffd;
    }

    Char32Iterator& operator++()
    {
      // ignoring invalid UTF-8, just trying to find next first valid byte
      while((*++_it & 0xc0) == 0x80);
      return *this;
    }

    Char32Iterator operator++(int)
    {
      Char32Iterator i = *this;
      return std::move(++i);
    }

    using difference_type = ptrdiff_t;
    using value_type = char32_t;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

  private:
    ByteIterator _it;
  };
}
