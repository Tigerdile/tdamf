/*
 * endian.hpp
 *
 * File to manage #define's for endian-ness.
 *
 * This was lifted entirely from librtmp - they did a very good
 * job at sorting out endian-ness on a level above and beyond
 * anything else I've seen.
 *
 * It has been lightly modified by sconley but credit goes here:
 *
 * https://rtmpdump.mplayerhq.hu/
 */

#ifndef __ENDIAN_HPP__
#define __ENDIAN_HPP__

/*
 * Define byte order if not defined
 *
 * These #define's are totally cribbed from librtmp
 * Though I don't think there's a better way to do this.
 *
 * This is needed for converting double types.
 */
#if defined(BYTE_ORDER) && !defined(__BYTE_ORDER)
#   define __BYTE_ORDER    BYTE_ORDER
#endif

#if defined(BIG_ENDIAN) && !defined(__BIG_ENDIAN)
#   define __BIG_ENDIAN    BIG_ENDIAN
#endif

#if defined(LITTLE_ENDIAN) && !defined(__LITTLE_ENDIAN)
#   define __LITTLE_ENDIAN LITTLE_ENDIAN
#endif

/* define default endianness */
#ifndef __LITTLE_ENDIAN
#   define __LITTLE_ENDIAN    1234
#endif

#ifndef __BIG_ENDIAN
#   define __BIG_ENDIAN    4321
#endif

#ifndef __BYTE_ORDER
#   warning "Byte order not defined on your system, assuming little endian!"
#   define __BYTE_ORDER    __LITTLE_ENDIAN
#endif

/* ok, we assume to have the same float word order and byte order
 * if float word order is not defined 
 */
#ifndef __FLOAT_WORD_ORDER
#   warning "Float word order not defined, assuming the same as byte order!"
#   define __FLOAT_WORD_ORDER    __BYTE_ORDER
#endif

#if !defined(__BYTE_ORDER) || !defined(__FLOAT_WORD_ORDER)
#   error "Undefined byte or float word order!"
#endif

#if __FLOAT_WORD_ORDER != __BIG_ENDIAN && __FLOAT_WORD_ORDER != __LITTLE_ENDIAN
#   error "Unknown/unsupported float word order!"
#endif

#if __BYTE_ORDER != __BIG_ENDIAN && __BYTE_ORDER != __LITTLE_ENDIAN
#   error "Unknown/unsupported byte order!"
#endif

// If we're debugging, let's add debugging commands
#ifdef DEBUG
#   include <iostream>
#   define LOG(s)   std::cout << "DEBUG: " << s << std::endl
#else
#   define LOG(s)
#endif


#endif
