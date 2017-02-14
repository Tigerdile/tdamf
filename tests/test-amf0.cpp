/*
 * test-amf0.cpo
 *
 * Put the AMF0 portion of the library through its paces.
 *
 * TODO: Find or produce some binary data to decode with this.
 * TODO: Make this actually nice instead of a lazy piece of
 *       crap.  Sorry guys :)
 */

#include <iostream>
#include <cstdio>
#include "amf.hpp"


using namespace Tigerdile;

int main(int argc, char** argv, char** envp)
{
    /*
     * Current methodology: set up an AMF0 structure, encode it, then decode it.
     */

    AMF0            sourceAMF;
    AMF::Property   tmp;

    // Let's put a little bit of everything in here.
    sourceAMF.isMap = false;
    sourceAMF.properties.propList = new std::vector<AMF::Property>();

    // Number
    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 1337;
    sourceAMF.properties.propList->push_back(tmp);

    // Boolean
    tmp.type = AMF0::Types::BOOLEAN;
    tmp.property.number = 0;
    sourceAMF.properties.propList->push_back(tmp);

    // String
    tmp.type = AMF0::Types::STRING;
    tmp.property.value.len = 4;
    tmp.property.value.val = "test";
    sourceAMF.properties.propList->push_back(tmp);

    // Object reference - note this object will belong to AMF
    // and be deleted by it
    AMF0* childAMF = new AMF0();
    AMF0* grandchildAMF = new AMF0();

    childAMF->isMap = true;
    childAMF->properties.propMap = new std::map<AMF::Value, AMF::Property>();
    grandchildAMF->isMap = true;
    grandchildAMF->properties.propMap = 
                                new std::map<AMF::Value, AMF::Property>();
    

    // Push a few things into child AMF
    AMF::Value key;

    key.len = 4;
    key.val = "key1";

    tmp.type = AMF0::Types::STRING;
    tmp.property.value.len = 4;
    tmp.property.value.val = "moar";
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));

    key.len = 4;
    key.val = "key2";

    tmp.type = AMF0::Types::STRING;
    tmp.property.value.len = 4;
    tmp.property.value.val = "data";
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));


    // Push a few things into grandchild, then push it onto the child.
    key.len = 5;
    key.val = "child";

    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 33;
    grandchildAMF->properties.propMap->insert(
                            std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                            ));

    // Push grandchild
    key.len = 10;
    key.val = "grandchild";
    tmp.type = AMF0::Types::OBJECT;
    tmp.property.object = grandchildAMF;
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));


    // One last thing into child, then push it.
    key.len = 6;
    key.val = "number";
    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 90210;
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));

    tmp.type = AMF0::Types::OBJECT;
    tmp.property.object = childAMF;
    sourceAMF.properties.propList->push_back(tmp);

    // Okay, that's a butt-ton of objects.  Let's add a null
    tmp.type = AMF0::Types::NILL;
    sourceAMF.properties.propList->push_back(tmp);

    // ECMA_ARRAY, which is basically an object but is technically encoded
    // a little differently.
    childAMF = new AMF0();
    childAMF->isMap = 1;
    childAMF->properties.propMap = new std::map<AMF::Value, AMF::Property>();

    // we'll put a couple numbers in    
    key.len = 7;
    key.val = "number1";
    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 90211;
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));
    key.len = 7;
    key.val = "number2";
    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 90212;
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));

    tmp.type = AMF0::Types::ECMA_ARRAY;
    tmp.property.object = childAMF;
    sourceAMF.properties.propList->push_back(tmp);

    // STRICT_ARRAY, which is a list type.
    childAMF = new AMF0();
    childAMF->isMap = 0;
    childAMF->properties.propList = new std::vector<AMF::Property>();

    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 27604;
    childAMF->properties.propList->push_back(tmp);

    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 27540;
    childAMF->properties.propList->push_back(tmp);

    tmp.type = AMF0::Types::STRICT_ARRAY;
    tmp.property.object = childAMF;
    sourceAMF.properties.propList->push_back(tmp);

    // DATE, which is pretty easy
    tmp.type = AMF0::Types::DATE;
    tmp.property.number = 13371337;
    sourceAMF.properties.propList->push_back(tmp);

    // LONG_STRING, which is basically STRING
    tmp.type = AMF0::Types::LONG_STRING;
    tmp.property.value.len = 4;
    tmp.property.value.val = "long";
    sourceAMF.properties.propList->push_back(tmp);

    // XML_DOC, which is the same as LONG_STRING
    tmp.type = AMF0::Types::XML_DOC;
    tmp.property.value.len = 3;
    tmp.property.value.val = "xml";
    sourceAMF.properties.propList->push_back(tmp);

    // Typed object, which we'll try here.
    childAMF = new AMF0("named", 5);
    childAMF->isMap = 1;
    childAMF->properties.propMap = new std::map<AMF::Value, AMF::Property>();

    key.len = 7;
    key.val = "number3";
    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 90213;
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));
    key.len = 7;
    key.val = "number4";
    tmp.type = AMF0::Types::NUMBER;
    tmp.property.number = 90214;
    childAMF->properties.propMap->insert(std::pair<AMF::Value, AMF::Property>(
                                            key, tmp
                                        ));

    tmp.type = AMF0::Types::TYPED_OBJECT;
    tmp.property.object = childAMF;
    sourceAMF.properties.propList->push_back(tmp);
    

    // AVMPLUS -- we'll support this later.
    // TODO


    // Run size check
    size_t totalSize = sourceAMF.encodedSize();

    std::cout << "Total encoded size for full AMF: " << totalSize << std::endl;

    // alloc
    char*   buf = (char*)malloc(totalSize);

    // encode!
    size_t  encodedSize = sourceAMF.encode(buf, totalSize);
    std::cout << "Bytes left after encode: " << encodedSize << std::endl;

    // First check
    if(encodedSize != totalSize) {
        std::cout << "Estimate size and encoded size not the same!" << std::endl;
        return (int) -1;
    }

    // decode!
    AMF0 targetAMF;
    int consumed;

    try {
        consumed = targetAMF.decode(buf, totalSize);
    } catch(const char* e) {
        std::cout << "Got exception: " << e << std::endl;
        return (int) -1;
    }

    std::cout << "Bytes consumed from buffer after decode: " << consumed
              << std::endl;

    // Second check!
    if(totalSize != consumed) {
        std::cout << "Didn't consume the right amount of bytes!" << std::endl;
        return (int) -1;
    }

    // Compare!

    // FIRST NUMBER: 1337
    if(targetAMF.properties.propList->at(0).type != AMF0::Types::NUMBER) {
        std::cout << "propList[0] not NUMBER, is type "
                  << (int)targetAMF.properties.propList->at(0).type
                  << std::endl;
        return (int) -1;
    }

    if(targetAMF.properties.propList->at(0).property.number != 1337) {
        std::cout << "propList[0] not 1337, is "
                  << (int)targetAMF.properties.propList->at(0).property.number
                  << std::endl;
        return (int) -1;
    }

    // NEXT: Boolean false
    if(targetAMF.properties.propList->at(1).type != AMF0::Types::BOOLEAN) {
        std::cout << "propList[1] not BOOLEAN, is type "
                  << (int)targetAMF.properties.propList->at(1).type
                  << std::endl;
        return (int) -1;
    }

    if(targetAMF.properties.propList->at(1).property.number != 0) {
        std::cout << "propList[1] not 0, is "
                  << (int)targetAMF.properties.propList->at(0).property.number
                  << std::endl;
        return (int) -1;
    }

    // NEXT: String "test"
    if(targetAMF.properties.propList->at(2).type != AMF0::Types::STRING) {
        std::cout << "propList[2] not STRING, is type "
                  << (int)targetAMF.properties.propList->at(2).type
                  << std::endl;
        return (int) -1;
    }

    if((targetAMF.properties.propList->at(2).property.value.len != 4) ||
       (strncmp(targetAMF.properties.propList->at(2).property.value.val,
                 "test", 4))) {
        std::cout << "propList[2] not test, is "
                  << strndup(
                    targetAMF.properties.propList->at(2).property.value.val,
                    targetAMF.properties.propList->at(2).property.value.len)
                  << std::endl;
        return (int) -1;
    }

    // NEXT: Check pushed object.
    if(targetAMF.properties.propList->at(3).type != AMF0::Types::OBJECT) {
        std::cout << "propList[3] not OBJECT, is type "
                  << (int)targetAMF.properties.propList->at(3).type
                  << std::endl;
        return (int) -1;
    }

    // Do the rest :)


    return (int) 0;
}
