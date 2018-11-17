// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include "azure_c_shared_utility/threadapi.h"

#define LOOP_COUNT      1000000

int main(int argc, char* argv[])
{
    int result;

    (void)printf("Starting the work loop\r\n");
    for (size_t index; index < LOOP_COUNT; index++)
    {
        if (index == LOOP_COUNT/2)
        {
            (void)printf("Half way complete\r\n");
        }
        ThreadAPI_Sleep(500);
    }
    (void)printf("Finished work\r\n");
    return 0;
}
