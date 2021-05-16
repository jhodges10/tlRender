// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#include <tlrCore/String.h>

#include <algorithm>

namespace tlr
{
    namespace string
    {
        std::vector<std::string> split(const std::string& s, char delimeter, bool keepEmpty)
        {
            std::vector<std::string> out;
            bool word = false;
            size_t wordStart = 0;
            size_t i = 0;
            for (; i < s.size(); ++i)
            {
                if (s[i] != delimeter)
                {
                    if (!word)
                    {
                        word = true;
                        wordStart = i;
                    }
                }
                else
                {
                    if (word)
                    {
                        word = false;
                        out.push_back(s.substr(wordStart, i - wordStart));
                    }
                    if (keepEmpty && i > 0 && s[i - 1] == delimeter)
                    {
                        out.push_back(std::string());
                    }
                }
            }
            if (word)
            {
                out.push_back(s.substr(wordStart, i - wordStart));
            }
            return out;
        }

        std::vector<std::string> split(const std::string& s, const std::vector<char>& delimeters, bool keepEmpty)
        {
            std::vector<std::string> out;
            bool word = false;
            size_t wordStart = 0;
            size_t i = 0;
            for (; i < s.size(); ++i)
            {
                if (std::find(delimeters.begin(), delimeters.end(), s[i]) == delimeters.end())
                {
                    if (!word)
                    {
                        word = true;
                        wordStart = i;
                    }
                }
                else
                {
                    if (word)
                    {
                        word = false;
                        out.push_back(s.substr(wordStart, i - wordStart));
                    }
                    if (keepEmpty && i > 0 && std::find(delimeters.begin(), delimeters.end(), s[i - 1]) != delimeters.end())
                    {
                        out.push_back(std::string());
                    }
                }
            }
            if (word)
            {
                out.push_back(s.substr(wordStart, i - wordStart));
            }
            return out;
        }

        std::string join(const std::vector<std::string>& values, const std::string& delimeter)
        {
            std::string out;
            const size_t size = values.size();
            for (size_t i = 0; i < size; ++i)
            {
                out += values[i];
                if (i < size - 1)
                {
                    out += delimeter;
                }
            }
            return out;
        }

        std::string toUpper(const std::string& value)
        {
            std::string out;
            for (auto i : value)
            {
                out.push_back(std::toupper(i));
            }
            return out;
        }

        std::string toLower(const std::string& value)
        {
            std::string out;
            for (auto i : value)
            {
                out.push_back(std::tolower(i));
            }
            return out;
        }

        bool compareNoCase(const std::string& a, const std::string& b)
        {
            return toLower(a) == toLower(b);
        }
    }
}