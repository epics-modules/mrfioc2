#ifndef EVG_INIT_H
#define EVG_INIT_H

#include <iostream>

#include <devcsr.h>

#include "evgMrm.h"

#define MRF_UCSR_DEFAULT 0x7fb03
#include "cardmap.h"

extern CardMap<evgMrm> evgmap;

#endif //EVG_INIT_H