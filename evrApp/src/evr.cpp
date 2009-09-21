
#include "evr.hpp"

EVR::EVR() :
   model(Type::Invalid)
  ,version(0)
  ,inputCount(0)
  ,outputCount(0)
  ,prescalerCount(0)
  ,pulserCount(0)
  ,fifoPresent(0)
  ,dataBufPresent(0)
{
}

EVR::~EVR()
{
}
