#include "tasks_storage.hpp"

namespace Coil
{
  Task<void> AsyncWritableStorage::AsyncEnd()
  {
    co_return;
  }
}
