// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrCore/SequenceIO.h>

extern "C"
{
#include <jpeglib.h>

} // extern "C"

#include <setjmp.h>

namespace tlr
{
    //! JPEG I/O.
    namespace jpeg
    {
        //! JPEG error.
        struct ErrorStruct
        {
            struct jpeg_error_mgr pub;
            std::vector<std::string> messages;
            jmp_buf jump;
        };

        //! JPEG error function.
        void errorFunc(j_common_ptr);

        //! JPEG warning function.
        void warningFunc(j_common_ptr, int level);

        //! JPEG reader.
        class Read : public avio::ISequenceRead
        {
        protected:
            void _init(
                const std::string& fileName,
                const avio::Options&);
            Read();

        public:
            ~Read() override;

            //! Create a new reader.
            static std::shared_ptr<Read> create(
                const std::string& fileName,
                const avio::Options&);

        protected:
            avio::Info _getInfo(const std::string& fileName) override;
            avio::VideoFrame _readVideoFrame(
                const std::string& fileName,
                const otime::RationalTime&) override;
        };

        //! JPEG writer.
        class Write : public avio::ISequenceWrite
        {
        protected:
            void _init(
                const std::string& fileName,
                const avio::Info&,
                const avio::Options&);
            Write();

        public:
            ~Write() override;

            //! Create a new writer.
            static std::shared_ptr<Write> create(
                const std::string& fileName,
                const avio::Info&,
                const avio::Options&);

        protected:
            void _writeVideoFrame(
                const std::string& fileName,
                const otime::RationalTime&,
                const std::shared_ptr<imaging::Image>&) override;
        };

        //! JPEG plugin.
        class Plugin : public avio::IPlugin
        {
        protected:
            Plugin();

        public:
            //! Create a new plugin.
            static std::shared_ptr<Plugin> create();

            std::shared_ptr<avio::IRead> read(
                const std::string& fileName,
                const avio::Options& = avio::Options()) override;
            std::vector<imaging::PixelType> getWritePixelTypes() const override;
            std::shared_ptr<avio::IWrite> write(
                const std::string& fileName,
                const avio::Info&,
                const avio::Options& = avio::Options()) override;
        };
    }
}
