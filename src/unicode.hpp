#pragma once

#include <iterator>

namespace Coil::Unicode
{
  // assuming char means UTF-8, char16_t means UTF-16, char32_t means Unicode code points

  template <typename From, typename To, std::input_iterator FromIterator>
  class Iterator;

  // converting UTF8 to Unicode code points
  template <std::input_iterator FromIterator>
  class Iterator<char, char32_t, FromIterator>
  {
  public:
    Iterator(FromIterator it)
    : _it(it) {}

    char32_t operator*() const
    {
      uint8_t b0 = *_it;
      if((b0 & 0x80) == 0x00) return b0;
      // check for invalid first UTF-8 byte
      if((b0 & 0xC0) == 0x80) return 0xFFFD; // return replacement character
      auto it = _it;
      uint8_t b1 = *++it;
      if((b0 & 0xE0) == 0xC0) return ((char32_t)b0 & 0x1F) | ((char32_t)b1 & 0x3F) << 5;
      uint8_t b2 = *++it;
      if((b0 & 0xF0) == 0xE0) return ((char32_t)b0 & 0x0F) | ((char32_t)b1 & 0x3F) << 4 | ((char32_t)b2 & 0x3F) << 10;
      uint8_t b3 = *++it;
      if((b0 & 0xF8) == 0xF0) return ((char32_t)b0 & 0x07) | ((char32_t)b1 & 0x3F) << 3 | ((char32_t)b2 & 0x3F) << 9 | ((char32_t)b3 & 0x3F) << 15;
      return 0xFFFD;
    }

    Iterator& operator++()
    {
      // ignoring invalid UTF-8, just trying to find next first valid byte
      while((*++_it & 0xC0) == 0x80);
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator i = *this;
      ++(*this);
      return i;
    }

    using difference_type = ptrdiff_t;
    using value_type = char32_t;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

  private:
    FromIterator _it;
  };

  // converting Unicode code points to UTF16
  template <std::input_iterator FromIterator>
  class Iterator<char32_t, char16_t, FromIterator>
  {
  public:
    Iterator(FromIterator it)
    : _it(it) {}

    char16_t operator*() const
    {
      char32_t c = *_it;
      // BMP code point
      if(c <= 0xFFFF) return (char16_t)c;

      c -= 0x10000;
      return _secondSurrogatePair ? 0xDC00 | (c & 0x3FF) : 0xD800 | ((c >> 10) & 0x3FF);
    }

    Iterator& operator++()
    {
      if(*_it <= 0xFFFF || _secondSurrogatePair)
      {
        ++_it;
        _secondSurrogatePair = false;
      }
      else
      {
        _secondSurrogatePair = true;
      }
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator i = *this;
      ++(*this);
      return i;
    }

    using difference_type = ptrdiff_t;
    using value_type = char16_t;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

  private:
    FromIterator _it;
    bool _secondSurrogatePair = false;
  };

  // generic chained conversion: From -> char32_t -> To
  template <typename From, typename To, std::input_iterator FromIterator>
  class Iterator : public Iterator<char32_t, To, Iterator<From, char32_t, FromIterator>>
  {
  public:
    Iterator(FromIterator it)
    : Iterator<char32_t, To, Iterator<From, char32_t, FromIterator>>(it) {}
  };

  // conversion from iterator into container with back-inserter
  template <typename From, typename To, std::input_iterator FromIterator, typename Container>
  void Convert(FromIterator s, Container& r)
  {
    Iterator<From, To, FromIterator> i(s);
    std::back_insert_iterator j(r);
    for(;;)
    {
      To c = *i++;
      *j++ = c;
      if(!c) break;
    }
  }
}
