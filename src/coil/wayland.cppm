module;

#include <wayland-client.h>
#include <wayland-xdg-shell-client-protocol.h>
#include <wayland-pointer-constraints-unstable-v1-client-protocol.h>
#include <wayland-relative-pointer-unstable-v1-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <poll.h>
#include <functional>
#include <string>

export module coil.core.wayland;

import coil.core.base;
import coil.core.input;
import coil.core.math;
import coil.core.platform;

// helper objects
namespace
{
  // event handler binding to class member
  template <typename F>
  struct WaylandEventBase;
  template <typename Object, typename... Args>
  struct WaylandEventBase<void (Object::*)(Args...)>
  {
    template <void (Object::*handler)(Args...)>
    static void Handler(void* data, Args... args)
    {
      (((Object*)data)->*handler)(args...);
    }
  };
  template <auto handler>
  struct WaylandEvent
  {
    static constexpr auto Handler = &WaylandEventBase<decltype(handler)>::template Handler<handler>;
  };
}

export namespace Coil
{
  void DestroyWaylandObject(wl_buffer* object)             { wl_buffer_destroy(object);             }
  void DestroyWaylandObject(wl_display* object)            { wl_display_disconnect(object);         }
  void DestroyWaylandObject(wl_keyboard* object)           { wl_keyboard_release(object);           }
  void DestroyWaylandObject(wl_output* object)             { wl_output_release(object);             }
  void DestroyWaylandObject(wl_pointer* object)            { wl_pointer_release(object);            }
  void DestroyWaylandObject(wl_registry* object)           { wl_registry_destroy(object);           }
  void DestroyWaylandObject(wl_shm_pool* object)           { wl_shm_pool_destroy(object);           }
  void DestroyWaylandObject(wl_surface* object)            { wl_surface_destroy(object);            }
  void DestroyWaylandObject(xdg_surface* object)           { xdg_surface_destroy(object);           }
  void DestroyWaylandObject(xdg_toplevel* object)          { xdg_toplevel_destroy(object);          }
  void DestroyWaylandObject(zwp_locked_pointer_v1* object) { zwp_locked_pointer_v1_destroy(object); }

  template <typename T>
  class WaylandObject
  {
  public:
    WaylandObject(T* object = nullptr)
    : _object(object) {}
    WaylandObject(WaylandObject const&) = delete;
    WaylandObject(WaylandObject&& other)
    {
      std::swap(_object, other._object);
    }
    WaylandObject& operator=(WaylandObject const&) = delete;
    WaylandObject& operator=(WaylandObject&& other)
    {
      std::swap(_object, other._object);
      return *this;
    }

    ~WaylandObject()
    {
      if(_object)
      {
        DestroyWaylandObject(_object);
      }
    }

    operator T*() const
    {
      return _object;
    }

  private:
    T* _object = nullptr;
  };

  class WaylandWindow;
  class WaylandWindowSystem;

  class WaylandInputManager final : public InputManager
  {
  private:
    vec2 _scale = { 1, 1 };
    ivec2 _lastCursor;

    friend class WaylandWindow;
  };

  class WaylandWindow final : public Window
  {
  public:
    WaylandWindow(WaylandWindowSystem& windowSystem, WaylandObject<wl_surface>&& surface, WaylandObject<xdg_surface>&& xdgSurface, WaylandObject<xdg_toplevel>&& xdgToplevel, std::string const& title, ivec2 const& size)
    : _windowSystem(windowSystem), _surface(std::move(surface)), _xdgSurface(std::move(xdgSurface)), _xdgToplevel(std::move(xdgToplevel))
    {
      wl_surface_set_user_data(_surface, this);

      {
        static constinit wl_surface_listener const listener =
        {
          .enter = WaylandEvent<&WaylandWindow::OnSurfaceEnter>::Handler,
          .leave = WaylandEvent<&WaylandWindow::OnSurfaceLeave>::Handler,
        };
        wl_surface_add_listener(_surface, &listener, this);
      }
      {
        static constinit xdg_surface_listener const listener =
        {
          .configure = WaylandEvent<&WaylandWindow::OnXdgSurfaceConfigure>::Handler,
        };
        xdg_surface_add_listener(_xdgSurface, &listener, this);
      }
      {
        static constinit xdg_toplevel_listener const listener =
        {
          .configure        = WaylandEvent<&WaylandWindow::OnXdgToplevelConfigure      >::Handler,
          .close            = WaylandEvent<&WaylandWindow::OnXdgToplevelClose          >::Handler,
          .configure_bounds = WaylandEvent<&WaylandWindow::OnXdgToplevelConfigureBounds>::Handler,
        };
        xdg_toplevel_add_listener(_xdgToplevel, &listener, this);
      }

      xdg_toplevel_set_title(_xdgToplevel, title.c_str());
      wl_surface_commit(_surface);
    }

    void SetTitle(std::string const& title) override
    {
      xdg_toplevel_set_title(_xdgToplevel, title.c_str());
    }

    void Close() override
    {
      Stop();
    }

    void SetFullScreen(bool fullScreen) override
    {
      if(fullScreen)
      {
        xdg_toplevel_set_fullscreen(_xdgToplevel, nullptr);
      }
      else
      {
        xdg_toplevel_unset_fullscreen(_xdgToplevel);
      }
    }

    ivec2 GetDrawableSize() const override
    {
      return { 0, 0 };
    }

    float GetDPIScale() const override
    {
      return 1;
    }

    WaylandInputManager& GetInputManager() override
    {
      return _inputManager;
    }

    void Run(std::function<void()> const& loop) override;

    void PlaceCursor(ivec2 const& cursor) override
    {
    }

    WaylandWindowSystem& GetSystem()
    {
      return _windowSystem;
    }

    wl_surface* GetSurface() const
    {
      return _surface;
    }

  protected:
    void _UpdateMouseLock() override;

    void _UpdateCursorVisible() override
    {
    }

  private:
    void OnSurfaceEnter(wl_surface* surface, wl_output* output)
    {
    }

    void OnSurfaceLeave(wl_surface* surface, wl_output* output)
    {
    }

    void OnXdgSurfaceConfigure(xdg_surface* xdgSurface, uint32_t serial);

    void OnXdgToplevelConfigure(xdg_toplevel* xdgToplevel, int32_t width, int32_t height, wl_array* states)
    {
      ivec2 size =
      {
        width ? width : _xdgBounds.x(),
        height ? height : _xdgBounds.y(),
      };

      if(size != ivec2{} && _size != size)
      {
        _size = size;
        if(_presenter)
        {
          _presenter->Resize(_size);
        }
      }
    }

    void OnXdgToplevelClose(xdg_toplevel* xdgToplevel)
    {
      Stop();
    }

    void OnXdgToplevelConfigureBounds(xdg_toplevel* xdgToplevel, int32_t width, int32_t height)
    {
      _xdgBounds = { width, height };
    }

    void OnEvent(InputEvent const& event)
    {
      _inputManager.AddEvent(event);
    }

    WaylandWindowSystem& _windowSystem;
    WaylandObject<wl_surface> _surface;
    WaylandObject<xdg_surface> _xdgSurface;
    WaylandObject<xdg_toplevel> _xdgToplevel;
    WaylandObject<zwp_locked_pointer_v1> _lockedPointer;

    ivec2 _size;
    ivec2 _xdgBounds;

    WaylandInputManager _inputManager;

    friend class WaylandWindowSystem;
  };

  class WaylandWindowSystem final : public WindowSystem
  {
  public:
    WaylandWindowSystem()
    {
      // connect to display
      _display = wl_display_connect(nullptr);
      if(!_display)
      {
        throw Coil::Exception("connecting to Wayland server failed");
      }

      _xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

      // get registry
      _registry = wl_display_get_registry(_display);
      {
        static constinit wl_registry_listener const listener =
        {
          .global        = WaylandEvent<&WaylandWindowSystem::OnRegistryGlobal      >::Handler,
          .global_remove = WaylandEvent<&WaylandWindowSystem::OnRegistryGlobalRemove>::Handler,
        };
        wl_registry_add_listener(_registry, &listener, this);
      }

      wl_display_roundtrip(_display);
    }

    ~WaylandWindowSystem()
    {
      if(_xkbState)
      {
        xkb_state_unref(_xkbState);
      }
      if(_xkbKeymap)
      {
        xkb_keymap_unref(_xkbKeymap);
      }
      if(_xkbContext)
      {
        xkb_context_unref(_xkbContext);
      }
    }

    WaylandWindow& CreateWindow(Book& book, std::string const& title, ivec2 const& size) override
    {
      // create surface
      WaylandObject<wl_surface> surface = wl_compositor_create_surface(GetCompositor());
      // create XDG surface
      WaylandObject<xdg_surface> xdgSurface = xdg_wm_base_get_xdg_surface(GetXdgWmBase(), surface);
      // assign toplevel role
      WaylandObject<xdg_toplevel> xdgToplevel = xdg_surface_get_toplevel(xdgSurface);

      return book.Allocate<WaylandWindow>(*this, std::move(surface), std::move(xdgSurface), std::move(xdgToplevel), title, size);
    }

    wl_display* GetDisplay() const
    {
      return _display;
    }

    // get global object by type
    template <typename T>
    T* Get() const
    {
      return std::get<T*>(_objects);
    }

    // getters for specific objects
    wl_compositor* GetCompositor() const
    {
      return Get<wl_compositor>();
    }
    wl_seat* GetSeat() const
    {
      return Get<wl_seat>();
    }
    wl_shm* GetShm() const
    {
      return Get<wl_shm>();
    }
    zwp_pointer_constraints_v1* GetPointerConstraints() const
    {
      return Get<zwp_pointer_constraints_v1>();
    }
    std::unordered_map<uint32_t, WaylandObject<wl_output>> const& GetOutputs() const
    {
      return std::get<std::unordered_map<uint32_t, WaylandObject<wl_output>>>(_objects);
    }
    xdg_wm_base* GetXdgWmBase() const
    {
      return Get<xdg_wm_base>();
    }

    wl_pointer* GetPointer() const
    {
      return _pointer;
    }

  private:
    void OnRegistryGlobal(wl_registry* registry, uint32_t name, char const* interface, uint32_t version)
    {
      [&]<typename... T>(std::tuple<T...>& objects)
      {
        ((
          (
            (strcmp(interface, GlobalInfo<T>::interface.name) == 0) &&
            (GlobalInfo<T>::requiredVersion <= version)
          )
          ? GlobalInfo<T>::Register(*this, name, wl_registry_bind(registry, name, &GlobalInfo<T>::interface, GlobalInfo<T>::requiredVersion))
          : (void)nullptr
        ), ...);
      }(_objects);
    }

    void OnRegistryGlobalRemove(wl_registry* registry, uint32_t name)
    {
      [&]<typename... T>(std::tuple<T...>& objects)
      {
        (GlobalInfo<T>::Unregister(*this, name), ...);
      }(_objects);
    }

    void OnKeyboardKeymap(wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
    {
      if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) return;

      if(_xkbState)
      {
        xkb_state_unref(_xkbState);
        _xkbState = nullptr;
      }
      if(_xkbKeymap)
      {
        xkb_keymap_unref(_xkbKeymap);
        _xkbKeymap = nullptr;
      }

      void* keymapData = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      close(fd);

      _xkbKeymap = xkb_keymap_new_from_string(_xkbContext, (char const*)keymapData, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
      munmap(keymapData, size);

      _xkbState = xkb_state_new(_xkbKeymap);
    }

    void OnKeyboardEnter(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys)
    {
      _keyboardFocusedWindow = (WaylandWindow*)wl_surface_get_user_data(surface);
    }

    void OnKeyboardLeave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
    {
      if(_keyboardFocusedWindow == (WaylandWindow*)wl_surface_get_user_data(surface))
      {
        _keyboardFocusedWindow = nullptr;
      }
    }

    void OnKeyboardKey(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
    {
      if(_keyboardFocusedWindow)
      {
        _keyboardFocusedWindow->OnEvent(InputKeyboardKeyEvent
        {
          .key = WaylandWindowSystem::ConvertKey(key),
          .isPressed = !!state,
        });

        char32_t character = xkb_state_key_get_utf32(_xkbState, key + 8);

        if(state && character)
        {
          _keyboardFocusedWindow->OnEvent(InputKeyboardCharacterEvent
          {
            .character = character,
          });
        }
      }
    }

    void OnKeyboardModifiers(wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
    {
      xkb_state_update_mask(_xkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }

    void OnKeyboardRepeatInfo(wl_keyboard* keyboard, int32_t rate, int32_t delay)
    {
    }

    void OnPointerEnter(wl_pointer* pointer, uint32_t serial, wl_surface* surface, int32_t surface_x, int32_t surface_y)
    {
      _pointerFocusedWindow = (WaylandWindow*)wl_surface_get_user_data(surface);
    }

    void OnPointerLeave(wl_pointer* pointer, uint32_t serial, wl_surface* surface)
    {
      if(_pointerFocusedWindow == (WaylandWindow*)wl_surface_get_user_data(surface))
      {
        _pointerFocusedWindow = nullptr;
      }
    }

    void OnPointerMotion(wl_pointer* pointer, uint32_t time, int32_t surface_x, int32_t surface_y)
    {
      if(_pointerFocusedWindow)
      {
        _pointerFocusedWindow->OnEvent(InputMouseCursorMoveEvent
        {
          .cursor =
          {
            (int32_t)wl_fixed_to_double(surface_x),
            (int32_t)wl_fixed_to_double(surface_y),
          },
          .wheel = 0,
        });
      }
    }

    void OnPointerButton(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t btn, uint32_t state)
    {
      InputMouseButton button;
      switch(btn)
      {
      case BTN_LEFT:   button = InputMouseButton::Left;   break;
      case BTN_RIGHT:  button = InputMouseButton::Right;  break;
      case BTN_MIDDLE: button = InputMouseButton::Middle; break;
      default:
        return;
      }
      if(_pointerFocusedWindow)
      {
        _pointerFocusedWindow->OnEvent(InputMouseButtonEvent
        {
          .button = button,
          .isPressed = !!state,
        });
      }
    }

    void OnPointerAxis(wl_pointer* pointer, uint32_t time, uint32_t axis, int32_t value)
    {
    }

    void OnPointerFrame(wl_pointer* pointer)
    {
    }

    void OnPointerAxisSource(wl_pointer* pointer, uint32_t axis_source)
    {
    }

    void OnPointerAxisStop(wl_pointer* pointer, uint32_t time, uint32_t axis)
    {
    }

    void OnPointerAxisDiscrete(wl_pointer* pointer, uint32_t axis, int32_t discrete)
    {
    }

    void OnRelativePointerRelativeMotion(zwp_relative_pointer_v1* relativePointer, uint32_t utime_hi, uint32_t utime_lo, int32_t dx, int32_t dy, int32_t dx_unaccel, int32_t dy_unaccel)
    {
      if(_pointerFocusedWindow)
      {
        _pointerFocusedWindow->OnEvent(InputMouseRawMoveEvent
        {
          .rawMove =
          {
            (float)wl_fixed_to_double(dx_unaccel),
            (float)wl_fixed_to_double(dy_unaccel),
            0,
          },
        });
      }
    }

    void OnXdgWmBasePing(xdg_wm_base* xdgWmBase, uint32_t serial)
    {
      xdg_wm_base_pong(xdgWmBase, serial);
    }

    // Wayland global objects info
    template <typename T>
    struct GlobalInfo {};

  // macro for singleton-type global objects
  // stored as simple pointer
  #define GLOBAL_SINGLETON_INFO(T, v) \
    template <> \
    struct GlobalInfo<T*> \
    { \
      static constexpr wl_interface const& interface = T##_interface; \
      static constexpr uint32_t requiredVersion = v; \
      static void Register(WaylandWindowSystem& windowSystem, uint32_t name, void* object) \
      { \
        std::get<T*>(windowSystem._objects) = (T*)object; \
        windowSystem.RegisterGlobalObject<T*>((T*)object); \
      } \
      static void Unregister(WaylandWindowSystem& windowSystem, uint32_t name) {} \
    }

  // macro for global objects of possibly multiple number
  // stored as unordered map by name
  #define GLOBAL_OBJECT_INFO(T, v) \
    template <> \
    struct GlobalInfo<std::unordered_map<uint32_t, WaylandObject<T>>> \
    { \
      static constexpr wl_interface const& interface = T##_interface; \
      static constexpr uint32_t requiredVersion = v; \
      static void Register(WaylandWindowSystem& windowSystem, uint32_t name, void* object) \
      { \
        std::get<std::unordered_map<uint32_t, WaylandObject<T>>>(windowSystem._objects)[name] = (T*)object; \
      } \
      static void Unregister(WaylandWindowSystem& windowSystem, uint32_t name) \
      { \
        std::get<std::unordered_map<uint32_t, WaylandObject<T>>>(windowSystem._objects).erase(name); \
      } \
    }

    GLOBAL_SINGLETON_INFO(wl_compositor, 4);
    GLOBAL_SINGLETON_INFO(wl_seat, 5);
    GLOBAL_SINGLETON_INFO(wl_shm, 1);
    GLOBAL_SINGLETON_INFO(xdg_wm_base, 2);
    GLOBAL_SINGLETON_INFO(zwp_pointer_constraints_v1, 1);
    GLOBAL_SINGLETON_INFO(zwp_relative_pointer_manager_v1, 1);
    GLOBAL_OBJECT_INFO(wl_output, 4);

#undef GLOBAL_SINGLETON_INFO
#undef GLOBAL_OBJECT_INFO

    template <typename T>
    void RegisterGlobalObject(T object) {}

    template <>
    void RegisterGlobalObject(wl_seat* object)
    {
      _keyboard = wl_seat_get_keyboard(object);
      {
        static constinit wl_keyboard_listener const listener =
        {
          .keymap      = WaylandEvent<&WaylandWindowSystem::OnKeyboardKeymap    >::Handler,
          .enter       = WaylandEvent<&WaylandWindowSystem::OnKeyboardEnter     >::Handler,
          .leave       = WaylandEvent<&WaylandWindowSystem::OnKeyboardLeave     >::Handler,
          .key         = WaylandEvent<&WaylandWindowSystem::OnKeyboardKey       >::Handler,
          .modifiers   = WaylandEvent<&WaylandWindowSystem::OnKeyboardModifiers >::Handler,
          .repeat_info = WaylandEvent<&WaylandWindowSystem::OnKeyboardRepeatInfo>::Handler,
        };
        wl_keyboard_add_listener(_keyboard, &listener, this);
      }

      _pointer = wl_seat_get_pointer(object);
      {
        static constinit wl_pointer_listener const listener =
        {
          .enter         = WaylandEvent<&WaylandWindowSystem::OnPointerEnter       >::Handler,
          .leave         = WaylandEvent<&WaylandWindowSystem::OnPointerLeave       >::Handler,
          .motion        = WaylandEvent<&WaylandWindowSystem::OnPointerMotion      >::Handler,
          .button        = WaylandEvent<&WaylandWindowSystem::OnPointerButton      >::Handler,
          .axis          = WaylandEvent<&WaylandWindowSystem::OnPointerAxis        >::Handler,
          .frame         = WaylandEvent<&WaylandWindowSystem::OnPointerFrame       >::Handler,
          .axis_source   = WaylandEvent<&WaylandWindowSystem::OnPointerAxisSource  >::Handler,
          .axis_stop     = WaylandEvent<&WaylandWindowSystem::OnPointerAxisStop    >::Handler,
          .axis_discrete = WaylandEvent<&WaylandWindowSystem::OnPointerAxisDiscrete>::Handler,
        };
        wl_pointer_add_listener(_pointer, &listener, this);
      }

      RegisterRelativePointer();
    }

    template <>
    void RegisterGlobalObject(xdg_wm_base* object)
    {
      static constinit xdg_wm_base_listener const listener =
      {
        .ping = WaylandEvent<&WaylandWindowSystem::OnXdgWmBasePing>::Handler,
      };
      xdg_wm_base_add_listener(object, &listener, this);
    }

    void RegisterRelativePointer()
    {
      if(!_pointer) return;
      auto* relativePointerManager = Get<zwp_relative_pointer_manager_v1>();
      if(!relativePointerManager) return;

      zwp_relative_pointer_v1* relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(relativePointerManager, _pointer);

      {
        static constinit zwp_relative_pointer_v1_listener const listener =
        {
          .relative_motion = WaylandEvent<&WaylandWindowSystem::OnRelativePointerRelativeMotion>::Handler,
        };
        zwp_relative_pointer_v1_add_listener(relativePointer, &listener, this);
      }
    }

    static InputKey ConvertKey(uint32_t code);

    WaylandObject<wl_display> _display;
    WaylandObject<wl_registry> _registry;

    std::tuple<
      wl_compositor*,
      wl_seat*,
      wl_shm*,
      xdg_wm_base*,
      zwp_relative_pointer_manager_v1*,
      zwp_pointer_constraints_v1*,
      std::unordered_map<uint32_t, WaylandObject<wl_output>>
    > _objects;

    WaylandObject<wl_keyboard> _keyboard;
    WaylandObject<wl_pointer> _pointer;

    xkb_context* _xkbContext = nullptr;
    xkb_keymap* _xkbKeymap = nullptr;
    xkb_state* _xkbState = nullptr;

    WaylandWindow* _keyboardFocusedWindow = nullptr;
    WaylandWindow* _pointerFocusedWindow = nullptr;
  };

  void WaylandWindow::Run(std::function<void()> const& loop)
  {
    _running = true;

    wl_display* display = _windowSystem.GetDisplay();

    while(_running)
    {
      bool wait = _loopOnlyVisible && !_visible;

      // loop for reading; ends when queue is empty
      while(_running)
      {
        // flush writes
        wl_display_flush(display);

        // start preparing to reading
        if(wl_display_prepare_read(display) == 0)
        {
          // wait for socket to become ready
          pollfd pfd =
          {
            .fd = wl_display_get_fd(display),
            .events = POLLIN | POLLPRI,
          };
          // read events or cancel read, depending on availability of socket
          if(poll(&pfd, 1, wait ? -1 : 0) > 0)
          {
            wl_display_read_events(display);
          }
          else
          {
            wl_display_cancel_read(display);
            break;
          }
        }

        wl_display_dispatch_pending(display);
      }

      _inputManager.Update();

      if(_visible || !_loopOnlyVisible)
      {
        loop();
      }
    }
  }

  void WaylandWindow::_UpdateMouseLock()
  {
    if(_mouseLock == !!_lockedPointer) return;

    if(_mouseLock)
    {
      _lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
        _windowSystem.GetPointerConstraints(), _surface,
        _windowSystem.GetPointer(),
        nullptr,
        ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
    }
    else
    {
      _lockedPointer = nullptr;
    }
  }

  void WaylandWindow::OnXdgSurfaceConfigure(xdg_surface* xdgSurface, uint32_t serial)
  {
    xdg_surface_ack_configure(xdgSurface, serial);

    wl_surface_commit(_surface);
  }
}
