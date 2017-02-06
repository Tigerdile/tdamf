/*
 * amf.hpp
 *
 * Classes for handling AMF encoding / decoding.
 *
 * @author sconley
 * Copyright 2017 Tigerdile LLC
 *********************************************************************
 *
 * This was very inspired by the librtmp code which is here:
 *
 * https://rtmpdump.mplayerhq.hu/
 *
 */

#ifndef __AMF_HPP__
#define __AMF_HPP__

#include <map>
#include <vector>

#include <cstring>
#include <arpa/inet.h>
#include <sys/param.h>
#include <stdint.h>

/*
 * Define byte order if not defined
 *
 * These #define's are totally cribbed from librtmp
 * Though I don't think there's a better way to do this.
 */
#if defined(BYTE_ORDER) && !defined(__BYTE_ORDER)
#define __BYTE_ORDER    BYTE_ORDER
#endif

#if defined(BIG_ENDIAN) && !defined(__BIG_ENDIAN)
#define __BIG_ENDIAN	BIG_ENDIAN
#endif

#if defined(LITTLE_ENDIAN) && !defined(__LITTLE_ENDIAN)
#define __LITTLE_ENDIAN	LITTLE_ENDIAN
#endif

/* define default endianness */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN	1234
#endif

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN	4321
#endif

#ifndef __BYTE_ORDER
#warning "Byte order not defined on your system, assuming little endian!"
#define __BYTE_ORDER	__LITTLE_ENDIAN
#endif

/* ok, we assume to have the same float word order and byte order if float word order is not defined */
#ifndef __FLOAT_WORD_ORDER
#warning "Float word order not defined, assuming the same as byte order!"
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#endif

#if !defined(__BYTE_ORDER) || !defined(__FLOAT_WORD_ORDER)
#error "Undefined byte or float word order!"
#endif

#if __FLOAT_WORD_ORDER != __BIG_ENDIAN && __FLOAT_WORD_ORDER != __LITTLE_ENDIAN
#error "Unknown/unsupported float word order!"
#endif

#if __BYTE_ORDER != __BIG_ENDIAN && __BYTE_ORDER != __LITTLE_ENDIAN
#error "Unknown/unsupported byte order!"
#endif


namespace Tigerdile
{
/*****************************************************************************
 * AMF
 *
 * Active Message Format handling code.  This translates AMF into stuff
 * we can use elsewhere.
 *****************************************************************************/

    class AMF
    {
        public:
            /*
             * This will be used all over
             */
            struct Value
            {
                const char* val;
                uint32_t    len;

                bool operator<(const Value &o) const
                {
                    if(o.len < len) { // shorter string is always smaller
                        return false;
                    }

                    if(o.len != len) {
                        return true;
                    }

                    return (strncmp(val, o.val, len) < 0);
                }

                bool operator==(const Value &o) const
                {
                    if(o.len != len) {
                        return false;
                    }

                    return !strncmp(val, o.val, len);
                }
            };

            struct Property
            {
                union
                {
                    double  number;
                    Value   value;
                    AMF*    object;
                } property;

                unsigned char    type;
            };

            /*
             * Simple decoders
             *
             * TODO: Use my own Endian library, I think its superior to this
             * OR use Boost Endian
             *
             * Performance-wise, I'm way better casting data as an integer
             * as I do for ntohl, etc.
             */
            static inline uint32_t decodeInt24(const unsigned char* data)
            {
                return (data[0] << 16) | (data[1] << 8) | data[2];
            }

            static inline uint32_t decodeInt24(const char* data)
            {
                return AMF::decodeInt24((const unsigned char*) data);
            }

            static inline uint32_t decodeInt32LE(const unsigned char* data)
            {
#if __BYTE_ORDER == __BIG_ENDIAN
                return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
#else
                return (*((uint32_t*)data));
#endif
            }

            static inline uint32_t decodeInt32LE(const char* data)
            {
                return AMF::decodeInt32LE((const unsigned char*) data);
            }

            static inline uint16_t decodeInt16(const char* data)
            {
                return ntohs(*((uint16_t*)data));
            }

            static inline uint32_t decodeInt32(const char* data)
            {
                return ntohs(*((uint32_t*)data));
            }

            /*
             * This method is shamelessly ripped off verbatim from
             * librtmp -- I couldn't figure out a way to implement
             * it any better.
             */
            static inline double decodeNumber(const char* data)
            {
#if __FLOAT_WORD_ORDER == __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
                return (*((double*)data));
#elif __BYTE_ORDER == __LITTLE_ENDIAN
                double ret;
                unsigned char *ci, *co;
                ci = (unsigned char *)data;
                co = (unsigned char *)&ret;
                co[0] = ci[7];
                co[1] = ci[6];
                co[2] = ci[5];
                co[3] = ci[4];
                co[4] = ci[3];
                co[5] = ci[2];
                co[6] = ci[1];
                co[7] = ci[0];
                return ret;
#endif
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN	/* __FLOAT_WORD_ORER == __BIG_ENDIAN */
                double ret;
                unsigned char *ci, *co;
                ci = (unsigned char *)data;
                co = (unsigned char *)&ret;
                co[0] = ci[3];
                co[1] = ci[2];
                co[2] = ci[1];
                co[3] = ci[0];
                co[4] = ci[7];
                co[5] = ci[6];
                co[6] = ci[5];
                co[7] = ci[4];
                return ret;
#else /* __BYTE_ORDER == __BIG_ENDIAN && __FLOAT_WORD_ORER == __LITTLE_ENDIAN */
                double ret;
                unsigned char *ci, *co;
                ci = (unsigned char *)data;
                co = (unsigned char *)&ret;
                co[0] = ci[4];
                co[1] = ci[5];
                co[2] = ci[6];
                co[3] = ci[7];
                co[4] = ci[0];
                co[5] = ci[1];
                co[6] = ci[2];
                co[7] = ci[3];
                return ret;
#endif
#endif
            }


            /*
             * This has to be defined by the individual type of AMF
             * call.
             *
             * This will process some buffer of data and load it into
             * this AMF object.
             *
             * Note that the buffer is NOT! owned by the AMF object,
             * but WILL be used by it.  The goal of this class is to
             * alloc as little memory as possible and to re-use th
             * buffer where available.
             *
             * A const char* will be thrown with an error message on
             * failure.
             *
             * The third parameter, isMap, will indicate if we're expecting
             * to load a map with key value pairs or just a list of
             * something.
             *
             * The fourth parameter, if non-zero AND isMap is false,
             * will be used as a hard limit of how many records
             * we will process -- basically that's to support AMF's
             * "strict array" type.
             *
             * Returns the number of bytes consumed from the buffer.
             */
            virtual int decode(const char* buf, int size, bool isMap = false,
                               uint32_t arraySize = 0)
            {
                throw "Needs definition";
            }

            /*
             * Properties may be either a map or a vector, depending on
             * if we have names or not.
             */
            union Properties {
                std::map<Value, Property>   propMap;
                std::vector<Property>       propList;
                Properties() { };
                ~Properties() { };
            };
            
            Properties properties;
            bool    isMap;
    };

    class AMF0 : public AMF
    {
        public:
            enum Types : unsigned char
                              { NUMBER = 0, BOOLEAN, STRING, OBJECT, MOVIECLIP,
                                NILL, UNDEFINED, REFERENCE, ECMA_ARRAY,
                                OBJECT_END, STRICT_ARRAY, DATE, LONG_STRING,
                                UNSUPPORTED, RECORDSET, XML_DOC, TYPED_OBJECT,
                                AVMPLUS, INVALID = 0xff };

            /*
             * This will process some buffer of data and load it into
             * this AMF object.
             *
             * Note that the buffer is NOT! owned by the AMF object,
             * but WILL be used by it.  The goal of this class is to
             * alloc as little memory as possible and to re-use th
             * buffer where available.
             *
             * A const char* will be thrown with an error message on
             * failure.
             *
             * The third parameter, isMap, will indicate if we're expecting
             * to load a map with key value pairs or just a list of
             * something.
             *
             * The fourth parameter, if non-zero AND isMap is false,
             * will be used as a hard limit of how many records
             * we will process -- basically that's to support AMF's
             * "strict array" type.
             *
             * Returns the number of bytes consumed from the buffer.
             */
            int decode(const char* buf, int size, bool isMap = false,
                       uint32_t arraySize = 0);

            /*
             * Clean out properties
             */
            ~AMF0();
    };

/*
    class AMF3 : public AMF
    {
        public:
            enum Types : char { UNDEFINED = 0, NILL, FALSE, TRUE, INTEGER, DOUBLE, STRING,
                                XML_DOC, DATE, ARRAY, OBJECT, XML, BYTE_ARRAY };
    };
 */
}


#endif
