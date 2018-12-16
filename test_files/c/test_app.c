// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tickcounter.h"

#define LOOP_COUNT          4200
#define ALLOC_SIZE          1024*1000 // one K
#define RUN_TIME_SEC        50


int main(int argc, char* argv[])
{
    TICK_COUNTER_HANDLE tickcounter_handle;

    if ((tickcounter_handle = tickcounter_create()) == NULL)
    {
        (void)printf("Failure creating tick counter\r\n");
    }
    else
    {
        //char* test_value = (char*)malloc(ALLOC_SIZE);
        //memset(test_value, '@', sizeof(ALLOC_SIZE));

        tickcounter_ms_t last_poll_time = 0;
        tickcounter_ms_t current_time = 0;
        size_t index = 1;

        (void)printf("test application will running for %d seconds\r\n", RUN_TIME_SEC);
        long temp_value;

        tickcounter_get_current_ms(tickcounter_handle, &last_poll_time);

        while (((current_time - last_poll_time)/1000) < RUN_TIME_SEC)
        {
            tickcounter_get_current_ms(tickcounter_handle, &current_time);

            //for (size_t inner = 0; inner < 50; inner++)
            //{
            //    temp_value = 25654 % 32;
            //}
            //if ((current_time - last_poll_time)/1000 < RUN_TIME_SEC/2)
            //{
            //    (void)printf("Half way complete\r\n");
            //}

            ThreadAPI_Sleep(1500);
            index++;
        }
        //free(test_value);
        (void)printf("Finished test application\r\n");
    }
    return 0;
}
