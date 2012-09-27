/*
 * Copyright 2011 Jim Knowler
 *           2012 Alexander Rojas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "Debug.hpp"

#include <stdio.h>
#include <stdarg.h>

#include "spotify/Core/Mutex.hpp"

namespace spotify {
namespace debug {
namespace {
core::Mutex g_mutex;
}

void PrintLine(const char *msg, ...) {
    core::ScopedLock autoLock(&g_mutex);

    va_list args;
    va_start(args, msg);

    vprintf(msg, args);

    va_end(args);

    printf("\n");
}

void PrintLine(int indent, const char *msg, ...) {
    core::ScopedLock autoLock(&g_mutex);

    for (int i = 0; i < indent; i++) {
        printf(" ");
    }

    va_list args;
    va_start(args, msg);

    vprintf(msg, args);

    va_end(args);

    printf("\n");

}

} // Debug
} // Spotify
