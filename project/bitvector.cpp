#include <cstdio>
#include <iostream>
#include "bitvector.h"

BitVector::BitVector(int sz)
{
    int i;

    size = sz;
    ui_size = sizeof(unsigned int) * 8;

    wordWidth = (size + ui_size - 1) / ui_size;
    bv = new int[wordWidth];
    //set all bits to zero

    for (i = 0; i < wordWidth; i++) bv[i] = 0;
}

BitVector::~BitVector()
{
    delete[] bv;
}

void BitVector::setBit(int pos)
{
    //first figure out the index to the integer array
    int index = pos / ui_size;
    //next which bit in the integer
    int offset = pos % ui_size;

    bv[index] |= (ON << offset);
}

void BitVector::resetBit(int pos)
{
    //first figure out the index to the integer array
    int index = pos / ui_size;
    //next which bit in the integer
    int offset = pos % ui_size;

    bv[index] &= (~(ON << offset));
}

int BitVector::testBit(int pos)
{
    //first figure out the index to the integer array
    int index = pos / ui_size;
    //next which bit in the integer
    int offset = pos % ui_size;

    if (bv[index] & (ON << offset))
        return ON;  //returns 1;
    else
        return OFF;  //returns zero bit
}

void BitVector::setBitVector(unsigned int* nbv)
{
    for (int i = 0; i < wordWidth; i++)
        //*(bv + i) = *(nbv + i);
        bv[i] = nbv[i];
}

void BitVector::getBitVector(unsigned int* nbv)
{
    for (int i = 0; i < wordWidth; i++)
        nbv[i] = bv[i];
}
