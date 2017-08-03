#include <stdexcept>
#include <iomanip>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <epicsStdio.h>
#include <epicsExport.h>
#include "mrfCommon.h"

int MRFVersion::compare(const MRFVersion& o) const
{
    if(m_major<o.m_major)
        return -1;
    else if(m_major>o.m_major)
        return 1;
    else if(m_minor<o.m_minor)
        return -1;
    else if(m_minor>o.m_minor)
        return 1;
    else
        return 0;
}

std::string MRFVersion::str() const
{
    std::ostringstream strm;
    strm<<(*this);
    return strm.str();
}

std::ostream& operator<<(std::ostream& strm, const MRFVersion& ver)
{
    strm<<std::hex<<ver.firmware()
        <<std::hex<<std::setfill('0')<<std::setw(2)<<ver.revision()
        <<'.'
        <<((ver.subrelease()<0) ? "-" : "")
        <<abs(ver.subrelease());
    return strm;
}

epicsUInt32 roundToUInt(double val, epicsUInt32 max)
{
    if(!isfinite(val))
        throw std::range_error("Value not finite");

    else if(val<0)
        throw std::range_error("Negative value not allowed");

    val+=0.5;

    if(val>(double)max)
        throw std::range_error("Value too large");

    return (epicsUInt32)val;
}

char *allocSNPrintf(size_t N, const char *fmt, ...)
{
    char *mem = (char*)calloc(1, N);
    if(!mem)
        throw std::bad_alloc();

    va_list args;

    va_start(args, fmt);

    epicsVsnprintf(mem, N, fmt, args);

    va_end(args);

    mem[N-1] = '\0';

    return mem;
}

#if (EPICS_VERSION_INT < VERSION_INT(3,15,0,2))

static
int
epicsParseULong(const char *str, unsigned long *to, int base, char **units)
{
    int c;
    char *endp;
    unsigned long value;

    while ((c = *str) && isspace(c))
        ++str;

    errno = 0;
    value = strtoul(str, &endp, base);

    if (endp == str)
        return S_stdlib_noConversion;
    if (errno == EINVAL)    /* Not universally supported */
        return S_stdlib_badBase;
    if (errno == ERANGE)
        return S_stdlib_overflow;

    while ((c = *endp) && isspace(c))
        ++endp;
    if (c && !units)
        return S_stdlib_extraneous;

    *to = value;
    if (units)
        *units = endp;
    return 0;
}

int
epicsParseUInt32(const char *str, epicsUInt32 *to, int base, char **units)
{
    unsigned long value;
    int status = epicsParseULong(str, &value, base, units);

    if (status)
        return status;

#if (ULONG_MAX > 0xffffffffULL)
    if (value > 0xffffffffUL && value <= ~0xffffffffUL)
        return S_stdlib_overflow;
#endif

    *to = (epicsUInt32) value;
    return 0;
}

#endif
