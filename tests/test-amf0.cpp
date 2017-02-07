/*
 * test-amf0.cpo
 *
 * Put the AMF0 portion of the library through its paces.
 */

#include <iostream>
#include <cstdio>
#include "amf.hpp"


using namespace Tigerdile;

int main(int argc, char** argv, char** envp)
{
    // Load some AMF data
    FILE*   in;
    char*   buf;
    size_t  len;
    AMF0    amf;

/* This would be great if we have some binary data to test
    in = fopen("binary-data", "rb");
    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);

    buf = new char[len];

    fread(buf, len, 1, in);
    fclose(in);
 */

    try {
        std::cout << amf.decode(buf, len) << std::endl;
    } catch(const char* e) {
        std::cout << e << std::endl;
    }
}
