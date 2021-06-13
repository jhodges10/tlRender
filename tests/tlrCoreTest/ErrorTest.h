// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace CoreTest
    {
        class ErrorTest : public Test::ITest
        {
        protected:
            ErrorTest();

        public:
            static std::shared_ptr<ErrorTest> create();

            void run() override;
        };
    }
}