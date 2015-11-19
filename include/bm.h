// *************************************************************************************************
// First Section: Basic Data Types
// *************************************************************************************************

// Fundamental #definitions
// CPU target idents are used for target dependent compilations

// Texas Instruments MSP430
#define _TI_MSP430_  (16)

#ifndef _ASSEMBLER_USED_
// Get the limits to autodetect the size of integral types
#    include <limits.h>
// Get floats to autodetect the size of float types
#    include <float.h>

// ***********************************************************************************************
//
// Common basic data types
//
// ***********************************************************************************************
#    if UCHAR_MAX == 0xFFu
#        define _CPU_8BIT_INT_ char
#    else
#        error "unable to get size of u8 automatically"
#    endif

#    if USHRT_MAX == 0xFFFFu
#        define _CPU_16BIT_INT_ short
#    elif UINT_MAX == 0xFFFFu
#        define _CPU_16BIT_INT_ int
#    else
#        error "unable to get size of u16 automatically"
#    endif

#    if USHRT_MAX == 0xFFFFFFFFu
#        define _CPU_32BIT_INT_ short
#    elif UINT_MAX == 0xFFFFFFFFu
#        define _CPU_32BIT_INT_ int
#    elif ULONG_MAX == 0xFFFFFFFFu
#        define _CPU_32BIT_INT_ long
#    else
#        error "unable to get size of u32 automatically"
#    endif

// ***********************************************************************************************
//
// Following lines #typedef the basic data types in a compiler independent way.
//
// ***********************************************************************************************

#    ifdef _CPU_8BIT_INT_
// unsigned 8 bit
typedef unsigned _CPU_8BIT_INT_ u8;
// unsigned 8 bit max value
#        define U8_MAX (0xFFU)
// signed 8 bit max value
typedef signed _CPU_8BIT_INT_ s8;
// signed 8 bit min value
#        define S8_MIN (-127 - 1)
// signed 8 bit max value
#        define S8_MAX (127)
#    endif

#    ifdef _CPU_16BIT_INT_
// unsigned 16 bit
typedef unsigned _CPU_16BIT_INT_ u16;
// unsigned 16 bit max value
#        define U16_MAX (0xFFFFU)
// signed 16 bit
typedef signed _CPU_16BIT_INT_ s16;
// signed 16 bit min value
#        define S16_MIN (-32767 - 1)
// signed 16 bit max value
#        define S16_MAX (32767)
#    endif

#    ifdef _CPU_32BIT_INT_
// unsigned 32 bit
typedef unsigned _CPU_32BIT_INT_ u32;
// unsigned 32 bit max value
#        define U32_MAX (0xFFFFFFFFUL)
// signed 32 bit
typedef signed _CPU_32BIT_INT_ s32;
// signed 32 bit min value
#        define S32_MIN (-2147483647L - 1L)
// signed 32 bit max value
#        define S32_MAX (2147483647L)
#    endif

#endif                          // _ASSMEBLER_USED_
