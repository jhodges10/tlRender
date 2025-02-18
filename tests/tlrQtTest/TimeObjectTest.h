// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace QtTest
    {
        class TimeObjectTest : public Test::ITest
        {
        protected:
            TimeObjectTest();

        public:
            static std::shared_ptr<TimeObjectTest> create();

            void run() override;
        };
    }
}
