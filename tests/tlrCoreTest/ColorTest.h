// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace CoreTest
    {
        class ColorTest : public Test::ITest
        {
        protected:
            ColorTest();

        public:
            static std::shared_ptr<ColorTest> create();

            void run() override;
        };
    }
}
