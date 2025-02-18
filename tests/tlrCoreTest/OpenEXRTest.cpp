// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#include <tlrCoreTest/OpenEXRTest.h>

#include <tlrCore/Assert.h>
#include <tlrCore/OpenEXR.h>

#include <sstream>

namespace tlr
{
    namespace CoreTest
    {
        OpenEXRTest::OpenEXRTest() :
            ITest("CoreTest::OpenEXRTest")
        {}

        std::shared_ptr<OpenEXRTest> OpenEXRTest::create()
        {
            return std::shared_ptr<OpenEXRTest>(new OpenEXRTest);
        }

        void OpenEXRTest::run()
        {
            auto plugin = exr::Plugin::create();
            const std::map<std::string, std::string> tags =
            {
                { "Chromaticities", "1.2 2.3 3.4 4.5 5.6 6.7 7.8 8.9" },
                { "White Luminance", "1.2" },
                { "X Density", "1.2" },
                { "Owner", "Owner" },
                { "Comments", "Comments" },
                { "Capture Date", "Capture Date" },
                { "UTC Offset", "1.2" },
                { "Longitude", "1.2" },
                { "Latitude", "1.2" },
                { "Altitude", "1.2" },
                { "Focus", "1.2" },
                { "Exposure Time", "1.2" },
                { "Aperture", "1.2" },
                { "ISO Speed", "1.2" },
                { "Keycode", "1:2:3:4:5" },
                { "Timecode", "01:02:03:04" }
            };
            for (const auto& size : std::vector<imaging::Size>(
                {
                    imaging::Size(16, 16),
                    imaging::Size(1, 1),
                    imaging::Size(0, 0)
                }))
            {
                for (const auto& pixelType : plugin->getWritePixelTypes())
                {
                    std::string fileName;
                    {
                        std::stringstream ss;
                        ss << "OpenEXRTest_" << size << '_' << pixelType << ".0.exr";
                        fileName = ss.str();
                        _print(fileName);
                    }
                    auto imageInfo = imaging::Info(size, pixelType);
                    imageInfo.layout.alignment = plugin->getWriteAlignment(pixelType);
                    imageInfo.layout.endian = plugin->getWriteEndian();
                    auto image = imaging::Image::create(imageInfo);
                    image->setTags(tags);
                    try
                    {
                        {
                            avio::Info info;
                            info.video.push_back(imageInfo);
                            info.videoDuration = otime::RationalTime(1.0, 24.0);
                            info.tags = tags;
                            auto write = plugin->write(fileName, info);
                            write->writeVideoFrame(otime::RationalTime(0.0, 24.0), image);
                        }
                        auto read = plugin->read(fileName);
                        const auto videoFrame = read->readVideoFrame(otime::RationalTime(0.0, 24.0)).get();
                        const auto frameTags = videoFrame.image->getTags();
                        for (const auto& j : tags)
                        {
                            const auto k = frameTags.find(j.first);
                            TLR_ASSERT(k != frameTags.end());
                            TLR_ASSERT(k->second == j.second);
                        }
                    }
                    catch(const std::exception& e)
                    {
                        _printError(e.what());
                    }
                }
            }
        }
    }
}
