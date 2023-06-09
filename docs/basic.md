# Coil Core basic libraries

## Base `coil_core_base`

Provides basic primitives: book, buffer, streams.

### Book

`Book` is an allocation helper, providing a way of management of long-living objects, alternative to RAII or smart pointers. The idea is that objects are to be allocated in a book, and then handled by simple reference (or pointer). When they are not needed anymore, book can be freed and then reused as necessary. Objects cannot be freed individually, as with smart pointers.

```cpp
class Object { ... };
Book book;
Object& object = book.Allocate<Object>(objectArg1, objectArg2, ...);
...
book.Free(); // destructor also frees the book
// book is empty now
```

## Unicode `coil_core_unicode`

Provides UTF-8, UTF-16, UTF-32 iterators and conversion.

## Utilities `coil_core_util`

Basic utilities missing from standard library, mostly templates.

## Entrypoint (`coil_core_entrypoint`, `coil_core_entrypoint_console`, `coil_core_entrypoint_graphical`)

Abstracts defining executable entry point on various platforms. Link `coil_core_entrypoint_console` or `coil_core_entrypoint_graphical` to the executable depending on whether it's console or graphical application.

```cpp
#include <coil/entrypoint.hpp>

int COIL_ENTRY_POINT(std::vector<std::string>&& args)
{
  return 0;
}
```

## Application identity `coil_core_appidentity`

Defines application identity globals, which are then used by various libraries (for example, to represent application in system's volume mixer).

```cpp
#include <coil/appidentity.hpp>
...
auto& appIdentity = AppIdentity::GetInstance();
appIdentity.Name() = "My Application";
appIdentity.Version() = 12345678;
```

## File system `coil_core_fs`

File system primitives: files, file streams, file memory mapping.

For simplicity represents file paths with `std::string`, always in UTF-8 encoding. Platform's native path separator is used.

## Tasks `coil_core_tasks`

Asyncronous, parallel, coroutine-based tasks.
