#include <stdbool.h>

static bool sleep_lock = false;
//------------------------------------------------------------------------------

void sleepLock(void)
{
  sleep_lock = true;
}

//------------------------------------------------------------------------------
bool sleepLock_check()
{
  bool retval = sleep_lock;
  sleep_lock = false;
  return retval;
}