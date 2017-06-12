
#include "devObj.h"

const
linkOptionEnumType readbackEnum[] = { {"No",0}, {"Yes",1} };

const
linkOptionDef objdef[] =
{
    linkString  (addrBase, obj , "OBJ"  , 1, 0),
    linkString  (addrBase, prop , "PROP"  , 1, 0),
    linkEnum    (addrBase, rbv, "RB"   , 0, 0, readbackEnum),
    linkString  (addrBase, klass , "CLASS"  , 0, 0),
    linkString  (addrBase, parent , "PARENT"  , 0, 0),
    linkOptionEnd
};
