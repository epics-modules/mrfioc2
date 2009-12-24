/***************************************************************************************************
|* mrfFracSynth.c -- EPICS Support Routines for the Micrel SY87739L Fractional-N Synthesizer Chip
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|* Date:     13 September 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 18 Sep 2008  E.Bjorklund     Clean up some compiler warnings
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
|* This module contains routines to create and analyze the control word for the Micrel SY87739L
|* Fractional-N synthesizer chip.  This chip is used in the MRF Series-200 event receiver cards to
|* synchronize with the expected event clock frequency.  It is also in the event generator card
|* where it can be used to generate the event clock in the absence of an RF source.
|*
|* Three routines are provided in this module:
|*
|*    mrfSetEventClockSpeed (InputClockSpeed, InputControlWord, ReferenceFreq,
|*                           &OutputClockSpeed, &OutputControlWord, PrintFlag);
|*        Determines the event clock speed and/or the fractional synthesizer control word given
|*        the knowledge of one or the other of these paramters.
|*
|*    FracSynthControlWord (DesiredFreq, ReferenceFreq, debugFlag, &Error):
|*        Creates an SY87739L control word which will generate the desired frequency from
|*        the specified reference frequency. If it can not generate the desired frequency
|*        exactly, it will generate a frequency as close to the desired frequency as it can.
|*        The routine will return the "error" (expressed in parts-per-million) between the desired
|*        and generated frequencies.
|*
|*        For the MRF Series-200 event system, the error must be below +/- 100 ppm.
|*
|*    FracSynthAnalyze (ControlWord, ReferenceFreq, PrintFlag):
|*        Examines the specified SY87739L control word and returns the output frequency
|*        that will be generated given the specified reference frequency.  Depending on
|*        the value of the "PrintFlag" parameter, it will also display the values of the
|*        control word fields and how the output frequency is created from them.  It will
|*        also analyze the control word for programming errors.
|*
|* For convenience, this module also defines EPICS IOC Shell versions of the "FracSynth" routines.
|* The IOC Shell versions only take the first argument.  The reference frequency defaults to
|* the MRF input reference frequency (24 MHz) and the debugFlag/PrintFlag are defaulted to
|* produce the maximum amount of printed output.
|*
|*--------------------------------------------------------------------------------------------------
|* NOTES:
|* o For the MRF Series-200 cards, the input reference frequency is 24 Mhz.
|*
|* o To use the EPICS IOC shell definitions, the "Data Base Definition" (dbd) file should include
|*   the following line:
|*
|*        registrar (FracSynthRegistrar)
|*
|* o These routines are not suitable for calling from the vxWorks shell as they require
|*   floating point input.
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

#ifndef DEBUG_PRINT
#define DEBUG_PRINT                       /* Debug printing always enabled for this module        */
#endif

#include <stdio.h>                        /* Standard C I/O library                               */
#include <math.h>                         /* Standard C Math library                              */

#include <epicsTypes.h>                   /* EPICS Type definitions                               */
#include <iocsh.h>                        /* EPICS IOC shell support library                      */
#include <registryFunction.h>             /* EPICS Registry support library                       */

#include <mrfCommon.h>                    /* MRF event system constants and definitions           */
#include <mrfFracSynth.h>                 /* MRF Fractional Synthesizer routines                  */

#include <epicsExport.h>                  /* EPICS Symbol exporting macro definitions             */

/**************************************************************************************************/
/*  Configuration Parameters                                                                      */
/**************************************************************************************************/

#define MAX_CORRECTION_RATIO   (17./14.)  /* Maximum value for correction term.                   */
#define MAX_VCO_FREQ             729.0    /* Maximum frequency for voltage-controlled oscillator  */
#define MIN_VCO_FREQ             540.0    /* Minimum frequency for voltage-controlled oscillator  */
#define MIN_P_VALUE                 17    /* Minimum val for integer part of fractional frequency */
#define MAX_FRAC_DIVISOR            31    /* Maximum divisor for fractional frequency value       */

#define NUM_POST_DIVIDES            31    /* Number of unique post-divider values                 */
#define NUM_POST_DIVIDE_VALS        32    /* Number of post-divider codes                         */
#define NUM_CORRECTIONS             23    /* Number of valid correction values (plus one)         */
#define NUM_CORRECTION_VALS          8    /* Number of correction value codes                     */

#define MAX_ERROR                100.0    /* Artifical error maximum                              */
#define ZERO_THRESHOLD          1.0e-9    /* Floating point threshold for zero detection          */


/**************************************************************************************************/
/*  Define the Fields in the SY87739L Control Word                                                */
/**************************************************************************************************/

/*---------------------
 * Define the size of each field
 */

#define CONTROL_MDIV_BITS        3        /* Denominator of correction term                       */
#define CONTROL_NDIV_BITS        3        /* Numerator of correction term                         */
#define CONTROL_POSTDIV_BITS     5        /* Post-Divider                                         */
#define CONTROL_MFG_BITS         3        /* Must Be Zero                                         */
#define CONTROL_P_BITS           4        /* Integer part of fractional frequency                 */
#define CONTROL_QPM1_BITS        5        /* Value for Q(p-1) term of fractional frequency        */
#define CONTROL_QP_BITS          5        /* Value for Q(p) term of fractional frequency          */
#define CONTROL_PREAMBLE_BITS    4        /* Must Be Zero                                         */

/*---------------------
 * Define the offset of each field
 */

#define CONTROL_MDIV_SHIFT       0        /* Denominator of correction term                       */
#define CONTROL_NDIV_SHIFT       3        /* Numerator of correction term                         */
#define CONTROL_POSTDIV_SHIFT    6        /* Post-Divider                                         */
#define CONTROL_MFG_SHIFT       11        /* Must Be Zero                                         */
#define CONTROL_P_SHIFT         14        /* Integer part of fractional frequency                 */
#define CONTROL_QPM1_SHIFT      18        /* Value for Q(p-1) term of fractional frequency        */
#define CONTROL_QP_SHIFT        23        /* Value for Q(p) term of fractional frequency          */
#define CONTROL_PREAMBLE_SHIFT  28        /* Must Be Zero                                         */

/*---------------------
 * Set up a bitmask for each field
 */

#define CONTROL_MDIV_MASK        (((1 << CONTROL_MDIV_BITS)     - 1) << CONTROL_MDIV_SHIFT)
#define CONTROL_NDIV_MASK        (((1 << CONTROL_NDIV_BITS)     - 1) << CONTROL_NDIV_SHIFT)
#define CONTROL_POSTDIV_MASK     (((1 << CONTROL_POSTDIV_BITS)  - 1) << CONTROL_POSTDIV_SHIFT)
#define CONTROL_MFG_MASK         (((1 << CONTROL_MFG_BITS)      - 1) << CONTROL_MFG_SHIFT)
#define CONTROL_P_MASK           (((1 << CONTROL_P_BITS)        - 1) << CONTROL_P_SHIFT)
#define CONTROL_QPM1_MASK        (((1 << CONTROL_QPM1_BITS)     - 1) << CONTROL_QPM1_SHIFT)
#define CONTROL_QP_MASK          (((1 << CONTROL_QP_BITS)       - 1) << CONTROL_QP_SHIFT)
#define CONTROL_PREAMBLE_MASK    (((1 << CONTROL_PREAMBLE_BITS) - 1) << CONTROL_PREAMBLE_SHIFT)

/*---------------------
 * Define the field values for the correction factor components
 */
#define CORRECTION_DIV_14        5       /* Numerator or denominator of 14                        */
#define CORRECTION_DIV_15        7       /* Numerator or denominator of 15                        */
#define CORRECTION_DIV_16        1       /* Numerator or denominator of 16                        */
#define CORRECTION_DIV_17        3       /* Numerator or denominator of 17                        */
#define CORRECTION_DIV_18        2       /* Numerator or denominator of 18                        */
#define CORRECTION_DIV_31        4       /* Numerator or denominator of 31                        */
#define CORRECTION_DIV_32        6       /* Numerator or denominator of 32                        */


/**************************************************************************************************/
/*  Structure Definitions                                                                         */
/**************************************************************************************************/

/*---------------------
 * Post-Divide Structure
 */
typedef struct {                         /* PostDivideStruct                                      */
    epicsFloat64    Divisor;             /*   Divisor value                                       */
    epicsUInt32     Code;                /*   Field code for post-divide value                    */
} PostDivideStruct;

/*---------------------
 * Correction Factor Structure
 */
typedef struct {                         /* CorrectionStruct                                      */
    epicsFloat64    Ratio;               /*   Correction factor ratio value                       */
    epicsUInt16     nDiv;                /*   Field code for dividend (N)                         */
    epicsUInt16     mDiv;                /*   Field code for divisor (M)                          */
} CorrectionStruct;

/*---------------------
 * Correction Factor Component Value Structure
 */
typedef struct {                         /* CorrectionValStruct                                   */
    epicsInt32      Value;               /*   Field code value                                    */
    epicsInt32      Class;               /*   Component class (1 or 2)                            */
} CorrectionValStruct;

/*---------------------
 * Fractional Synthesizer Component Structure
 */
typedef struct {                         /* FracSynthComponents                                   */
    epicsFloat64    Error;               /*   Deviation from desired output frequency             */
    epicsFloat64    EffectiveFreq;       /*   Actual frequency generated by these components      */
    epicsInt32      PostDivIndex;        /*   Index into post-divider table                       */
    epicsInt32      CorrectionIndex;     /*   Index into correction factor table                  */
    epicsInt32      P;                   /*   Integer part of fractional frequency                */
    epicsInt32      Qp;                  /*   Value of Q(p) term of fractional frequency          */
    epicsInt32      Qpm1;                /*   Value of Q(p-1) term of fractional frequency        */
} FracSynthComponents;

/**************************************************************************************************/
/*  Tables for Creating and Decoding the SY87739L Control Word                                    */
/**************************************************************************************************/

/*---------------------
 * Post-Divider Table:
 *---------------------
 *   Contains legal post-divider values and their control-word field codes
 */

static const PostDivideStruct   PostDivideList [NUM_POST_DIVIDES] = {
    { 1.0, 0x00},  { 2.0, 0x02},  { 3.0, 0x03},  { 4.0, 0x04},  { 5.0, 0x05},  { 6.0, 0x06},
    { 7.0, 0x07},  { 8.0, 0x08},  { 9.0, 0x09},  {10.0, 0x0A},  {11.0, 0x0B},  {12.0, 0x0C},
    {13.0, 0x0D},  {14.0, 0x0E},  {15.0, 0x0F},  {16.0, 0x10},  {18.0, 0x11},  {20.0, 0x12},
    {22.0, 0x13},  {24.0, 0x14},  {26.0, 0x15},  {28.0, 0x16},  {30.0, 0x17},  {32.0, 0x18},
    {36.0, 0x19},  {40.0, 0x1A},  {44.0, 0x1B},  {48.0, 0x1C},  {52.0, 0x1D},  {56.0, 0x1E},
    {60.0, 0x1F}
};

/*---------------------
 * Post-Divider Value Table:
 *---------------------
 *   Translates the post-divider field code into the actual post-divider value.
 */

static epicsInt32 PostDivideValList [NUM_POST_DIVIDE_VALS] = {
     1,  3,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 18, 20, 22, 24, 26, 28, 30, 32, 36, 40, 44, 48, 52, 56, 60
};

/*---------------------
 * Correction Factor Table:
 *---------------------
 *   Contains legal correction factor ratios and their control-word field codes.
 */

static const CorrectionStruct  CorrectionList [NUM_CORRECTIONS] = {
    {(1.0),     CORRECTION_DIV_14, CORRECTION_DIV_14},
    {(14./18.), CORRECTION_DIV_14, CORRECTION_DIV_18},
    {(14./17.), CORRECTION_DIV_14, CORRECTION_DIV_17},
    {(15./18.), CORRECTION_DIV_15, CORRECTION_DIV_18},
    {(14./16.), CORRECTION_DIV_14, CORRECTION_DIV_16},
    {(15./17.), CORRECTION_DIV_15, CORRECTION_DIV_17},
    {(16./18.), CORRECTION_DIV_18, CORRECTION_DIV_18},
    {(14./15.), CORRECTION_DIV_14, CORRECTION_DIV_15},
    {(15./16.), CORRECTION_DIV_15, CORRECTION_DIV_16},
    {(16./17.), CORRECTION_DIV_16, CORRECTION_DIV_17},
    {(17./18.), CORRECTION_DIV_17, CORRECTION_DIV_18},
    {(31./32.), CORRECTION_DIV_31, CORRECTION_DIV_32},
    {(1.0),     CORRECTION_DIV_14, CORRECTION_DIV_14},
    {(32./31.), CORRECTION_DIV_32, CORRECTION_DIV_31},
    {(18./17.), CORRECTION_DIV_18, CORRECTION_DIV_17},
    {(17./16.), CORRECTION_DIV_17, CORRECTION_DIV_16},
    {(16./15.), CORRECTION_DIV_16, CORRECTION_DIV_15},
    {(15./14.), CORRECTION_DIV_15, CORRECTION_DIV_14},
    {(18./16.), CORRECTION_DIV_18, CORRECTION_DIV_16},
    {(17./15.), CORRECTION_DIV_17, CORRECTION_DIV_15},
    {(16./14.), CORRECTION_DIV_16, CORRECTION_DIV_14},
    {(18./15.), CORRECTION_DIV_18, CORRECTION_DIV_15},
    {(17./14.), CORRECTION_DIV_17, CORRECTION_DIV_14}
};/*Correction List*/

/*---------------------
 * Correction Factor Value Table:
 *---------------------
 *   Translates correction factor field codes into the actual correction value term values
 */

static const CorrectionValStruct  CorrectionValList [NUM_CORRECTION_VALS] = {
    {16, 1}, {16, 1}, {18, 1}, {17, 1}, {31, 2}, {14, 1}, {32, 2}, {15, 1}
};/* CorrectionValList*/

/**************************************************************************************************
|* mrfSetEventClockSpeed () -- Determine the Desired Event Clock Speed and Frac Synth Control Word
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will determine the system's event clock speed (in Megahertz) and/or the value for
|* the fractional synthesizer control word given at least one of these two values.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*
|* If the InputClockSpeed parameter is specified (not zero), the routine will compute the value
|* of the fractional synthesizer control word to produce that frequency.
|*
|* If the InputControlWord parameter is specified (not zero), the routine will check the control
|* word for programming errors and return the actual frequency (in Megahertz) produced by that
|* control word.
|*
|* If neither parameter is specified, the routine will output a default event clock frequency and
|* a default control word.
|*
|* If both parameters are specified, the routine will make sure the frequency generated by the
|* control word is within 100 ppm of the desired clock speed.
|*
|* If no errors are encountered, the routine will return both the final event clock speed and
|* the fractional synthesizer control word.  If errors are encountered, both output parameters
|* will contain zero.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = mrfSetEventClockSpeed (InputClockSpeed, InputControlWord, ReferenceFreq,
|*                                      &OutputClockSpeed, &OutputControlWord, PrintFlag);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      InputClockSpeed     = (epicsFloat64)  Desired event clock speed in Megahertz.
|*                                            0.0 means no clock speed specified.      
|*      InputControlWord    = (epicsUInt32)   Desired value for the fractional synthesizer
|*                                            control word. 0 means no control word specified.
|*      ReferenceFreq       = (epicsFloat64)  SY87739L input reference frequency in MegaHertz.
|*      PrintFlag           = (epicsInt32)    Flag to control output messages.  Output levels
|*                                            correspond to the DEBUGPRINT output levels:
|*                                            DP_NONE  (0) = Only display errors that prevent us
|*                                                           from returning the event clock speed
|*                                                           and the control word.
|*                                            DP_ERROR (2) = Display any programming errors
|*                                                           detected in the control word
|*                                            DP_WARN  (3) = Display warning messages such as
|*                                                           the one warning that we are using
|*                                                           the default event clock frequency.
|*                                            DP_INFO  (4) = Display the value of the computed
|*                                                           control word, its actual output
|*                                                           frequency, and the error (in ppm)
|*                                                           between the desired and actual
|*                                                           frequencies.
|*                                            DP_DEBUG (5) = Display the actual values of the
|*                                                           fields in the control word, along
|*                                                           with the constituent parts of the
|*                                                           output frequency (VCO frequency,
|*                                                           correction term, reference freq.)
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*      OutputClockSpeed    = (epicsFloat64)  The input or computed value for the event clock speed
|*      OutputControlWord   = (epicsUInt32)   The input or computed value for the fractional
|*                                            synthesizer control word.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      MRF_DEF_CLOCK_SPEED = (epicsFloat64)  Default clock speed if neither InputeClockSpeed or
|*                                            InputControlWord were specified.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status              = (epicsStatus)   OK if we were able to return both an event clock
|*                                            frequency and a fractional synthesizer control word.
|*                                            ERROR if we could not.
|*
\**************************************************************************************************/

epicsStatus mrfSetEventClockSpeed (

    /**********************************************************************************************/
    /*  Parameter Declarations                                                                    */
    /**********************************************************************************************/

    epicsFloat64   InputClockSpeed,             /* Desired event clock speed in MHz (or zero)     */
    epicsUInt32    InputControlWord,            /* Fractional synthesizer control word (or zero)  */
    epicsFloat64   ReferenceFreq,               /* SY87739L input reference frequency (in MHz)    */
    epicsFloat64  *OutputClockSpeed,            /* Resulting event clock speed in MHz             */
    epicsUInt32   *OutputControlWord,           /* Resulting synthesizer control word             */
    epicsInt32     PrintFlag)                   /* Flag to control what we print in this routine  */
{
    /**********************************************************************************************/
    /*  Local Variables                                                                           */
    /**********************************************************************************************/

    epicsFloat64   ActualClockSpeed;            /* Actual frequency generated by the control word */
    epicsFloat64   Error = 0.0;                 /* Error (in ppm) between desired & actual freq.  */

    /**********************************************************************************************/
    /*  Code                                                                                      */
    /**********************************************************************************************/

   /*---------------------
    * Zero the output clock speed and control word parameters just in case there is an error.
    */
    *OutputClockSpeed  = 0.0;
    *OutputControlWord = 0;

   /*---------------------
    * Compute the Fractional Synthesizer control word if one was not provided
    */
    if (0 == InputControlWord) {

       /*---------------------
        * If a clock speed was not specified either, use the default clock speed.
        */
        if (0.0 == InputClockSpeed) {
            InputClockSpeed = MRF_DEF_CLOCK_SPEED;
            DEBUGPRINT (DP_WARN, PrintFlag,
                       (" *Warning: Event clock speed not specified, will default to %f MHz.\n",
                        MRF_DEF_CLOCK_SPEED));
        }/*end if neither the control word nor the clock speed were specified*/

       /*---------------------
        * Compute the control word for the desired clock speed.
        * Abort on error.
        */
        InputControlWord = FracSynthControlWord (
                               InputClockSpeed,
                               ReferenceFreq,
                               PrintFlag,
                               &Error);

        if (0 == InputControlWord) {
            DEBUGPRINT (DP_FATAL, DP_FATAL,
                   (" *Error: Unable to compute fractional synthesizer control word for %f MHz.\n",
                    InputClockSpeed));
            return ERROR;
        }/*end if could not compute fractional synthesizer control word*/

    }/*end if control word not specified*/

   /*---------------------
    * Analyze the control word for programming errors and get the actual frequency it will produce.
    * Abort if there are any serious problems.
    */
    ActualClockSpeed = FracSynthAnalyze (InputControlWord, ReferenceFreq, PrintFlag);
    if (0.0 == ActualClockSpeed) {
        DEBUGPRINT (DP_FATAL, DP_FATAL,
                (" *Error: Fractional Synthesizer control word 0x%08x is invalid.\n",
                 InputControlWord));
        return ERROR;
    }/*end if control word was invalid*/

   /*---------------------
    * If a clock speed was not specified, use the actual frequency generated by the control word.
    */
    if (0.0 == InputClockSpeed)
        InputClockSpeed = ActualClockSpeed;

   /*---------------------
    * Check to make sure the difference between the requested and the actual frequencies
    * is within +/- 100 ppm.
    */
    if (0.0 == Error)
        Error = 1.e6 * (ActualClockSpeed - InputClockSpeed) / InputClockSpeed;

    if (fabs(Error) >= 100.0) {
        DEBUGPRINT (DP_FATAL, DP_FATAL,
                   (" *Error: Requested frequency (%f) and actual frequency (%f) ",
                    InputClockSpeed, ActualClockSpeed));
        DEBUGPRINT (DP_FATAL, DP_FATAL,
                   ("differ by %6.2f ppm.\n         Difference must be less than +/- 100 ppm.\n",
                    Error));
        return ERROR;
    }/*end if error is too large*/

   /*---------------------
    * If we made it this far, everything checked out OK.
    * Return the event clock speed and the fractional sythesizer control word.
    */
    *OutputClockSpeed  = InputClockSpeed;
    *OutputControlWord = InputControlWord;
    return OK;

}/*end mrfSetEventClockSpeed()*/

/**************************************************************************************************
|* FracSynthControlWord () -- Construct Control Word For SY87739L Fractional Synthesizer
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will take a desired output frequency (expressed in MegaHertz)and a reference
|* frequency (also expressed in MegaHertz) and create a control word for the Micrel SY877391L
|* Fractional-N Synthesizer chip that will produce an output frequency as close as possible to
|* the desired frequency.  The routine also returns an error value (expressed in parts-per-million)
|* between the actual output frequency and the desired output frequency.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*
|* A complete description of how the Micrel SY87739L chip works is way beyond the scope of a
|* function header comment.  The reader who wants a complete understanding of what this routine
|* is doing should refer to the Micrel SY87739L product description sheet available at:
|*         www.micrel.com
|*
|* A brief description of the chip programming is useful, however, and is provided here:
|*
|* The output frequency is described by the following formulae:
|*
|*      F(out) = F(vco) / PostDiv
|*
|* where F(vco) is the frequency generated by a voltage-controlled oscillator, and PostDiv is
|* a frequency divider value between 1 and 60 (not inclusive).  The PostDiv value must be chosen
|* so that the VCO frequency is within its operating range of 540 - 729 MHz.  The VCO frequency
|* is given by:
|*
|*      F(vco) = C * F(frac) * F(ref)
|*
|* where C is a "Correction Factor" (also referred to as the "wrapper correction" in the
|* documentation0 expressed as (N/M) where N and M are from the set {14, 15, 16, 17, 18, 31, 32}.
|* Not all combinations, however, are legal.  F(frac) is the fractional frequency produced by a
|* fractional-N P/P-1 divider circuit.  F(ref) is the input reference frequency to the Micrel chip.
|* Typically it is 27 MHz, although in the MRF timing boards it is 24 MHz.  The fractional frequency
|* is basically a multiplier for the reference frequency.  It is expressed as a rational number
|* with a divisor less than 32 and is given by:
|*
|*      F(frac) = P - (Q(p-1) / (Q(p) + Q(p-1)))
|*
|* where P is the integer part of the fractional frequency, Q(p) is the number of clock periods
|* where the reference clock is divided by P and Q(p-1) is the number of clock periods where the
|* reference clock is divided by P-1.  A hardware implementation of Bresenham's algorithm is
|* used to evenly space out the P and P-1 divisions.  Beyond this, you'll have to read the
|* manual for further details.
|*
|* The routine works by doing an exhaustive search (with some optimizations) of the parameter space
|* {C, Q(p), Q(p-1), PostDiv}.  P is predetermined by F(out) and PostDiv, so it does not need to
|* be searched.  The Q(p), Q(p-1) search is accomplished by searching all the numerators for
|* the denominator (Q(p) + Q(p-1)) that produce a fraction less than 1.  Although an exhaustive
|* search is not the most efficient way to optimize a 4-parameter space, it turns out that the
|* dimensions of each parameter are not that big.  PostDiv only has 31 unique values, and we
|* only search the values that produce a valid F(vco).  When you rule out duplications and
|* illegal combinations it turns out that C only has 22 valid values, and if you arrange them
|* in ascending order, you only have to search until the error value stops decreasing.  For
|* Q(p) and Q(p-1), we only have to search 31 denominators and for each denominator we only have
|* to search n-1 numerators.  This gives us n(n+1)/2, or 465 (for n=30) possible
|* numerator/denominator pairs to search, although we stop immediately if we find a denominator
|* that exactly divides the desired fractional frequency.
|*
|* If we search 22 correction factors for each numerator/denominator pair, and we repeat the
|* process for each of 31 possible PostDiv values, we get a worst case number of 317,130 possible
|* combinations. In practice, the VCO frequency limitations (between 540 and 729 Megahertz) will
|* usually eliminate most of the 31 possible PostDiv values.  Given the other optimizations
|* mentioned above, the worst case number is a pretty pathological example.  In most cases the
|* number of combinations actually searched will be far less than that.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      controlWord = FracSynthControlWord (DesiredFreq, ReferenceFreq, debugFlag, &Error);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      DesiredFreq    = (epicsFloat64)      Desired output frequency in MegaHertz.
|*      ReferenceFreq  = (epicsFloat64)      SY87739L input reference frequency in MegaHertz.
|*      debugFlag      = (epicsInt32)        Flag for debug output.  If the value is 4 (DP_INFO)
|*                                           or higher, the routine will print the value of the
|*                                           control word and the frequency actually produced.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*      Error          = (epicsFloat64)      Error between the actual output frequency and the
|*                                           desired output frequency.  Expressed in
|*                                           parts-per-million.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      CorrectionList = (CorrectionStruct)  List of all valid correction factors and their
|*                                           control-word encodings.
|*
|*      PostDivideList = (PostDivideStruct)  List of valid post-divide values and their
|*                                           control-word encodings.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      controlWord    = (epicsUInt32)       SY87739L Control Word for the desired frequency.
|*
\**************************************************************************************************/


epicsUInt32 FracSynthControlWord (

    /**********************************************************************************************/
    /*  Parameter Declarations                                                                    */
    /**********************************************************************************************/

    epicsFloat64         DesiredFreq,         /* Desired output frequency                         */
    epicsFloat64         ReferenceFreq,       /* SY87739L input reference frequency               */
    epicsInt32           debugFlag,           /* Flag for debug/informational output              */
    epicsFloat64        *Error)               /* Error value (in parts per million)               */
{

    /**********************************************************************************************/
    /*  Local Variables                                                                           */
    /**********************************************************************************************/

    FracSynthComponents  Best;                /* Best overall parameters seen so far              */
    FracSynthComponents  BestFracFreq;        /* Best fractional frequency parameters seen so far */
    epicsUInt32          ControlWord;         /* Computed control word value                      */
    epicsFloat64         CorrectionErr;       /* Error value for current correction factor        */
    epicsFloat64         CorrectionFreq;      /* Frequency after applying the correction factor   */
    epicsInt32           CorrectionIndex;     /* Index to correction factor list                  */
    epicsFloat64         d;                   /* FP representation of the current denominator     */
    epicsFloat64         EffectiveFreq = 0.0; /* Effective output frequency                       */
    epicsFloat64         FracFreqErr;         /* Best fractional frequency error seen so far      */
    epicsFloat64         FractionalFreq;      /* Desired fractional frequency                     */
    epicsFloat64         FreqErr;             /* Error value for the desired frequency            */
    epicsInt32           i, j, k, n;          /* Loop indicies                                    */
    epicsInt32           Numerator = 1;       /* Best numerator found for current denominator     */
    epicsFloat64         OldCorrectionErr;    /* Error value for previous correction factor       */
    epicsInt32           p;                   /* Integer part of fractional frequency             */
    epicsInt32           p1;                  /* Numerator of fractional frequency ratio          */
    epicsFloat64         PostDivide;          /* Current post divider value                       */
    epicsInt32           Qp;                  /* Number of "P" pulses in the frac. div. circuit   */
    epicsInt32           Qpm1;                /* Number of "P-1" pulses in the frac. div. circuit */
    epicsFloat64         TestFreq;            /* Fractional frequency to try in correction loop   */
    epicsFloat64         VcoFreq;             /* Desired VCO frequency                            */

    /**********************************************************************************************/
    /*  Code                                                                                      */
    /**********************************************************************************************/

   /*---------------------
    * Initialize the "Best Fractional Frequency So Far" parameters
    */
    BestFracFreq.Error           = MAX_ERROR;   /* Deviation from desired output frequency        */
    BestFracFreq.EffectiveFreq   = 0.0;         /* Actual frequency generated by these components */
    BestFracFreq.PostDivIndex    = 0;           /* Index into post-divider table                  */
    BestFracFreq.CorrectionIndex = 0;           /* Index into correction factor table             */
    BestFracFreq.P               = 0;           /* Integer part of fractional frequency           */
    BestFracFreq.Qp              = 1;           /* Value of Q(p) term of fractional frequency     */
    BestFracFreq.Qpm1            = 1;           /* Value of Q(p-1) term of fractional frequency   */

    Best = BestFracFreq;                        /* Best overall parameters                        */

   /*---------------------
    * Post-Divider Loop:
    *---------------------
    *  Check all post-divider values that would put the VCO frequency within
    *  the allowable range (540 - 729 MHz).
    */
    Best.Error = MAX_ERROR;
    for (i=0;  i < NUM_POST_DIVIDES;  i++) {
        PostDivide = PostDivideList[i].Divisor;

       /*---------------------
        * Compute the VCO frequency and make sure it is within the allowable range.
        */
        VcoFreq = DesiredFreq * PostDivide;

        if (VcoFreq >= MAX_VCO_FREQ)
            break;

        if (VcoFreq >= MIN_VCO_FREQ) {

           /*---------------------
            * We have a VCO frequency inside the allowable range.
            * Now compute the desired fractional frequency.
            *
            * The desired fractional frequency is derived by dividing the VCO frequency by the
            * reference frequency.  The actual fractional-N frequency is created by a fractional-N
            * P/P-1 divider which attempts to represent the desired fractional frequency as a
            * rational fraction (with a divisor less than 32) of the reference frequency.
            *
            * Note that the actual fractional-N frequency may not be exactly identical to the
            * desired fractional frequency.
            */
            FractionalFreq = VcoFreq / ReferenceFreq;
            BestFracFreq.Error = MAX_ERROR;

           /*---------------------
            * Divisor Loop:
            *--------------------- 
            *  Search for a divisor that will produce the closest fractional-N P/P-1 frequency to
            *  the desired fractional frequency.  If we are lucky, we will find a number that
            *  exactly divides the desired fractional frequency and we can stop the search here.
            *  If not, we will have to try different numerator and correction factor combinations
            *  in order to find the closest fit.
            */
            for (j=1;  j <= MAX_FRAC_DIVISOR;  j++) {
                d = j;                  /* Floating point representation of denominator  */
                CorrectionIndex = 0;    /* No correction factor                          */

               /*---------------------
                * Compute the integer and fractional parts of the fractional-N frequency
                */
                p1 = (FractionalFreq * j);
                p = (p1 / j) + 1;
                Qpm1 = j - (p1 % j);
                Qp = j - Qpm1;

               /*---------------------
                * Compute the actual frequency generated by the fractional-N P/P-1 divider
                * for these values.  Also compute the error between the actual fractional
                * frequency and the desired fractional frequency.
                */
                EffectiveFreq = (double)p - ((double)Qpm1 / (double)(Qp + Qpm1));
                FracFreqErr = fabs (FractionalFreq - EffectiveFreq);

               /*---------------------
                * If the current divisor does not exactly divide the desired fractional frequency,
                * search for a numerator and correction-factor pair that will come closest to
                * generating the desired fractional frequency.
                */
                if (FracFreqErr >= ZERO_THRESHOLD) {
                    FracFreqErr = MAX_ERROR;

                   /*---------------------
                    * Numerator Loop:
                    *---------------------
                    *  Search for the numerator/correction-factor pair with the lowest error.
                    */
                    for (n = 1;  n <= j;  n++) {
                        TestFreq = (double)p - ((double)n / d);
                        OldCorrectionErr = MAX_ERROR;

                       /*---------------------
                        * Correction Factor Loop:
                        *---------------------
                        *  Search for the correction factor that produces the lowest error
                        *  with the current numerator/denominator pair.  Note that since the
                        *  correction factor list is arranged in ascending order, we can
                        *  stop the search as soon as the error value stops decreasing.
                        */
                        for (k = 1;  k < NUM_CORRECTIONS;  k++) {
                            CorrectionFreq = CorrectionList[k].Ratio * TestFreq;
                            CorrectionErr = fabs (FractionalFreq - CorrectionFreq);
                            if (CorrectionErr > OldCorrectionErr) break;
                            OldCorrectionErr = CorrectionErr;

                           /*---------------------
                            * If we found a numerator/correction-factor pair with a lower error
                            * than the previous error for this denominator, replace the previous
                            * values.
                            */
                            if (CorrectionErr < FracFreqErr) {
                                EffectiveFreq = CorrectionFreq;
                                FracFreqErr = CorrectionErr;
                                Numerator = n;
                                CorrectionIndex = k;
                            }/*end if we found a better numerator/correction pair*/

                        }/*end correction factor loop*/
                    }/*end numerator loop*/

                   /*---------------------
                    * At this point, we now have the best numerator/correction-factor pair
                    * for the current denominator.  Recompute the Q(p) and Q(p-1) values
                    * based on the new numerator value.
                    *
                    * It could happen that the numerator/denominator/correction set we found
                    * produces the desired fractional frequency exactly (FracFreqErr == 0).
                    * If this occurs, we will set FracFreqErr to the zero-threshold value so that
                    * the algorithm gives preference to solutions that do not require a correction
                    * factor.
                    */
                    Qpm1 = Numerator;
                    Qp = j - Numerator;
                    if (FracFreqErr < ZERO_THRESHOLD)
                        FracFreqErr = ZERO_THRESHOLD;

                }/*end denominator does not exactly divide the desired fractional frequency*/
 
               /*---------------------
                * Store the parameters for the numerator/correction pair that produces the lowest
                * fractional frequency error for this denominator.  If it turns out that the
                * denominator exactly divides the desired fractional frequency (FracFreqErr == 0),
                * exit the denominator loop now.
                */
                if (FracFreqErr < BestFracFreq.Error) {
                    BestFracFreq.Error = FracFreqErr;
                    BestFracFreq.EffectiveFreq = EffectiveFreq;
                    BestFracFreq.P = p;
                    BestFracFreq.Qp = Qp;
                    BestFracFreq.Qpm1 = Qpm1;
                    BestFracFreq.CorrectionIndex = CorrectionIndex;
                    if (FracFreqErr < ZERO_THRESHOLD) break;
                }/*end if fractional frequency is best so far*/

            }/*end denominator loop*/

           /*---------------------
            * Adjust the fractional frequency error for the current post divider.
            * If the adjusted frequency error is less than the current best, make
            * this the current best.  If the new parameters produce an exact match
            * (FreqErr == 0), exit the post-divider loop now.
            */
            FreqErr = (BestFracFreq.Error * ReferenceFreq) / PostDivide;
            if (FreqErr < Best.Error) {
                Best = BestFracFreq;
                Best.PostDivIndex = i;
                Best.Error = FreqErr;
                Best.EffectiveFreq = (BestFracFreq.EffectiveFreq * ReferenceFreq) / PostDivide;
                if (FreqErr < ZERO_THRESHOLD) break;
            }/*end if this post-divide solution is the best so far*/

        }/*end if VCO frequency is within range*/
    }/*end for each post divider*/

   /*---------------------
    * Abort if we could not come up with a set of parameters that would produce the
    * desired frequency.
    */
    if (MAX_ERROR == Best.Error) return 0;

   /*---------------------
    * Construct the control word from the parameters found in the search above.
    */
    CorrectionIndex = Best.CorrectionIndex;
    ControlWord =
      ((CorrectionList[CorrectionIndex].mDiv   << CONTROL_MDIV_SHIFT)    & CONTROL_MDIV_MASK)      |
      ((CorrectionList[CorrectionIndex].nDiv   << CONTROL_NDIV_SHIFT)    & CONTROL_NDIV_MASK)      |
      ((PostDivideList[Best.PostDivIndex].Code << CONTROL_POSTDIV_SHIFT) & CONTROL_POSTDIV_MASK)   |
      (((Best.P - 1)                           << CONTROL_P_SHIFT)       & CONTROL_P_MASK)         |
      ((Best.Qpm1                              << CONTROL_QPM1_SHIFT)    & CONTROL_QPM1_MASK)      |
      ((Best.Qp                                << CONTROL_QP_SHIFT)      & CONTROL_QP_MASK);

   /*---------------------
    * Compute the frequency that will actually be generated from these parameters.
    */
    EffectiveFreq = CorrectionList[CorrectionIndex].Ratio * ReferenceFreq *
                     ((double)Best.P - ((double)Best.Qpm1 /(double)(Best.Qp + Best.Qpm1))) /
                       PostDivideList[Best.PostDivIndex].Divisor;

   /*---------------------
    * Return the error (in parts-per-million) between the desired and actual frequencies.
    */
    *Error = 1.e6 * (EffectiveFreq - DesiredFreq) / DesiredFreq;

   /*---------------------
    * Output debug information about the results of this call
    */
    DEBUGPRINT (DP_INFO, debugFlag,
               ("Desired Frequency = %f,  Control Word = %08X\n", DesiredFreq, ControlWord));
    DEBUGPRINT (DP_INFO, debugFlag,
               ("Effective Frequency = %15.12f, Error = %5.3f ppm\n", EffectiveFreq, *Error));

   /*---------------------
    * Return the computed control word.
    */
    return ControlWord;

}/*end FracSynthControlWord()*/

/**************************************************************************************************
|* FracSynthAnalyze () -- Analyze an SY87739L Fractional Synthesizer Control Word
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will take a bit pattern representing an SY87739L fractional synthesizer
|* control word, break it down into its constituent parts, and return the effective output
|* frequency (in MegaHertz) that the control word will generate.  Depending on the value of
|* the "PrintFlag" parameter, it will also display the constituent parts of the the control
|* word and the resulting values that make up the output frequency, and analyze the control
|* word for programming errors.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      OutputFreq = FracSynthAnalyze (ControlWord, ReferenceFreq, PrintFlag);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      ControlWord       = (epicsUInt32)         The control word bit pattern to analyze.
|*      ReferenceFreq     = (epicsFloat64)        SY87739L input reference frequency in MegaHertz.
|*      PrintFlag         = (epicsInt32)          Flag to control output messages.  Output levels
|*                                                correspond to the DEBUGPRINT output levels:
|*                                                DP_NONE  (0) = No printed output, just return the
|*                                                               effective output frequency.
|*                                                DP_ERROR (2) = Display any programming errors
|*                                                               detected in the control word
|*                                                DP_DEBUG (5) = Display the actual values of the
|*                                                               fields in the control word, along
|*                                                               with the constituent parts of the
|*                                                               output frequency (VCO frequency,
|*                                                               correction term, reference freq.)
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      CorrectionValList = (CorrectionValStruct) Array to translate correction term components
|*                                                from their coded values to actual values and
|*                                                classes.
|*
|*      PostDivideValList = (epicsInt32)          Array to translate the post-divider field from
|*                                                its coded value to its actual value.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      OutputFreq        = (epicsFloat64)        Output frequency generated by the control word.
|*                                                Returns 0.0 if the analysis discovered
|*                                                programming errors in the control word.
|*
\**************************************************************************************************/


epicsFloat64 FracSynthAnalyze (

    /**********************************************************************************************/
    /*  Parameter Declarations                                                                    */
    /**********************************************************************************************/

    epicsUInt32    ControlWord,                 /* Control word to analyze                        */
    epicsFloat64   ReferenceFreq,               /* SY87739L input reference frequency             */
    epicsInt32     PrintFlag)                   /* Flag to control what we print in this routine  */
{
    /**********************************************************************************************/
    /*  Local Variables                                                                           */
    /**********************************************************************************************/

    epicsFloat64   CorrectionTerm;
    epicsFloat64   EffectiveFreq = 0.0;         /* Computed effective output frequency            */
    epicsBoolean   Error = epicsFalse;          /* Error flag from control word analysis          */
    epicsInt32     i;                           /* Local loop counter                             */
    epicsUInt32    MDiv;                        /* Coded value of the correction term denominator */
    epicsInt32     MDivClass;                   /* Class designator of the correction denominator */
    epicsFloat64   MDivVal;                     /* Actual value of the correction term denominator*/
    epicsUInt32    Mfg;                         /* MFG field extracted from the control word      */
    epicsUInt32    NDiv;                        /* Coded value of the correction term numerator   */
    epicsInt32     NDivClass;                   /* Class designator of the correction numerator   */
    epicsFloat64   NDivVal;                     /* Actual value of the correction term numerator  */
    epicsUInt32    P;                           /* Coded value of the frac freq integer component */
    epicsFloat64   PVal;                        /* Actual value of the frac freq integer component*/
    epicsUInt32    PostDivide;                  /* Coded value of the post-divider                */
    epicsFloat64   PostDivideVal;               /* Actual value of the post-divider               */
    epicsUInt32    Preamble;                    /* Preamble field extracted from the control word */
    epicsUInt32    Qp;                          /* Number of times to divide by P                 */
    epicsUInt32    Qpm1;                        /* Number of times to divide by P-1               */
    epicsFloat64   VcoFreq;                     /* Computed VCO frequency                         */

    /**********************************************************************************************/
    /*                                        Code                                                */
    /*                                                                                            */

   /*---------------------
    * Decompose the control word into its constituent elements
    */
    Preamble   = (ControlWord & CONTROL_PREAMBLE_MASK) >> CONTROL_PREAMBLE_SHIFT;
    Qp         = (ControlWord & CONTROL_QP_MASK)       >> CONTROL_QP_SHIFT;
    Qpm1       = (ControlWord & CONTROL_QPM1_MASK)     >> CONTROL_QPM1_SHIFT;
    P          = (ControlWord & CONTROL_P_MASK)        >> CONTROL_P_SHIFT;
    Mfg        = (ControlWord & CONTROL_MFG_MASK)      >> CONTROL_MFG_SHIFT;
    PostDivide = (ControlWord & CONTROL_POSTDIV_MASK)  >> CONTROL_POSTDIV_SHIFT;
    NDiv       = (ControlWord & CONTROL_NDIV_MASK)     >> CONTROL_NDIV_SHIFT;
    MDiv       = (ControlWord & CONTROL_MDIV_MASK)     >> CONTROL_MDIV_SHIFT;

   /*---------------------
    * Translate the elements of the correction term fraction from their encoded values
    * to their actual values.
    */
    MDivVal = (double)CorrectionValList[MDiv].Value;
    NDivVal = (double)CorrectionValList[NDiv].Value;

   /*---------------------
    * Obtain the class number for each of the elements of the correction term fraction.
    */
    MDivClass = CorrectionValList[MDiv].Class;
    NDivClass = CorrectionValList[NDiv].Class;

   /*---------------------
    * Translate the integer part (P) if the fractional frequency and the post-divider from
    * their encoded values to their actual values.
    */
    PVal = (double)(P + MIN_P_VALUE);
    PostDivideVal = (double)PostDivideValList[PostDivide];

    /**********************************************************************************************/
    /*  Check for fatal errors that would prevent us from completing the analysis                 */
    /**********************************************************************************************/

    DEBUGPRINT (DP_DEBUG, PrintFlag,
               ("Analysis of Control Word 0x%08X:\n", ControlWord));

   /*---------------------
    * Check for fractional frequency denominator being zero.
    */
    if ((Qp + Qpm1) == 0) {
        DEBUGPRINT (DP_FATAL, PrintFlag, (" *Error: Q(p) + Q(p-1) [%d + %d] is 0.\n", Qp, Qpm1));
        Error = epicsTrue;
    }/*end if fractional denominator is zero*/

   /*---------------------
    * Abort if there are any fatal errors that would prevent us from computing the effective
    * frequency.
    */
    if (Error) return 0.0;

    /**********************************************************************************************/
    /*  Analyze the control word                                                                  */
    /**********************************************************************************************/

   /*---------------------
    * Compute the components of the effective output frequency generated by this control word.
    */
    CorrectionTerm = NDivVal / MDivVal;
    VcoFreq =  (PVal - ((double)Qpm1 / (double)(Qp + Qpm1))) * CorrectionTerm * ReferenceFreq;
    EffectiveFreq = VcoFreq / PostDivideVal;

   /*---------------------
    * Display the control word fields and how they combine to produce the effective output
    * frequency.
    */
    Error = epicsFalse;
    DEBUGPRINT (DP_DEBUG, PrintFlag,
               ("  P = %d,  Q(p) = %d,  Q(p-1) = %d,  Post Divider = %d\n",
               (int)PVal, Qp, Qpm1, (int)PostDivideVal));
    DEBUGPRINT (DP_DEBUG, PrintFlag,
               ("  Correction Term (N/M) = %d/%d = %f,  Reference Frequency = %3.1f MHz.\n",
               (int)NDivVal, (int)MDivVal, CorrectionTerm, ReferenceFreq));
    DEBUGPRINT (DP_DEBUG, PrintFlag,
               ("  VCO Frequency = %f MHz.  Effective Frequency = %15.12f MHz.\n",
               VcoFreq, EffectiveFreq));

    /**********************************************************************************************/
    /*  Check for error conditions                                                                */
    /**********************************************************************************************/

   /*---------------------
    * Check for preamble field not zero
    */
    if (Preamble != 0) {
        Error = epicsTrue;
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   (" *Error: PREAMBLE field (bits %d-%d) is 0x%X.\n",
                   CONTROL_PREAMBLE_SHIFT, CONTROL_PREAMBLE_SHIFT+CONTROL_PREAMBLE_BITS-1,
                   Preamble));
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   ("         Should be 0x0.\n"));
    }/*end if preamble field is not zero*/

   /*---------------------
    * Check for MFG field not zero
    */
    if (Mfg != 0) {
        Error = epicsTrue;
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   (" *Error: MFG field (bits %d-%d) is 0x%X.\n",
                   CONTROL_MFG_SHIFT, CONTROL_MFG_SHIFT+CONTROL_MFG_BITS-1, Mfg));
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   ("         Should be 0x0.\n"));
    }/*end if mfg field is not zero*/

   /*---------------------
    * Check for fractional denominator too big.
    * (one more than the maximum actually does seem to work, so we'll allow it to pass).
    */
    if ((Qp + Qpm1) > MAX_FRAC_DIVISOR) {
        if ((Qp + Qpm1) > (MAX_FRAC_DIVISOR + 1))
            Error = epicsTrue;
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   (" *Error: Q(p) + Q(p-1) [%d + %d] is %d.\n", Qp, Qpm1, (Qp+Qpm1)));
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   ("         Sum should be less than or equal to %d.\n", MAX_FRAC_DIVISOR));
    }/*end if fractional divisor is too large*/

   /*---------------------
    * Check for correction term too big.
    */
    if ((NDivVal / MDivVal) > MAX_CORRECTION_RATIO) {
        Error = epicsTrue;
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   (" *Error: Correction Term Ratio = (N/M) = (%d/%d) = %f is too big.\n",
                   (int)NDivVal, (int)MDivVal, (NDivVal/MDivVal)));
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   ("         Should be less than (17/14) = %f\n", MAX_CORRECTION_RATIO));
    }/*end if correction term ratio is too big*/

   /*---------------------
    * Check for correction term numerator and denominator being from different classes.
    */
    if (NDivClass != MDivClass) {
        Error = epicsTrue;
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   (" *Error: Correction Term numerator (%d) is not compatible with",
                   (int)NDivVal));
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   (" Correction Term Denominator (%d).\n", (int)MDivVal));
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   ("         Valid numerator values are:"));
        for (i=1;  i < NUM_CORRECTION_VALS;  i++) {
            if (CorrectionValList[i].Class == MDivClass)
                DEBUGPRINT (DP_ERROR, PrintFlag, (" %d", CorrectionValList[i].Value));
        }/*end search for valid numerators*/

        DEBUGPRINT (DP_ERROR, PrintFlag, (".\n"));
    }/*end if correction term numerator/denominator pair is invalid*/

   /*---------------------
    * Check for VCO frequency out of range
    */
    if ((VcoFreq > MAX_VCO_FREQ) || (VcoFreq < MIN_VCO_FREQ)) {
        Error = epicsTrue;
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   (" *Error: VCO Frequency (%f MHz.) is outside the valid range.\n", VcoFreq));
        DEBUGPRINT (DP_ERROR, PrintFlag,
                   ("         Should be between %5.1f MHz. and %5.1f MHz.\n",
                    MIN_VCO_FREQ, MAX_VCO_FREQ));
    }/*end if VCO Frequency is out of range*/

   /*---------------------
    * If the analysis did not uncover any serious errors, return the effective output frequency.
    */
    if (Error) return 0.0;
    else       return EffectiveFreq;

}/*end FracSynthAnalyze()*/

/**************************************************************************************************/
/*                              EPICS IOC Shell Registery                                         */
/*                                                                                                */


/**************************************************************************************************/
/*   FracSynthControlWord() -- Construct a Fractional Synthesizer Control Word                    */
/**************************************************************************************************/

#ifndef HOST_BUILD

static const iocshArg         FracSynthControlWordArg0    = {"DesiredFreq", iocshArgDouble};
static const iocshArg *const  FracSynthControlWordArgs[1] = {&FracSynthControlWordArg0};
static const iocshFuncDef     FracSynthControlWordDef     = {"FracSynthControlWord", 1,
                                                            FracSynthControlWordArgs};

static
void FracSynthControlWordCall (const iocshArgBuf *args) {
    epicsFloat64  Dummy;
    FracSynthControlWord (args[0].dval, MRF_FRAC_SYNTH_REF, DP_DEBUG, &Dummy);
}/*end FracSynthControlWordCall()*/


/**************************************************************************************************/
/*   FracSynthAnalyze() -- Analyze a Fractional Synthesizer Control Word                          */
/**************************************************************************************************/

static const iocshArg         FracSynthAnalyzeArg0    = {"ControlWord", iocshArgInt};
static const iocshArg *const  FracSynthAnalyzeArgs[1] = {&FracSynthAnalyzeArg0};
static const iocshFuncDef     FracSynthAnalyzeDef     = {"FracSynthAnalyze", 1,
                                                        FracSynthAnalyzeArgs};

static
void FracSynthAnalyzeCall (const iocshArgBuf *args) {
    FracSynthAnalyze (args[0].ival, MRF_FRAC_SYNTH_REF, DP_DEBUG);
}/*end FracSynthAnalyzeCall()*/


/**************************************************************************************************/
/*  EPICS Registrar Function for this Module                                                      */
/**************************************************************************************************/

static
void FracSynthRegistrar () {
    iocshRegister (&FracSynthControlWordDef , FracSynthControlWordCall );
    iocshRegister (&FracSynthAnalyzeDef     , FracSynthAnalyzeCall);
}/*end FracSynthRegistrar()*/

epicsExportRegistrar (FracSynthRegistrar);

#endif
