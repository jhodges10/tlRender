// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#include <string>
#include <vector>

namespace tlr
{
    namespace string
    {
        //! Format
        //!
        //! Example:
        //! std::string result = Format("Testing {2} {1} {0}").arg("one").arg("two").arg("three");
        //!
        //! Results in the string "Testing three two one".
        class Format
        {
        public:
            Format(const std::string&);

            //! \name Arguments
            //! Replace the next argument in the string with the given value.
            //! Arguments consist of an integer enclosed by curly brackets (eg., "{0}").
            //! The argument with the smallest integer will be replaced. The
            //! object is returned so that you can chain calls together.
            ///@{

            Format& arg(const std::string&);
            Format& arg(float, int precision = 0);
            Format& arg(double, int precision = 0);
            template<typename T>
            Format& arg(T);

            ///@}

            //! \name Errors
            ///@{

            bool hasError() const;
            const std::string& getError() const;

            ///@}

            operator std::string() const;

        private:
            std::string _text;
            std::string _error;
        };
    }
}

#include <tlrCore/StringFormatInline.h>
