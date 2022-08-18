#pragma once

#include "platform.hpp"
#include "graphics_format.hpp"

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

  // Function type for recreating resources for present pass.
  // Accepts special presenter book (which gets freed on next recreate), and pixel size of final frame.
  using GraphicsRecreatePresentPassFunc = void(Book&, ivec2 const&);

  class GraphicsDevice
  {
  public:
    virtual GraphicsPresenter& CreateWindowPresenter(Book& book, Window& window, std::function<GraphicsRecreatePresentPassFunc>&& recreatePresentPass) = 0;
  };
}
