/**************************************************************************************************
|* FracSynthAnalyze () -- Analyze a Micrel SY87739L Fractional Synthesizer Control Word
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|* Date:     13 September 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 13 Sep 2006  E.Bjorklund     Original Release
|*
|*--------------------------------------------------------------------------------------------------
|* REFERENCE:
|* Micrel SY87739L Product Description.  Available at:
|*        http://www.micrel.com
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This function will take a hexadecimal number representing an SY87739L fractional synthesizer
|* control word and display the effective values of its six components.  These are:
|*
|*    P:          The integer part of the fractional frequency.
|*    Q(p):       The number of times to divide by P in the fractional-N control circuit.
|*    Q(p-1):     The number of times to divide by P-1 in the fractional-N control circuit.
|*    PostDivide: The final post-divider.
|*    N:          The numerator of the correction term.
|*    M:          The denominator of the correction term.
|*
|* It also displays the effective frequency of the internal Voltage-Controlled-Oscillator (VCO)
|* and the final output frequency that the control word will produce.
|*
|* The control word is also analyzed for programming errors and will display any problems it
|* finds. 
|*
|*-------------------------------------------------------------------------------------------------
|* USAGE:
|*      FracSynthAnalyze <ControlWord>
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      ControlWord  = (unsigned long)  Hexadecimal representation of the Micrel SY87739L
|*                                      control word to be analyzed.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      Always returns 0 (OK).
|*
\**************************************************************************************************/

/**************************************************************************************************
|*                                     COPYRIGHT NOTIFICATION
|**************************************************************************************************
|*  
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|**************************************************************************************************
|*
|* Copyright (c) 2006 Los Alamos National Security, LLC
|* as Operator of Los Alamos National Laboratory.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\*************************************************************************************************/

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <stdlib.h>             /* Standard C utility routines and definitions                    */
#include <stdio.h>              /* Standard C I/O library                                         */
#include <errno.h>              /* Standard C errno defintions                                    */

#include <epicsTypes.h>         /* EPICS type definitions                                         */

#include <mrfCommon.h>          /* MRF common definitions                                         */

/**************************************************************************************************/
/*  Import the Fractional Synthesizer Utility Routines                                            */
/**************************************************************************************************/

#define HOST_BUILD

#include <mrfFracSynth.h>       /* MRF SY87739L control word creation & analysis prototypes       */
#include <mrfFracSynth.c>       /* MRF SY87739L control word creation & analysis routines         */

/**************************************************************************************************/
/*  Main Program                                                                                  */
/**************************************************************************************************/

int main (int argc, char *argv[]) {

   /*---------------------
    * Local Variables
    */
    epicsBoolean   badArgs = epicsTrue;         /* True if we could not parse the argument list   */
    epicsUInt32    controlWord;                 /* SY87739L control word to analyze               */
    char          *tailPtr;                     /* Pointer to tail of parsed control word string  */

   /*---------------------
    * Make sure we were passed only one argument.
    * If so, see if we can parse it as a hexadecimal number.
    */
    if (argc == 2) {
        controlWord = strtoul (argv[1], &tailPtr, 16);

       /*---------------------
        * If we successfully parsed the control word,
        * analyze it and display the results.
        */
        if ((errno == OK) && (tailPtr != argv[1])) {
            FracSynthAnalyze (controlWord, MRF_FRAC_SYNTH_REF, DP_DEBUG);
            badArgs = epicsFalse;
        }/*end if control word parse was successful*/

    }/*end if we had the right number of arguments*/

   /*---------------------
    * Print the "Usage" message if we could not parse the argument.
    */
    if (badArgs) {
        printf ("Usage:\n");
        printf ("FracSynthAnalyze <ControlWord>\n");
        printf ("  Where <ControlWord> is a hexadecimal number representing\n");
        printf ("  a Micrel SY87739L control word to be analyzed.\n");
    }/*end if could not parse arguments*/

   /*---------------------
    * Always return success.
    */
    return OK;

}/*end main()*/
