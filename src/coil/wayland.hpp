#pragma once

#include "platform.hpp"
#include <unordered_map>

// prototypes for Wayland types
struct wl_array;
struct wl_buffer;
struct wl_compositor;
struct wl_display;
struct wl_interface;
struct wl_keyboard;
struct wl_output;
struct wl_pointer;
struct wl_registry;
struct wl_seat;
struct wl_shm;
struct wl_shm_pool;
struct wl_surface;
struct xdg_surface;
struct xdg_toplevel;
struct xdg_wm_base;
struct zwp_locked_pointer_v1;
struct zwp_pointer_constraints_v1;
struct zwp_relative_pointer_manager_v1;
struct zwp_relative_pointer_v1;
// prototypes for xkbcommon types
struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace Coil
{
  void DestroyWaylandObject(wl_buffer* object);
  void DestroyWaylandObject(wl_display* object);
  void DestroyWaylandObject(wl_keyboard* object);
  void DestroyWaylandObject(wl_output* object);
  void DestroyWaylandObject(wl_pointer* object);
  void DestroyWaylandObject(wl_registry* object);
  void DestroyWaylandObject(wl_shm_pool* object);
  void DestroyWaylandObject(wl_surface* object);
  void DestroyWaylandObject(xdg_surface* object);
  void DestroyWaylandObject(xdg_toplevel* object);
  void DestroyWaylandObject(zwp_locked_pointer_v1* object);

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
    WaylandWindow(WaylandWindowSystem& windowSystem, WaylandObject<wl_surface>&& surface, WaylandObject<xdg_surface>&& xdgSurface, WaylandObject<xdg_toplevel>&& xdgToplevel, std::string const& title, ivec2 const& size);

    void SetTitle(std::string const& title) override;
    void Close() override;
    void SetFullScreen(bool fullScreen) override;
    ivec2 GetDrawableSize() const override;
    float GetDPIScale() const override;
    WaylandInputManager& GetInputManager() override;
    void Run(std::function<void()> const& loop) override;
    void PlaceCursor(ivec2 const& cursor) override;

    WaylandWindowSystem& GetSystem();
    wl_surface* GetSurface() const;

  protected:
    void _UpdateMouseLock() override;
    void _UpdateCursorVisible() override;

  private:
    void OnSurfaceEnter(wl_surface* surface, wl_output* output);
    void OnSurfaceLeave(wl_surface* surface, wl_output* output);
    void OnXdgSurfaceConfigure(xdg_surface* xdgSurface, uint32_t serial);
    void OnXdgToplevelConfigure(xdg_toplevel* xdgToplevel, int32_t width, int32_t height, wl_array* states);
    void OnXdgToplevelClose(xdg_toplevel* xdgToplevel);
    void OnXdgToplevelConfigureBounds(xdg_toplevel* xdgToplevel, int32_t width, int32_t height);
    void OnEvent(InputEvent const& event);

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
    WaylandWindowSystem();
    ~WaylandWindowSystem();

    WaylandWindow& CreateWindow(Book& book, std::string const& title, ivec2 const& size) override;

    wl_display* GetDisplay() const;

    // get global object by type
    template <typename T>
    T* Get() const
    {
      return std::get<T*>(_objects);
    }

    // getters for specific objects
    wl_compositor* GetCompositor() const;
    wl_seat* GetSeat() const;
    wl_shm* GetShm() const;
    zwp_pointer_constraints_v1* GetPointerConstraints() const;
    std::unordered_map<uint32_t, WaylandObject<wl_output>> const& GetOutputs() const;
    xdg_wm_base* GetXdgWmBase() const;

    wl_pointer* GetPointer() const;

  private:
    void OnRegistryGlobal(wl_registry* registry, uint32_t name, char const* interface, uint32_t version);
    void OnRegistryGlobalRemove(wl_registry* registry, uint32_t name);
    void OnKeyboardKeymap(wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
    void OnKeyboardEnter(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    void OnKeyboardLeave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
    void OnKeyboardKey(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    void OnKeyboardModifiers(wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
    void OnKeyboardRepeatInfo(wl_keyboard* keyboard, int32_t rate, int32_t delay);
    void OnPointerEnter(wl_pointer* pointer, uint32_t serial, wl_surface* surface, int32_t surface_x, int32_t surface_y);
    void OnPointerLeave(wl_pointer* pointer, uint32_t serial, wl_surface* surface);
    void OnPointerMotion(wl_pointer* pointer, uint32_t time, int32_t surface_x, int32_t surface_y);
    void OnPointerButton(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    void OnPointerAxis(wl_pointer* pointer, uint32_t time, uint32_t axis, int32_t value);
    void OnPointerFrame(wl_pointer* pointer);
    void OnPointerAxisSource(wl_pointer* pointer, uint32_t axis_source);
    void OnPointerAxisStop(wl_pointer* pointer, uint32_t time, uint32_t axis);
    void OnPointerAxisDiscrete(wl_pointer* pointer, uint32_t axis, int32_t discrete);
    void OnRelativePointerRelativeMotion(zwp_relative_pointer_v1* relativePointer, uint32_t utime_hi, uint32_t utime_lo, int32_t dx, int32_t dy, int32_t dx_unaccel, int32_t dy_unaccel);
    void OnXdgWmBasePing(xdg_wm_base* xdgWmBase, uint32_t serial);

    template <typename T>
    struct GlobalInfo;

    template <typename T>
    void RegisterGlobalObject(T object) {}
    template <>
    void RegisterGlobalObject(wl_seat* object);
    template <>
    void RegisterGlobalObject(xdg_wm_base* object);

    void RegisterRelativePointer();

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
}
