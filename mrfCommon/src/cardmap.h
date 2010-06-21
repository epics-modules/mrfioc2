
#ifndef CARDMAP_H_INC
#define CARDMAP_H_INC

#include <map>
#include <stdexcept>

bool cardIdInUse(short);
void cardIdAdd(short);

class cardmap_error : public std::runtime_error
{
public:
    cardmap_error(const char* m) : std::runtime_error(m) {}
};


/**@brief Mapping of card number to class interface
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
public:
    typedef C value_type;

    bool available(short id)
    {
        return mapping.find(id) != mapping.end();
    }

    C& get(short id)
    {
        typename mapping_t::const_iterator it=mapping.find(id);
        if (it==mapping.end())
            throw cardmap_error("Card interface not found");
        return *(it->second);
    }

    template<class DERV>
    DERV& get(short id)
    {
        C& base=get(id);
        DERV* derv=dynamic_cast<DERV*>(&base);
        if (!derv)
            throw cardmap_error("Card interface can't be cast to requested type");
        return *derv;
    }

    /**@brief Create/Add interface to ID
     */
    void store(short id, C& card, bool append=false)
    {
        if (!append && cardIdInUse(id))
            throw cardmap_error("Card ID already in use");

        std::pair<typename mapping_t::iterator,bool> add
         = mapping.insert(typename mapping_t::value_type(id,&card));

        if (!add.second)
            throw cardmap_error("Interface already in use for this ID");

        cardIdAdd(id);
    }

    void append(short id, C& iface){store(id,iface,true);}

    template<typename T, class DERV>
    void visit(T arg, bool (*fptr)(T,short,DERV&))
    {
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
