// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace CoreTest
    {
        class CacheTest : public Test::ITest
        {
        protected:
            CacheTest();

        public:
            static std::shared_ptr<CacheTest> create();

            void run() override;
        };
    }
}
