/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef KTF_KTF_H
#define KTF_KTF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <limits.h>

#include <compiler.h>

typedef uint16_t io_port_t;

typedef int (*user_func_t)(void *arg);

extern bool opt_debug;

extern int usermode_call(user_func_t fn, void *fn_arg);

extern void kernel_main(void);
extern void test_main(void);

#endif /* KTF_KTF_H */
