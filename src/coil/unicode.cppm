module;

#include <iterator>

export module coil.core.unicode;

export namespace Coil::Unicode
{
  // assuming char means UTF-8, char16_t means UTF-16, char32_t means UTF-32

  template <typename From, typename To, std::input_iterator FromIterator>
  class Iterator;

  // converting UTF-8 to UTF-32
  template <std::input_iterator FromIterator>
  class Iterator<char, char32_t, FromIterator>
  {
  public:
    Iterator(FromIterator const& it)
    : _it(it) {}
    Iterator(FromIterator&& it)
    : _it(std::move(it)) {}

    char32_t operator*() const
    {
      uint8_t b0 = *_it;
      if((b0 & 0x80) == 0x00) return b0;
      // check for invalid first UTF-8 byte
      if((b0 & 0xC0) == 0x80) return 0xFFFD; // return replacement character
      auto it = _it;
      uint8_t b1 = *++it;
      if((b0 & 0xE0) == 0xC0) return ((char32_t)b0 & 0x1F) << 6 | ((char32_t)b1 & 0x3F);
      uint8_t b2 = *++it;
      if((b0 & 0xF0) == 0xE0) return ((char32_t)b0 & 0x0F) << 12 | ((char32_t)b1 & 0x3F) << 6 | ((char32_t)b2 & 0x3F);
      uint8_t b3 = *++it;
      if((b0 & 0xF8) == 0xF0) return ((char32_t)b0 & 0x07) << 18 | ((char32_t)b1 & 0x3F) << 12 | ((char32_t)b2 & 0x3F) << 6 | ((char32_t)b3 & 0x3F);
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

    friend auto operator<=>(Iterator const&, Iterator const&) = default;

    using difference_type = ptrdiff_t;
    using value_type = char32_t;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

  private:
    FromIterator _it;
  };

  // converting UTF-32 to UTF-8
  template <std::input_iterator FromIterator>
  class Iterator<char32_t, char, FromIterator>
  {
  public:
    Iterator(FromIterator const& it)
    : _it(it) {}
    Iterator(FromIterator&& it)
    : _it(std::move(it)) {}

    char operator*() const
    {
      Update();
      char32_t c = *_it;
      switch(_size)
      {
      case 1: return c & 0x7F;
      case 2:
        switch(_octet)
        {
        case 0: return ((c >> 6) & 0x1F) | 0xC0;
        case 1: return (c & 0x3F) | 0x80;
        }
      case 3:
        switch(_octet)
        {
        case 0: return ((c >> 12) & 0x0F) | 0xE0;
        case 1: return ((c >> 6) & 0x3F) | 0x80;
        case 2: return (c & 0x3F) | 0x80;
        }
      case 4:
        switch(_octet)
        {
        case 0: return ((c >> 18) & 0x07) | 0xF0;
        case 1: return ((c >> 12) & 0x3F) | 0x80;
        case 2: return ((c >> 6) & 0x3F) | 0x80;
        case 3: return (c & 0x3F) | 0x80;
        }
      }
      return 0; // impossible
    }

    Iterator& operator++()
    {
      Update();
      ++_octet;
      if(_octet >= _size)
      {
        ++_it;
        _size = 0;
        _octet = 0;
      }
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator i = *this;
      ++(*this);
      return i;
    }

    friend auto operator<=>(Iterator const&, Iterator const&) = default;

    using difference_type = ptrdiff_t;
    using value_type = char;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

  private:
    void Update() const
    {
      if(_size) return;
      char32_t c = *_it;
      if(c < 0x80) _size = 1;
      else if(c < 0x800) _size = 2;
      else if(c < 0x10000) _size = 3;
      else _size = 4;
    }

    FromIterator _it;
    mutable uint8_t _size = 0;
    mutable uint8_t _octet = 0;
  };

  // converting UTF-16 to UTF-32
  template <std::input_iterator FromIterator>
  class Iterator<char16_t, char32_t, FromIterator>
  {
  public:
    Iterator(FromIterator const& it)
    : _it(it) {}
    Iterator(FromIterator&& it)
    : _it(std::move(it)) {}

    char32_t operator*() const
    {
      uint16_t w0 = *_it;
      // non-surrogate
      if(w0 < 0xD800 || w0 > 0xDFFF) return w0;

      // first code point cannot be trailing surrogate
      if(w0 >= 0xDC00) return 0xFFFD; // return replacement character

      auto it = _it;
      uint16_t w1 = *++it;

      // second code point must be trailing surrogate
      if(w1 < 0xDC00 || w1 > 0xDFFF) return 0xFFFD; // return replacement character

      return 0x10000 + ((char32_t)(w0 - 0xD800) << 10) + (char32_t)(w1 - 0xDC00);
    }

    Iterator& operator++()
    {
      // ignoring trailing surrogates, just trying to find next first valid word
      while((*++_it & 0xFC00) == 0xDC00);
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator i = *this;
      ++(*this);
      return i;
    }

    friend auto operator<=>(Iterator const&, Iterator const&) = default;

    using difference_type = ptrdiff_t;
    using value_type = char32_t;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

  private:
    FromIterator _it;
  };

  // converting UTF-32 to UTF-16
  template <std::input_iterator FromIterator>
  class Iterator<char32_t, char16_t, FromIterator>
  {
  public:
    Iterator(FromIterator const& it)
    : _it(it) {}
    Iterator(FromIterator&& it)
    : _it(std::move(it)) {}

    char16_t operator*() const
    {
      uint32_t c = *_it;
      // BMP code point
      if(c <= 0xFFFF) return (char16_t)(uint16_t)c;

      c -= 0x10000;
      return (char16_t)(_secondSurrogatePair ? (0xDC00 | (c & 0x3FF)) : (0xD800 | ((c >> 10) & 0x3FF)));
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

    friend auto operator<=>(Iterator const&, Iterator const&) = default;

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

  // conversion from iterator pair into container with back-inserter
  template <typename From, typename To, std::input_iterator FromIterator, typename Container>
  void Convert(FromIterator begin, FromIterator end, Container& r)
  {
    std::back_insert_iterator j(r);
    Iterator<From, To, FromIterator> iend = end;
    for(Iterator<From, To, FromIterator> i = begin; i < iend; ++i)
      *j++ = *i;
  }
  // conversion from iterator to null-terminated string into container with back-inserter
  template <typename From, typename To, std::input_iterator FromIterator, typename Container>
  void Convert(FromIterator begin, Container& r)
  {
    std::back_insert_iterator j(r);
    for(Iterator<From, To, FromIterator> i = begin; *i; ++i)
      *j++ = *i;
  }
}
