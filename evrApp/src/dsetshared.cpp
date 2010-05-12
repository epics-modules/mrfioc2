
#include <cstdlib>
#include <devLib.h>

#include "dsetshared.h"

extern "C" {

long init_record_empty(void *)
{
  return 0;
}

long init_record_return2(void *)
{
  return 2;
}

long del_record_empty(dbCommon*)
{
  return 0;
}

}
