#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

add_executable(test_app test_app.c)
target_link_libraries(test_app aziotsharedutil)
