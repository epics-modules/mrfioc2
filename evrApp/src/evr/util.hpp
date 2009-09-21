
#ifndef UTIL_HPP_INC
#define UTIL_HPP_INC

#include <dbScan.h>

/**@file util.hpp
 *
 * Misc definitions.
 */

/**@brief A general way to notify device support of status changes
 *
 * Device Supports can use statusChange() to implement get_ioint_info()
 */
class IOStatus
{
public:
  virtual IOSCANPVT statusChange(){return NULL;};
};

struct TimeUnits {
  enum type {
    Tick,
    Micro,
    Milli,
    Sec
  };
};

//! Three state logic
struct TSL {
  enum type {
    Float, //!< Driver off
    Low,
    High
  };
};

#endif // UTIL_HPP_INC
