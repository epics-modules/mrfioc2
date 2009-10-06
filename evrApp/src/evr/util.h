
#ifndef UTIL_HPP_INC
#define UTIL_HPP_INC

#include <dbScan.h>

struct dbCommon;

/**@file util.hpp
 *
 * Misc definitions.
 */

/**@brief A general way to notify device support of status changes
 *
 * Device Supports can use statusChange() to implement get_ioint_info()
 *
 * A more complicated implimentaton would use the argument to
 * impliment reference counting to allow the 'interrupt' to
 * be disabled when unused.
 */
class IOStatus
{
public:
  virtual IOSCANPVT statusChange(bool up=true){return 0;};
};

/*! Assumes that prec->dpvt contains an instance of an IOStatus sub-class
 *
 * Store with prec->dpvt=static_cast<void*>(subcls)
 */
long get_ioint_info_statusChange(int dir,dbCommon* prec,IOSCANPVT* io);

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
