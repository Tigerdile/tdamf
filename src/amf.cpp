/*
 * amf.cpp
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
 * AMF Version 0 Definitions
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
int AMF0::decode(const char* buf, int size, bool isMap, uint32_t arraySize)
{
    Value name;
    Property prop;
    int originalSize = size;
    int objectCount = 0;

    if((this->isMap = isMap) && (!this->properties.propMap)) {
        this->properties.propMap = new std::map<Value, Property>();
    } else if(!this->properties.propList) {
        this->properties.propList = new std::vector<Property>();
    }

    while(size > 0 && ((arraySize == 0) || objectCount < arraySize)) {
        // We're looking for hex 0x00 0x00 0x09 
        if((size >= 3) &&
           (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x09)) {
            // We're done here
            size -= 3;
            buf += 3;
            break;
        }

        // try to decode the property
        // If we're loading as a map (object) then there will
        // be a string key first.
        if(isMap) {
            if(size < 4) {
                // error
                throw "isMap is true and size less than 4 bytes";
            }


            name.len = this->decodeInt16(buf);
            if(name.len > size - 2) {
                throw "Got out-of-bounds name.len";
            }

            size -= 2;
            buf += 2;

            name.val = buf;
            size -= name.len;
            buf += name.len;
        }

        // Type will be the first byte.
        prop.type = buf[0];
        buf++;
        size--;

        // What the type is determines how we proces it.
        // This may be better implemented in a number of ways, such as
        // some kind of callback map
        switch((Types)prop.type) {
            case Types::NUMBER: // a "double" - 8 bttes, IEEE-754
                if(size < 8) {
                    throw "Could not decode number - less than 8 bytes";
                }

                prop.property.number = this->decodeNumber(buf);
                buf += 8;
                size -= 8;
                break;
            case Types::BOOLEAN: // Single byte, true/false
                if(size < 1) {
                    throw "Could not decode boolean - less than 1 byte";
                }

                prop.property.number = (*buf != 0);
                buf++;
                size--;
                break;
            case Types::STRING: // "small" string, 2 bytes length then string.
                if(size < 3) {
                    throw "String requires at least 3 bytes in buffer.";
                }

                prop.property.value.len = this->decodeInt16(buf);

                buf += 2;
                size -= 2;

                if(size < prop.property.value.len) {
                    throw "Couldn't decode a string with not enough buffer";
                }

                prop.property.value.val = buf;
                buf += prop.property.value.len;
                size -= prop.property.value.len;
                break;
            case Types::ECMA_ARRAY:
                // This is an associative array.  It has a 4-byte count
                // then otherwise is treated like an object.
                if(size < 4) {
                    throw "ECMA_ARRAY with not enough bytes";
                }

                // librtmp's implementation throws out the size.
                // We could strictly enforce the size and error
                // if it doesn't match expectations, but I'm not
                // sure why we'd really care that much.
                // @TODO ?
                size -= 4;
                buf += 4;
            case Types::OBJECT: // This will be a "map" basically.
                {
                    int res;

                    prop.property.object = new AMF0();
                    res = prop.property.object->decode(buf, size, true);

                    buf += res;
                    size -= res;
                }

                break;
            case Types::TYPED_OBJECT:
                /* From the AMF0 spec
                 *
                 * If a strongly typed object has an alias registered for
                 * its class then the type name will also be serialized.
                 * Typed objects are considered complex types and reoccurring
                 * instances can be sent by reference.
                 *
                 * This is an object with a STRING (16 bit) prefix.
                 * Read string which is the type name, and the rest is the
                 * object.
                 */
                {
                    int res;

                    if(size < 2) {
                        // Minimum
                        throw "TYPED_OBJECT without enough buffer for type str";
                    }

                    res = this->decodeInt16(buf);

                    buf += 2;
                    size -= 2;

                    if(size < res) {
                        throw "TYPED_OBJECT without enough buffer to load name";
                    }

                    prop.property.object = new AMF(buf, res);
                    res = prop.property.object->decode(buf, size, true);

                    buf += res;
                    size -= res;
                }

                break;
            case Types::REFERENCE:
                /* From the AMF0 spec
                 *
                 * AMF0 defines a complex object as an anonymous object, a
                 * typed object, an array or an ecma-array. If the exact
                 * same instance of a complex object appears more than once
                 * in an object graph then it must be sent by reference. The
                 * reference type uses an unsigned 16- bit integer to point 
                 * to an index in a table of previously serialized objects.
                 * Indices start at 0.
                 *
                 * This seems like an unpopular thing to implement so I
                 * am leaving this @TODO
                 *
                 * The specs aren't clear as to how a reference is initially
                 * defined .. "index of previously serialized objects" ?
                 * Is this something I'm supposed to maintain automatically?
                 * If so, that's pretty easy.  I'd like to see an example
                 * of this ... if there's ever any need for this feature,
                 * I will dig into it more.
                 *
                 * UPDATE: Yeah, it looks like I should be automatically
                 * keeping a reference array of all things that are considered
                 * complex objects.
                 *
                 * This is a 16-bit (2 byte) unsigned integer for a max of
                 * 65,535 complex types.
                 *
                 * MOAR info, from AMF3 spec
                 * Similar to AMF 0, AMF 3 object reference tables, object trait
                 * reference tables and string reference tables must be reset
                 * each time a new context header or message is processed.
                 */
            case Types::MOVIECLIP:
            case Types::RECORDSET:
                // These are not supported -- Movieclip is a reserved type
                // in AMF0 with no implementation, as is RECORDSET.
                throw "Reserved/Unsupported type!";
            case Types::UNDEFINED:
            case Types::UNSUPPORTED:
                // These two will be treated like null
                prop.type = Types::NILL;
            case Types::NILL:
                // Nothing to do, NULL type speaks for itself.
                break;
            case Types::STRICT_ARRAY:
                // This is a numeric array.
                if(size < 4) {
                    throw "STRICT_ARRAY type with not enough bytes";
                }
                {
                    int res;

                    // use arrayCount instead of arraySize to support
                    // nested arrays.  Be careful!
                    uint32_t arrayCount = this->decodeInt32(buf);
                    buf += 4;
                    size -= 4;

                    prop.property.object = new AMF0();
                    res = prop.property.object->decode(buf, size, false,
                                                      arrayCount);

                    buf += res;
                    size -= res;
                }

                break;
            case Types::DATE:
                /*
                 * Per the AMF0 specs
                 *
                 * An ActionScript Date is serialized as the number of
                 * milliseconds elapsed since the epoch of midnight on
                 * 1st Jan 1970 in the UTC time zone. While the design
                 * of this type reserves room for time zone offset
                 * information, it should not be filled in, nor used, as
                 * it is unconventional to change time zones when
                 * serializing dates on a network. It is suggested that
                 * the time zone be queried independently as needed.
                 *
                 * I'll bet whomever added that time zone feature really
                 * didn't want to add it.  Or there was some kind of
                 * argument.  This reeks of a developer being made to
                 * do something they don't want to do!
                 *
                 * librtmp actually supports this, but I'm going to
                 * throw the timezone out.
                 *
                 * @TODO : log it?  Care?  I dunno
                 */
                if(size < 10) {
                    throw "Got DATE type but not enough bytes";
                }

                prop.property.number = this->decodeNumber(buf);

                // TZ is a 2-byte signed short int, we are callously
                // skipping it.
                buf += 10;
                size -= 10;
                break;
            case Types::LONG_STRING:
            case Types::XML_DOC:
                // These are identical types according to spec.
                // This must be a least 4 bytes
                if(size < 4) {
                    throw "Not enough bytes to process LONG_STRING/XML_DOC";
                }

                prop.property.value.len = this->decodeInt32(buf);
                size -= 4;
                buf += 4;

                // do we have enough?
                if(size < prop.property.value.len) {
                    throw "Not enough bytes to load LONG_STRING/XML_DOC";
                }

                prop.property.value.val = buf;

                buf += prop.property.value.len;
                size -= prop.property.value.len;
                break;
            case Types::AVMPLUS:
                // Swap to AMF3 decoder.
                {
                    int res = 0;

                    // This won't compile yet.
                    //prop.property.object = new AMF3();
                    //res = prop.property.object->decode(buf, size, true);

                    buf += res;
                    size -= res;
                }
                break;
            default:
                throw "Unknown type received";
        }

        // Add it to our map or vector
        // Use emplace_back ?  Could save some CPU
        if(isMap) {
            this->properties.propMap->insert(
                std::pair<Value, Property>(name, prop)
            );
        } else {
            this->properties.propList->push_back(prop);
        }

        objectCount++;
    }

    return originalSize - size;
}


/*
 * Method to produce a size (in bytes) to encode a given
 * Property.  Note 'type' field always adds 1 byte
 */
size_t  AMF0::propertySize(const Property& prop)
{
    switch(prop.type) {
        case Types::OBJECT_END:
            return 3;
        case Types::NUMBER:
            return 9;
        case Types::BOOLEAN:
            return 2;
        case Types::STRING:
            return 3+prop.property.value.len;
        case Types::ECMA_ARRAY:
            return 5+prop.property.object->encodedSize();
        case Types::OBJECT:
            return 1+prop.property.object->encodedSize();
        case Types::REFERENCE:
            return 3;
        case Types::TYPED_OBJECT:
            return 3+prop.property.object->name.len
                    +prop.property.object->encodedSize();
        case Types::MOVIECLIP:
        case Types::RECORDSET:
            throw "Reserved / unsupported type!";
        case Types::UNDEFINED:
        case Types::UNSUPPORTED:
        case Types::NILL:
            return 1;
        case Types::STRICT_ARRAY:
            return 5+prop.property.object->encodedSize();
        case Types::DATE:
            return 11;
        case Types::LONG_STRING:
        case Types::XML_DOC:
            return 5+prop.property.value.len;
        case Types::AVMPLUS:
            return 1+prop.property.object->encodedSize();
        default:
            throw "Unknown type received";
    }
}


/*
 * Return size of buffer required to encode this object.
 * How this buffer is alloc'd is up to the caller.  The
 * resulting buffer will not be larger than this.
 *
 * It iterates over all items and child items, so therefore
 * this is a potentially expensive call
 */
size_t AMF0::encodedSize()
{
    size_t result = 0;

    if(this->isMap) {
        for(const auto& kv: *this->properties.propMap) {
            result += this->propertySize(kv.second);

            // add in our key size, small string
            result += kv.first.len + 2;
        }

        // This will have the end bytes
        result += 3;
    } else {
        for(const Property& prop: *this->properties.propList) {
            result += this->propertySize(prop);
        }
    }

    return result;
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
 *
 * IMPLEMENTATION NOTE: The top level AMF0 object has no
 * type; it should be treated like a list.  encodeProperty
 * will use this method, but it will add the window dressing
 * (any pre-amble or post-amble bytes).
 *
 * The point is, only call this on a top-level AMF0 object.
 */
int AMF0::encode(char* buf, int size)
{
    int consumed;
    int originalSize = size;

    // How we iterate depends on isMap
    if(this->isMap) {
        for(auto& kv: *this->properties.propMap) {
            consumed = this->encodeProperty(buf, size, kv.second);
            size -= consumed;
            buf += consumed;
        }
    } else {
        for(const Property& prop: *this->properties.propList) {
            consumed = this->encodeProperty(buf, size, prop);
            size -= consumed;
            buf += consumed;
        }
    }

    return originalSize - size;
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
int AMF0::encodeProperty(char* buf, int size, const Property& prop)
{
    int consumed = 0; // used in a few places.

    switch(prop.type) {
        case Types::OBJECT_END:
            // We probably don't have a property of this type,
            // but we can encode it if we do!
            if(size < 3) {
                throw "Not enough buffer room to write OBJECT_END.";
            }

            buf[0] = 0x00;
            buf[1] = 0x00;
            buf[2] = 0x09;

            return 3;
        case Types::NUMBER:
            // Type byte + 8 byte double
            if(size < 9) {
                throw "Not enough buffer to write NUMBER";
            }

            buf[0] = prop.type;
            this->encodeNumber(prop.property.number, &buf[1]);

            return 9;
        case Types::BOOLEAN:
            // Type byte + 1 byte boolean
            if(size < 2) {
                throw "Not enough buffer room to write BOOLEAN";
            }

            buf[0] = prop.type;
            buf[1] = (char)(prop.property.number != 0);

            return 2;
        case Types::STRING:
            // Small string - type byte + 2 byte len + string
            if(size < 3+prop.property.value.len) {
                throw "Not enough buffer to write STRING";
            }

            buf[0] = prop.type;
            this->encodeInt16(prop.property.value.len, &buf[1]);
            memcpy(&buf[3], prop.property.value.val, prop.property.value.len);

            return 3+prop.property.value.len;
        case Types::ECMA_ARRAY:
            // This is identical to object, except it has a 4 byte header
            if(size < 5) {
                throw "Not enough buffer to write ECMA_ARRAY";
            }

            buf[0] = prop.type;
            this->encodeInt32(prop.property.object->properties.propList->size(),
                              &buf[1]);

            consumed = 5;
        case Types::TYPED_OBJECT:
        case Types::OBJECT:
            // We may have fallen through from ECMA_Array.  if consumed > 0
            if(!consumed) {
                if(size < 1) {
                    // need 1 byte + length of name
                    throw "Not enough buffer to write OBJECT/TYPED_OBJECT";
                }

                buf[0] = prop.type;
                consumed++;

                // write name if typed object, if available.
                // for my purposes this is a minority case and therefore
                // this somewhat unoptimized process is okay :P
                if(prop.property.object->name.len) {
                    // check size
                    if((size - 1) < (2+prop.property.object->name.len)) {
                        throw "Not enough buffer to write TYPED_OBJECT";
                    }

                    // write string
                    this->encodeInt16(prop.property.object->name.len, &buf[1]);
                    memcpy(&buf[3], prop.property.object->name.val,
                           prop.property.object->name.len);

                    consumed += 2 + prop.property.object->name.len;
                }
            }

            consumed += prop.property.object->encode(&buf[consumed],
                                                     size-consumed);

            // Now add OBJECT_END
            if(size - consumed < 3) {
                throw "Not enough buffer to write terminating OBJECT_END";
            }

            buf[consumed] = 0x00;
            buf[consumed+1] = 0x00;
            buf[consumed+2] = 0x09;

            return consumed + 3;
        case Types::REFERENCE:
            // NOT implemented yet.
            throw "REFERENCE not implemented yet";
            // return 3;
        case Types::MOVIECLIP:
        case Types::RECORDSET:
            throw "Reserved / unsupported type!";
        case Types::UNDEFINED:
        case Types::UNSUPPORTED:
        case Types::NILL:
            // These are just a type with no data
            if(size < 1) {
                throw "Not enough buffer to write NULL type";
            }

            buf[0] = prop.type;
            return 1;
        case Types::STRICT_ARRAY:
            // 4 byte size followed by elements
            if(size < 5) {
                throw "Not enough buffer to write STRICT_ARRAY";
            }

            buf[0] = prop.type;
            this->encodeInt32(prop.property.object->properties.propList->size(),
                              &buf[1]);

            return 5+prop.property.object->encode(&buf[5], size-5);
        case Types::DATE:
            // type byte + 8 byte NUMBER + 2 bytes all 0's
            // NOTE: if we decided to implement TZ's, we need to implement
            // it here too.
            if(size < 11) {
                throw "Not enough buffer to write DATE";
            }

            buf[0] = prop.type;
            this->encodeNumber(prop.property.number, &buf[1]);
            buf[9] = 0x00;
            buf[10] = 0x00;

            return 11;
        case Types::LONG_STRING:
        case Types::XML_DOC:
            // These are identical except for type byte
            // type bite + 4 byte len + string
            if(size < 5+prop.property.value.len) {
                throw "Not enough buffer to write LONG_STRING/XML_DOC";
            }

            buf[0] = prop.type;
            this->encodeInt32(prop.property.value.len, &buf[1]);
            memcpy(&buf[5], prop.property.value.val, prop.property.value.len);

            return 5+prop.property.value.len;
        case Types::AVMPLUS:
            // Type byte followed by AMF03 encoding
            if(size < 1) {
                throw "Not enough buffer to write AVMPLUS";
            }

            buf[0] = prop.type;
            return 1+prop.property.object->encode(&buf[1], size-1);
        default:
            throw "Unknown type received";
    }
}


/*
 * AMF0 destructor to clean out properties that use objects.
 */
AMF0::~AMF0()
{
    if(this->isMap && this->properties.propMap) {
        // Iterate over map, delete what's an object type
        for(auto& kv: *this->properties.propMap) {
            switch((Types)kv.second.type) {
                case Types::OBJECT:
                case Types::ECMA_ARRAY:
                case Types::STRICT_ARRAY:
                case Types::AVMPLUS:
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
                case Types::ECMA_ARRAY:
                case Types::STRICT_ARRAY:
                case Types::AVMPLUS:
                    delete prop.property.object;
                default: // avoids warning
                    break;
            }
        }

        delete this->properties.propList;
    }
}
