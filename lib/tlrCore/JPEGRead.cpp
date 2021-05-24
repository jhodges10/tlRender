// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#include <tlrCore/JPEG.h>

#include <tlrCore/Assert.h>
#include <tlrCore/StringFormat.h>

#include <cstring>

namespace tlr
{
    namespace jpeg
    {
        namespace
        {
            bool jpegCreate(
                jpeg_decompress_struct* decompress,
                ErrorStruct* error)
            {
                if (::setjmp(error->jump))
                {
                    return false;
                }
                jpeg_create_decompress(decompress);
                return true;
            }

            bool jpegOpen(
                FILE* f,
                jpeg_decompress_struct* decompress,
                ErrorStruct* error)
            {
                if (::setjmp(error->jump))
                {
                    return false;
                }
                jpeg_stdio_src(decompress, f);
                jpeg_save_markers(decompress, JPEG_COM, 0xFFFF);
                if (!jpeg_read_header(decompress, static_cast<boolean>(1)))
                {
                    return false;
                }
                if (!jpeg_start_decompress(decompress))
                {
                    return false;
                }
                return true;
            }

            bool jpegScanline(
                jpeg_decompress_struct* decompress,
                uint8_t* out,
                ErrorStruct* error)
            {
                if (::setjmp(error->jump))
                {
                    return false;
                }
                JSAMPROW p[] = { (JSAMPLE*)(out) };
                if (!jpeg_read_scanlines(decompress, p, 1))
                {
                    return false;
                }
                return true;
            }

            bool jpegEnd(
                jpeg_decompress_struct* decompress,
                ErrorStruct* error)
            {
                if (::setjmp(error->jump))
                {
                    return false;
                }
                jpeg_finish_decompress(decompress);
                return true;
            }

            class File
            {
            public:
                File(const std::string& fileName)
                {
                    std::memset(&decompress, 0, sizeof(jpeg_decompress_struct));

                    decompress.err = jpeg_std_error(&error.pub);
                    error.pub.error_exit = errorFunc;
                    error.pub.emit_message = warningFunc;
                    if (!jpegCreate(&decompress, &error))
                    {
                        throw std::runtime_error(string::Format("{0}: Cannot open").arg(fileName));
                    }
                    init = true;
                    f = fopen(fileName.c_str(), "rb");
                    if (!f)
                    {
                        throw std::runtime_error(string::Format("{0}: Cannot open").arg(fileName));
                    }
                    if (!jpegOpen(f, &decompress, &error))
                    {
                        throw std::runtime_error(string::Format("{0}: Cannot open").arg(fileName));
                    }

                    imaging::PixelType pixelType = imaging::getIntType(decompress.out_color_components, 8);
                    if (imaging::PixelType::None == pixelType)
                    {
                        throw std::runtime_error(string::Format("{0}: File not supported").arg(fileName));
                    }

                    info = imaging::Info(decompress.output_width, decompress.output_height, pixelType);
                }

                ~File()
                {
                    if (init)
                    {
                        jpeg_destroy_decompress(&decompress);
                        init = false;
                    }
                    if (f)
                    {
                        fclose(f);
                        f = nullptr;
                    }
                }

                FILE*                  f = nullptr;
                jpeg_decompress_struct decompress;
                bool                   init = false;
                ErrorStruct            error;
                imaging::Info          info;
            };
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
            io::VideoInfo videoInfo;
            videoInfo.info = std::shared_ptr<File>(new File(fileName))->info;
            videoInfo.duration = _defaultSpeed;
            videoInfo.codec = "JPEG";
            out.video.push_back(videoInfo);
            return out;
        }

        io::VideoFrame Read::_readVideoFrame(
            const std::string& fileName,
            const otime::RationalTime& time,
            const std::shared_ptr<imaging::Image>& image)
        {
            io::VideoFrame out;

            auto f = std::shared_ptr<File>(new File(fileName));

            out.time = time;
            if (image && image->getInfo() == f->info)
            {
                out.image = image;
            }
            else
            {
                out.image = imaging::Image::create(f->info);
            }

            std::size_t scanlineByteCount = 0;
            switch (f->info.pixelType)
            {
            case imaging::PixelType::L_U8:
                scanlineByteCount = f->info.size.w;
                break;
            case imaging::PixelType::RGB_U8:
                scanlineByteCount = f->info.size.w * 3;
                break;
            }
            for (uint16_t y = 0; y < f->info.size.h; ++y)
            {
                if (!jpegScanline(&f->decompress, out.image->getData() + scanlineByteCount * y, &f->error))
                {
                    break;
                }
            }

            jpegEnd(&f->decompress, &f->error);

            return out;
        }
    }
}