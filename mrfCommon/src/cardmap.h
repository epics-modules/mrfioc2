/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef CARDMAP_H_INC
#define CARDMAP_H_INC

#include <map>
#include <stdexcept>
#include <epicsMutex.h>

#include <mrfCommon.h>

bool cardIdInUse(short);
void cardIdAdd(short);

/** @brief Mapping of card number to class interface
 *
 * CardMap is a way to associate several class instances with
 * a numeric global identifier.  This allows EPICS device support
 * and IOC shell functions to find instances.
 *
 * There will be one instance of CardMap<INTRFACECLASS>
 * for each interface type exported by any drivers.
 * A potential user will query the instance to find
 * out if an ID number has been associated with
 * an instance of that interface.
 *
 * ID numbers are unique across all instances
 * of CardMap.
 */
template<class C>
class CardMap
{
    typedef std::map<short,C*> mapping_t;
    mapping_t mapping;

    epicsMutex mapLock;
public:
    typedef C value_type;

    bool available(short id)
    {
        SCOPED_LOCK(mapLock);
        return mapping.find(id) != mapping.end();
    }

    C& get(short id)
    {
        SCOPED_LOCK(mapLock);
        typename mapping_t::const_iterator it=mapping.find(id);
        if (it==mapping.end())
            throw std::runtime_error("Card interface not found");
        return *(it->second);
    }

    template<class DERV>
    DERV& get(short id)
    {
        SCOPED_LOCK(mapLock);
        C& base=get(id);
        DERV* derv=dynamic_cast<DERV*>(&base);
        if (!derv)
            throw std::runtime_error("Card interface can't be cast to requested type");
        return *derv;
    }

    /**@brief Create/Add interface to ID
     */
    void store(short id, C& card, bool append=false)
    {
        SCOPED_LOCK(mapLock);
        if (!append && cardIdInUse(id))
            throw std::runtime_error("Card ID already in use");

        std::pair<typename mapping_t::iterator,bool> add
         = mapping.insert(typename mapping_t::value_type(id,&card));

        if (!add.second)
            throw std::runtime_error("Interface already in use for this ID");

        cardIdAdd(id);
    }

    void append(short id, C& iface){store(id,iface,true);}

    /* visiting the base class does not require a dynamic_cast */
    template<typename T>
    void visit(T arg, bool (*fptr)(T,short,C&))
    {
        SCOPED_LOCK(mapLock);
        for(typename mapping_t::iterator it=mapping.begin();
            it!=mapping.end(); ++it)
        {
            if (!fptr(arg, it->first, *it->second))
                return;
        }
    }

    /* visiting a sub-class requires a dynamic_cast */
    template<typename T, class DERV>
    void visit(T arg, bool (*fptr)(T,short,DERV&))
    {
        SCOPED_LOCK(mapLock);
        for(typename mapping_t::iterator it=mapping.begin();
            it!=mapping.end(); ++it)
        {
            DERV *derv=dynamic_cast<DERV*>(it->second);
            if (!derv)
                continue;
            if (!fptr(arg, it->first, *derv))
                return;
        }
    }
};

#endif /* CARDMAP_H_INC */
