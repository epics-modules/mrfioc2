/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef LINKOPTIONS_H
#define LINKOPTIONS_H

#include <shareLib.h>
#include <dbDefs.h>
#include <epicsTypes.h>

/**@file linkoptions.h
 *@brief Hardware link parsing and storage
 *
 * Utility to parse a macLib style string ('key=val, key2=val2')
 * and store the results directly into a user supplied struture
 * after appropriate type conversion.
 *
 * Before calling linkOptionsStore() a linkOptionDef structure
 * must be defined for the user structure.
 *
 * For example:
 *
 * typedef struct myStruct {
 *   ... other stuff
 *   epicsUInt32 ival;
 *   double      dval;
 *   epicsUInt32 ival2;
 *   int         enumval;
 *   char       strval[20];
 * } myStruct;
 *
 * static const
 * linkOptionEnumType colorEnum[] = { {"Red",1}, {"Green",2}, {"Blue",3}, {NULL,0} };
 *
 * static const
 * linkOptionDef myStructDef[] = {
 *   linkInt32 (myStruct, ival,    "Integer" , 0, 0),
 *   linkInt32 (myStruct, ival2,   "Second"  , 1, 0),
 *   linkDouble(myStruct, dval,    "Double"  , 1, 0),
 *   linkString(myStruct, strval , "String"  , 1, 0),
 *   linkEnum  (myStruct, enumval, "Color"   , 1, 0, colorEnum),
 *   linkOptionEnd
 * };
 *
 * Note: The 4th argument is required (1) or optional (0), the 5th
 *       whether later definitions override previous ones (1), or
 *       are an error (0).
 *
 * void someFunc(const char *arg) {
 *   myStruct mine;
 *
 *   memset(&mine, 0, sizeof(myStruct)); // set defaults
 *
 *   if (linkOptionsStore(myStructDef, &mine, arg, 0))
 *     goto error;
 *
 *   printf("Second=%d\n",mine->ival2);
 * }
 *
 * Would parse the string 'Second=17, Color=Green, String=Hello world, Double=4.2'
 * and assign ival2=17, dval=4.2, enumval=2, strval="Hello world".  ival is not
 * required and since 'Integer' was not specified would not remain unchanged.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum linkOption {
    linkOptionInvalid=0,
    linkOptionInt32,
    linkOptionDouble,
    linkOptionString,
    linkOptionEnum
} linkOption;

typedef struct linkOptionEnumType {
    const char *name;
    const int value;
} linkOptionEnumType;

typedef struct linkOptionDef {
    const char * name;
    linkOption optType;
    int required:1;
    int overwrite:1;
    epicsUInt32 offset;
    epicsUInt32 size;
    const linkOptionEnumType *Enums;
} linkOptionDef;

#define linkInt32(Struct, Member, Name, Req, Over) \
{Name, linkOptionInt32, Req, Over, OFFSET(Struct, Member), sizeof( ((Struct*)0)->Member ), NULL}

#define linkDouble(Struct, Member, Name, Req, Over) \
{Name, linkOptionDouble, Req, Over, OFFSET(Struct, Member), sizeof( ((Struct*)0)->Member ), NULL}

#define linkString(Struct, Member, Name, Req, Over) \
{Name, linkOptionString, Req, Over, OFFSET(Struct, Member), sizeof( ((Struct*)0)->Member ), NULL}

#define linkEnum(Struct, Member, Name, Req, Over, Enums) \
{Name, linkOptionEnum, Req, Over, OFFSET(Struct, Member), sizeof( ((Struct*)0)->Member ), Enums}

#define linkOptionEnd {0,linkOptionInvalid,0,0,0,0,NULL}

/* Extra output when parsing and converting */
#define LINKOPTIONDEBUG 1

/**@brief Parse a string a store the result
 *
 * Takes the string 'str', parses it according to 'opts', then sorts the
 * result in 'user'.
 *
 *@param opts A null-terminated array of options.
 *@param user Pointer to a structure whos member offsets are given in 'opts'
 *@param str  The string to parse
 *@param options Some modifiers for the parsing process or 0.
 *               The only option is LINKOPTIONDEBUG.
 *@return 0 Ok
 *@return -1 Fail ('user' may be partially modified)
 */
epicsShareFunc
int
epicsShareAPI
linkOptionsStore(const linkOptionDef* opts, void* user, const char* str, int options);

/**@brief Return the string associated with Enum 'i'
 *
 *@param Enums A null-terminated array of string/integer pairs
 *@param i An Enum index
 *@param def String to be returned in 'i' isn't a valid Enum index.
 *@return A constant string
 */
epicsShareFunc
const char*
epicsShareAPI
linkOptionsEnumString(const linkOptionEnumType *Enums, int i, const char* def);

#ifdef __cplusplus
}
#endif

#endif /* LINKOPTIONS_H */
