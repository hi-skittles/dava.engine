/*
Copyright 2011 Erik Gorset. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

      THIS SOFTWARE IS PROVIDED BY Erik Gorset ``AS IS'' AND ANY EXPRESS OR IMPLIED
      WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
      FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Erik Gorset OR
      CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
      CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
      SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
      ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
      NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
      ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

      The views and conclusions contained in the software and documentation are those of the
      authors and should not be interpreted as representing official policies, either expressed
      or implied, of Erik Gorset.
*/

#include <iostream>
#include <algorithm>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "Base/Radix/Radix.h"

namespace DAVA
{
void insertion_sort(intptr_t* array, int offset, int end)
{
    intptr_t x, y, temp;
    for (x = offset; x < end; ++x)
    {
        for (y = x; y > offset && array[y - 1] > array[y]; y--)
        {
            temp = array[y];
            array[y] = array[y - 1];
            array[y - 1] = temp;
        }
    }
}

void RadixSortImpl(intptr_t* array, int offset, int end, int shift)
{
    intptr_t x, y, value, temp;
    intptr_t last[256] = { 0 }, pointer[256];

    for (x = offset; x < end; ++x)
    {
        ++last[(array[x] >> shift) & 0xFF];
    }

    last[0] += offset;
    pointer[0] = offset;
    for (x = 1; x < 256; ++x)
    {
        pointer[x] = last[x - 1];
        last[x] += last[x - 1];
    }

    for (x = 0; x < 256; ++x)
    {
        while (pointer[x] != last[x])
        {
            value = array[pointer[x]];
            y = (value >> shift) & 0xFF;
            while (x != y)
            {
                temp = array[pointer[y]];
                array[pointer[y]++] = value;
                value = temp;
                y = (value >> shift) & 0xFF;
            }
            array[pointer[x]++] = value;
        }
    }

    if (shift > 0)
    {
        shift -= 8;
        for (x = 0; x < 256; ++x)
        {
            temp = x > 0 ? pointer[x] - pointer[x - 1] : pointer[0] - offset;
            if (temp > 64)
            {
                RadixSortImpl(array, static_cast<int32>(pointer[x] - temp), static_cast<int32>(pointer[x]), shift);
            }
            else if (temp > 1)
            {
                // std::sort(array + (pointer[x] - temp), array + pointer[x]);
                insertion_sort(array, static_cast<int32>(pointer[x] - temp), static_cast<int32>(pointer[x]));
            }
        }
    }
}
}
