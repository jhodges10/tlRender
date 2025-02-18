// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace CoreTest
    {
        class AVIOTest : public Test::ITest
        {
        protected:
            AVIOTest();

        public:
            static std::shared_ptr<AVIOTest> create();

            void run() override;

        private:
            void _videoFrame();
            void _ioSystem();
        };
    }
}
