/**************************************************************************************************
|* FracSynthControlWord () -- Analyze a Micrel SY87739L Fractional Synthesizer Control Word
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
|* This function will take a the frequency (in MegaHertz) that you would like to produce
|* and constructs a Micrel SY87739L control word to produce that frequency.  If the desired
|* frequency can not be produced exactly, the function will produce an output frequency
|* as close to the desired frequency as it can.
|*
|* The following information is displayed:
|*
|*    Control Word:        The Micrel SY87739L control word that will produce the desired freq.
|*    Desired Frequency:   Echo of the input parameter.  The frequency you asked it to produce.
|*    Effective Frequency: The output frequency actually produced by the control word
|*    Error:               The error ratio between the desired and effective frequencies
|*                         expressed in "parts-per-million".  For the MRF Series-200
|*                         event system, the error should be less than 100 ppm.
|*
|*-------------------------------------------------------------------------------------------------
|* USAGE:
|*      FracSynthControlWord <DesiredFreq>
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      DesiredFreq = (epicsFloat64)  The desired output frequency (in MegaHertz) that you
|*                                    would like the Micrel SY87739L chip to produce.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      0 (OK)    if we were able to create a control word for the desired frequency.
|*     -1 (ERROR) if we could not create a control word for the desired frequency.
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
#ifdef _WIN32
 #include <mrfFracSynth.h>       /* MRF SY87739L control word creation & analysis prototypes      */
#endif
#include <debugPrint.h>                   /* SLAC Debug print utility                             */

#include <mrfCommon.h>          /* MRF common definitions                                         */

#include <epicsExport.h>        /* EPICS Symbol exporting macro definitions                       */

/**************************************************************************************************/
/*  Import the Fractional Synthesizer Utility Routines                                            */
/**************************************************************************************************/

#define HOST_BUILD

#ifndef _WIN32
# include <mrfFracSynth.h>       /* MRF SY87739L control word creation & analysis prototypes       */
# include <mrfFracSynth.c>       /* MRF SY87739L control word creation & analysis routines         */
#endif

/**************************************************************************************************/
/*  Main Program                                                                                  */
/**************************************************************************************************/

int main (int argc, char *argv[]) {

   /*---------------------
    * Local Variables
    */
    epicsBoolean   badArgs = epicsTrue;         /* True if we could not parse the argument list   */
    epicsUInt32    controlWord = 0;             /* Genearated SY87739L control word               */
    epicsFloat64   DesiredFreq;                 /* Frequency we wish to create a control word for */
    epicsFloat64   EffectiveFreq;               /* Freq. actually generated by the control word   */
    epicsFloat64   Error;                       /* Error between the desired and actual freqs.    */
    char          *tailPtr;                     /* Pointer to tail of parsed control word string  */

   /*---------------------
    * Make sure we were passed only one argument.
    * If so, see if we can parse it as a floating point value.
    */
    if (argc == 2) {
        DesiredFreq = strtod (argv[1], &tailPtr);

       /*---------------------
        * If we successfully parsed the desired frequency,
        * try to compute a control word that will generate it.
        */
        if ((errno == OK) && (tailPtr != argv[1])) {
            controlWord = FracSynthControlWord (DesiredFreq, MRF_FRAC_SYNTH_REF, DP_NONE, &Error);
            badArgs = epicsFalse;

           /*---------------------
            * Abort if we could not successfully create a control word for this frequency
            */
            if (controlWord == 0) {
                printf ("Unable to create a control word for %f MHz.\n", DesiredFreq);
                return ERROR;
            }/*end if could not create control word*/

           /*---------------------
            * Compute the effective frequency generated by the control word we created and
            * check it for errors (we don't expect any errors, since we created it, but
            * you never know....)
            */
            EffectiveFreq = FracSynthAnalyze (controlWord, MRF_FRAC_SYNTH_REF, DP_ERROR);

           /*---------------------
            * Display the control word, the effective frequency, and the error.
            */
            printf ("Control Word = 0x%08X.\n", controlWord);
            printf ("Desired Frequency = %f Mhz.  Effective Frequency = %f MHz. ",
                    DesiredFreq, EffectiveFreq);
            printf ("Error = %5.3f ppm.\n", Error);

        }/*end if control word parse was successful*/

    }/*end if we had the right number of arguments*/

   /*---------------------
    * Print the "Usage" message if we could not parse the argument.
    */
    if (badArgs) {
        printf ("Usage:\n");
        printf ("FracSynthControlWord <DesiredFreq>\n");
        printf ("  Where <DesiredFreq> is the frequency (in MegaHertz)\n");
        printf ("  that you wish to generate an SY87739L control word for.\n");
    }/*end if could not parse arguments*/

   /*---------------------
    * Always return success.
    */
    return OK;

}/*end main()*/
