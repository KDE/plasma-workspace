/*
    SPDX-FileCopyrightText: 2022 Xaver Hugl <xaver.hugl@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#pragma once
#include <memory>

struct CDeleter {
    template<typename T>
    void operator()(T *ptr)
    {
        if (ptr) {
            free(ptr);
        }
    }
};

template<typename T>
using UniqueCPointer = std::unique_ptr<T, CDeleter>;
