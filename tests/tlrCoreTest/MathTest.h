// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace CoreTest
    {
        class MathTest : public Test::ITest
        {
        protected:
            MathTest();

        public:
            static std::shared_ptr<MathTest> create();

            void run() override;
        };
    }
}
