#pragma once

#include "platform.hpp"

namespace Coil
{
  class GraphicsDevice;

  class GraphicsSystem
  {
  public:
    virtual GraphicsDevice& CreateDefaultDevice(Book& book) = 0;
  };

  class GraphicsFrame
  {
  public:
    virtual void EndFrame() = 0;
  };

  class GraphicsPresenter
  {
  public:
    virtual void Resize(ivec2 const& size) = 0;
    virtual GraphicsFrame& StartFrame() = 0;
  };

  class GraphicsDevice
  {
  public:
    virtual GraphicsPresenter& CreateWindowPresenter(Book& book, Window& window) = 0;
  };
}
