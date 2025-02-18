// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace CoreTest
    {
        class ListObserverTest : public Test::ITest
        {
        protected:
            ListObserverTest();

        public:
            static std::shared_ptr<ListObserverTest> create();

            void run() override;
        };
    }
}
