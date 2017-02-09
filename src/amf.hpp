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
 *
 * TO DO:
 *
 * * ADD OPERATORS TO ACCESS MAP/VECTOR
 * * ADD ENCODER
 * * ADD EASY WAY TO ADD STUFF TO OBJECT.
 * * MAKE TESTS
 * * DO ALL THIS FOR AMF3
 * * PROPER EXCEPTIONS - out_of_buffer, return number of bytes needed
 */

#ifndef __AMF_HPP__
#define __AMF_HPP__

#include <map>
#include <vector>
#include <cstring>
#include <cstdint>
#include <stdexcept>

#include <arpa/inet.h>
#include <sys/param.h>

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

                /*
                 * This operator is for using Value's as map keys.
                 */
                bool operator<(const Value &o) const
                {
                    if(o.len < len) { // shorter string is always smaller
                        return false;
                    }

                    if(o.len != len) { // thus longer is always bigger
                        return true;
                    }

                    // Same size?  Fall back to string compare.
                    return (strncmp(val, o.val, len) < 0);
                }

                /*
                 * This operator is for using Value's as map keys.
                 */
                bool operator==(const Value &o) const
                {
                    if(o.len != len) { // Sanity check.
                        return false;
                    }

                    // Do actual compare.
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
             * PRIMITIVE DECODERS
             *
             * TODO: Improve (or remove) decodeInt24 which is needed for RTMP
             * timestamp processing with its awful, awkward 3 byte integers.
             * decodeInt24 is not used by AMF.  This should maybe just be moved
             * to the RTMP server.
             *
             * TODO: decodeInt32LE - there's *one* integer in RTMP protocol
             * that is randomly little endian which kind of screws everything
             * up.  There's no native C code to massage the byte order to
             * little endian.  This is, again, not used by AMF and should
             * possibly be moved.
             */
            static inline uint32_t decodeInt24(const char* data)
            {
                return (data[0] << 16) | (data[1] << 8) | data[2];
            }

            static inline uint32_t decodeInt32LE(const char* data)
            {
#               if __BYTE_ORDER == __BIG_ENDIAN
                    return (data[3] << 24) | (data[2] << 16) |
                           (data[1] << 8) | data[0];
#               else
                    return (*((uint32_t*)data));
#               endif
            }

            static inline uint16_t decodeInt16(const char* data)
            {
                return ntohs(*((uint16_t*)data));
            }

            static inline uint32_t decodeInt32(const char* data)
            {
                return ntohl(*((uint32_t*)data));
            }

            /*
             * This method is shamelessly ripped off verbatim from
             * librtmp -- I couldn't figure out a way to implement
             * it any better.
             *
             * Doing a simple byte-flip can be done a number of
             * ways ... but librtmp seems to have gone out of its
             * way to figure out every byte order case for a double
             * which is the "best" solution of the many bad solutions
             * I've seen.
             */
            static inline double decodeNumber(const char* data)
            {
#               if __FLOAT_WORD_ORDER == __BYTE_ORDER
#                   if __BYTE_ORDER == __BIG_ENDIAN
                        return (*((double*)data));
#                   elif __BYTE_ORDER == __LITTLE_ENDIAN
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
#                   endif
#               else
#                   if __BYTE_ORDER == __LITTLE_ENDIAN
                        /* __FLOAT_WORD_ORER == __BIG_ENDIAN */
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
#                   else
    /* __BYTE_ORDER == __BIG_ENDIAN && __FLOAT_WORD_ORER == __LITTLE_ENDIAN */
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
#                   endif
#               endif
            }


            /*
             * PRIMITIVE ENCODERS
             *
             * NOTE: These encode methods assume that buffer size
             * checking is done by the caller (as is normally the case)
             */
            static inline void encodeInt16(uint16_t val, char* data)
            {
                (*((uint16_t*)data)) = htons(val);
            }

            static inline void encodeInt32(uint32_t val, const char* data)
            {
                (*((uint32_t*)data)) = htonl(val);
            }

            /*
             * This was also shamelessly ripped from librtmp
             */
            static inline void encodeNumber(double val, char *data)
            {
#               if __FLOAT_WORD_ORDER == __BYTE_ORDER
#                   if __BYTE_ORDER == __BIG_ENDIAN
                        (*((double*)data)) = val;

#                   elif __BYTE_ORDER == __LITTLE_ENDIAN
                        unsigned char *ci, *co;
                        ci = (unsigned char *)&val;
                        co = (unsigned char *)data;
                        co[0] = ci[7];
                        co[1] = ci[6];
                        co[2] = ci[5];
                        co[3] = ci[4];
                        co[4] = ci[3];
                        co[5] = ci[2];
                        co[6] = ci[1];
                        co[7] = ci[0];
#                   endif
#               else
#                   if __BYTE_ORDER == __LITTLE_ENDIAN
                        /* __FLOAT_WORD_ORER == __BIG_ENDIAN */
                        unsigned char *ci, *co;
                        ci = (unsigned char *)&val;
                        co = (unsigned char *)data;
                        co[0] = ci[3];
                        co[1] = ci[2];
                        co[2] = ci[1];
                        co[3] = ci[0];
                        co[4] = ci[7];
                        co[5] = ci[6];
                        co[6] = ci[5];
                        co[7] = ci[4];
#                   else
    /* __BYTE_ORDER == __BIG_ENDIAN && __FLOAT_WORD_ORER == __LITTLE_ENDIAN */
                        unsigned char *ci, *co;
                        ci = (unsigned char *)&val;
                        co = (unsigned char *)data;
                        co[0] = ci[4];
                        co[1] = ci[5];
                        co[2] = ci[6];
                        co[3] = ci[7];
                        co[4] = ci[0];
                        co[5] = ci[1];
                        co[6] = ci[2];
                        co[7] = ci[3];
#                   endif
#               endif
            }

            /*
             * Constructor to initialize 'name'.  'name' is used by
             * AMF TypedObjects.
             */
            AMF(const char* name = NULL, uint32_t nameSize = 0)
            {
                this->name.val = name;
                this->name.len = nameSize;
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
                throw std::runtime_error("Needs definition");
            }

            /*
             * Return size of buffer required to encode this object.
             * How this buffer is alloc'd is up to the caller.  The
             * resulting buffer will not be larger than this.
             *
             * It iterates over all items and child items, so therefore
             * this is a potentially expensive call.
             */
            virtual size_t  encodedSize()
            {
                throw std::runtime_error("Needs definition");
            }

            /*
             * Method to produce a size (in bytes) to encode a given
             * Property.  For complex objects, this will run encodedSize
             * on those objects.  encodedSize, in turn, calls this to
             * size its component properties.
             */
            virtual size_t  propertySize(const Property& prop)
            {
                throw std::runtime_error("Needs definition");
            }

            /*
             * This encodes the object into an AMF data stream suitable
             * for transmission or storage to file system.
             *
             * Requires a buffer that we will write to, with a size
             * parameter to say how much buffer is provided.  It will
             * return how many bytes of that buffer we actually consumed.
             *
             * You can use the "encodedSize" call to figure out the
             * minimum buffer size required to encode an object.
             */
            virtual int encode(char* buf, int size)
            {
                throw std::runtime_error("Needs definition");
            }

            /*
             * This encodes an individual AMF property into the provided
             * buffer.  The buffer must be large enough to handle it.
             *
             * You will usually use encode to encode a whole AMF
             * message, but if you need to encode some small part of
             * an AMF message, you can use this instead.
             *
             * Returns number of bytes consumed.
             */
            virtual int encodeProperty(char* buf, int size,
                                       const Property& prop)
            {
                throw std::runtime_error("Needs definition");
            }

            /*
             * Properties may be either a map or a vector, depending on
             * if we have names or not.
             */
            union Properties {
                std::map<Value, Property>*  propMap;
                std::vector<Property>*      propList;
                Properties() {
                    memset(this, 0, sizeof(Properties));
                }
            };

            Properties  properties;
            bool        isMap;
            Value       name;           // This is for "typed" objects.
    };

/*****************************************************************************
 * AMF0
 *
 * Version 0 of AMF
 *****************************************************************************/

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
             * Return size of buffer required to encode this object.
             * How this buffer is alloc'd is up to the caller.  The
             * resulting buffer will not be larger than this.
             *
             * It iterates over all items and child items, so therefore
             * this is a potentially expensive call.
             */
            size_t  encodedSize();

            /*
             * Method to produce a size (in bytes) to encode a given
             * Property.
             */
            size_t  propertySize(const Property& prop);

            /*
             * This encodes the object into an AMF data stream suitable
             * for transmission or storage to file system.
             *
             * Requires a buffer that we will write to, with a size
             * parameter to say how much buffer is provided.  It will
             * return how many bytes of that buffer we actually consumed.
             *
             * You can use the "encodedSize" call to figure out the
             * minimum buffer size required to encode an object.
             * IMPLEMENTATION NOTE: The top level AMF0 object has no
             * type; it should be treated like a list.  encodeProperty
             * will use this method, but it will add the window dressing
             * (any pre-amble or post-amble bytes).
             *
             * The point is, only call this on a top-level AMF0 object.
             */
            int encode(char* buf, int size);

            /*
             * This encodes an individual AMF property into the provided
             * buffer.  The buffer must be large enough to handle it.
             *
             * You will usually use encode to encode a whole AMF
             * message, but if you need to encode some small part of
             * an AMF message, you can use this instead.
             *
             * Returns number of bytes consumed.
             */
            int encodeProperty(char* buf, int size, const Property& prop);

            /*
             * Clean out properties
             */
            ~AMF0();
    };

/*****************************************************************************
 * AMF3
 *
 * Version 3 of AMF
 *****************************************************************************/

/*
    class AMF3 : public AMF
    {
        public:
            enum Types : char { UNDEFINED = 0, NILL, FALSE, TRUE, INTEGER, DOUBLE, STRING,
                                XML_DOC, DATE, ARRAY, OBJECT, XML, BYTE_ARRAY };
    };
 */

/*****************************************************************************
 * Exceptions
 *
 * These are exceptions that can be thrown by the AMF library.  We will
 * try to throw helpful exceptions that give enough information to diagnose
 * a potential problem.
 *****************************************************************************/


}


#endif
