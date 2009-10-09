#include <iostream>
#include "compressor.h"
using namespace std;

int main(int argc, char *argv[])
{
    using namespace glzip;
    string infile_name("big.log");
    string outfile_name;
    Compressor<> compressor(infile_name, outfile_name);
    compressor.compress();

    string infile_name2 = outfile_name;
    string outfile_name2;
    Decompressor<> decompressor(infile_name2, outfile_name2);
    decompressor.decompress();

    return 0;
}
