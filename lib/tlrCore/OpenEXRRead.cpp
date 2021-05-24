// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#include <tlrCore/OpenEXR.h>

#include <tlrCore/StringFormat.h>

#include <ImfRgbaFile.h>

namespace tlr
{
    namespace exr
    {
        namespace
        {
            imaging::Info imfInfo(const Imf::RgbaInputFile& f)
            {
                imaging::PixelType pixelType = imaging::getFloatType(4, 16);
                if (imaging::PixelType::None == pixelType)
                {
                    throw std::runtime_error(string::Format("{0}: File not supported").arg(f.fileName()));
                }
                const auto dw = f.dataWindow();
                const int width = dw.max.x - dw.min.x + 1;
                const int height = dw.max.y - dw.min.y + 1;
                io::VideoInfo info;
                return imaging::Info(width, height, pixelType);
            }
        }

        void Read::_init(
            const std::string& fileName,
            const otime::RationalTime& defaultSpeed)
        {
            ISequenceRead::_init(fileName, defaultSpeed);
        }

        Read::Read()
        {}

        Read::~Read()
        {}

        std::shared_ptr<Read> Read::create(
            const std::string& fileName,
            const otime::RationalTime& defaultSpeed)
        {
            auto out = std::shared_ptr<Read>(new Read);
            out->_init(fileName, defaultSpeed);
            return out;
        }

        io::Info Read::_getInfo(const std::string& fileName)
        {
            io::Info out;
            Imf::RgbaInputFile f(fileName.c_str());
            io::VideoInfo videoInfo;
            videoInfo.info = imfInfo(f);
            videoInfo.duration = _defaultSpeed;
            videoInfo.codec = "EXR";
            out.video.push_back(videoInfo);
            return out;
        }

        io::VideoFrame Read::_readVideoFrame(
            const std::string& fileName,
            const otime::RationalTime& time,
            const std::shared_ptr<imaging::Image>& image)
        {
            io::VideoFrame out;

            Imf::RgbaInputFile f(fileName.c_str());

            out.time = time;
            const auto info = imfInfo(f);
            if (image && image->getInfo() == info)
            {
                out.image = image;
            }
            else
            {
                out.image = imaging::Image::create(info);
            }

            const auto dw = f.dataWindow();
            const int width = dw.max.x - dw.min.x + 1;
            const int height = dw.max.y - dw.min.y + 1;
            f.setFrameBuffer(
                reinterpret_cast<Imf::Rgba*>(out.image->getData()) - dw.min.x - dw.min.y * width,
                1,
                width);
            f.readPixels(dw.min.y, dw.max.y);

            return out;
        }
    }
}