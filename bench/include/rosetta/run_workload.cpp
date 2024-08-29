#include <set>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include "dst.h"
#ifdef USE_DISK
#include "disk.h"
#endif

#ifndef KEYTYPE
#define KEYTYPE uint64_t
#endif
int cnt_reads = 0;

int main()
{
    return 0;
}