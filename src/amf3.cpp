/*
 * amf3.cpp
 *
 * Source code for AMF library
 *
 * @author sconley
 * Copyright 2017
 *********************************************************************
 *
 * This was very inspired by the librtmp code which is here:
 *
 * https://rtmpdump.mplayerhq.hu/
 *
 */

#include "amf.hpp"

using namespace Tigerdile;

/*****************************************************************************
 * AMF Version 3 Definitions
 ****************************************************************************/

/*
 * This will process some buffer of data and load it into
 * this AMF object.
 *
 * Note that the buffer is NOT! owned by the AMF object,
 * but WILL be used by it.  The goal of this class is to
 * alloc as little memory as possible and to re-use th
 * buffer where available.
 *
 * Decode will throw an underflow_error if there is not
 * enough data to decode, or a runtime_error if there
 * is a problem.
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
uint32_t AMF3::decode(const char* buf, uint32_t size)
{
    return 0;
}

/*
 * Return size of buffer required to encode this object.
 * How this buffer is alloc'd is up to the caller.  The
 * resulting buffer will not be larger than this.
 *
 * It iterates over all items and child items, so therefore
 * this is a potentially expensive call.
 */
uint32_t  AMF3::encodedSize()
{
    return 0;
}

/*
 * Method to produce a size (in bytes) to encode a given
 * Property.
 */
uint32_t  AMF3::propertySize(const Property& prop)
{
    return 0;
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
 * IMPLEMENTATION NOTE: The top level AMF0 object has no
 * type; it should be treated like a list.  encodeProperty
 * will use this method, but it will add the window dressing
 * (any pre-amble or post-amble bytes).
 *
 * The point is, only call this on a top-level AMF0 object.
 */
uint32_t AMF3::encode(char* buf, uint32_t size)
{
    return 0;
}

/*
 * AMF3 destructor to clean out properties that use objects.
 */
AMF3::~AMF3()
{
    if(this->isMap && this->properties.propMap) {
        // Iterate over map, delete what's an object type
        for(auto& kv: *this->properties.propMap) {
            switch((Types)kv.second.type) {
                case Types::OBJECT:
                case Types::ARRAY:
                case Types::VECTOR_INT:
                case Types::VECTOR_UINT:
                case Types::VECTOR_DOUBLE:
                case Types::VECTOR_OBJECT:
                case Types::DICTIONARY:
                    delete kv.second.property.object;
                default: // avoids warning
                    break;
            }
        }

        delete this->properties.propMap;
    } else if(this->properties.propList) {
        for(Property& prop : *this->properties.propList) {
            switch((Types)prop.type) {
                case Types::OBJECT:
                case Types::ARRAY:
                case Types::VECTOR_INT:
                case Types::VECTOR_UINT:
                case Types::VECTOR_DOUBLE:
                case Types::VECTOR_OBJECT:
                case Types::DICTIONARY:
                    delete prop.property.object;
                default: // avoids warning
                    break;
            }
        }

        delete this->properties.propList;
    }
}
