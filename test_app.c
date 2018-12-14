// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include "azure_c_shared_utility/threadapi.h"

#define LOOP_COUNT      3200
#define ALLOC_SIZE      1024 // one K

int main(int argc, char* argv[])
{
    long temp_value;
    (void)printf("test application running\r\n");
    for (size_t index = 0; index < LOOP_COUNT; index++)
    {
        char* test_value = (char*)malloc(ALLOC_SIZE*index+1);
        
        if (index == LOOP_COUNT/2)
        {
            (void)printf("Test application half way\r\n");
        }

        for (size_t inner = 0; inner < 50; inner++)
        {
            temp_value = 25654 % 32;
        }
        free(test_value);
        for (size_t inner = 0; inner < 50; inner++)
        {
            temp_value = 452132 % 22;
        }
        ThreadAPI_Sleep(1);
    }
    (void)printf("Finished test application\r\n");
    return 0;
}
