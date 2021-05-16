// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#include <tlrCore/Timeline.h>

#include <tlrCore/Error.h>
#include <tlrCore/File.h>
#include <tlrCore/IO.h>
#include <tlrCore/String.h>

#include <opentimelineio/imageSequenceReference.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/stackAlgorithm.h>
#include <opentimelineio/timeline.h>

#if defined(TLR_ENABLE_PYTHON)
#include <Python.h>
#endif

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace tlr
{
    namespace timeline
    {
        TLR_ENUM_VECTOR_IMPL(Playback);
        TLR_ENUM_LABEL_IMPL(Playback, "Stop", "Forward", "Reverse");

        TLR_ENUM_VECTOR_IMPL(Loop);
        TLR_ENUM_LABEL_IMPL(Loop, "Loop", "Once", "Ping-Pong");

        TLR_ENUM_VECTOR_IMPL(Frame);
        TLR_ENUM_LABEL_IMPL(Frame, "Start", "End", "Prev", "Next");

        otime::RationalTime loopTime(const otime::RationalTime& time, const otime::TimeRange& range)
        {
            auto out = time;
            if (out < range.start_time())
            {
                out = range.end_time_inclusive();
            }
            else if (out > range.end_time_inclusive())
            {
                out = range.start_time();
            }
            return out;
        }

        std::vector<std::string> getExtensions()
        {
            //! \todo Get extensions for the Python adapters.
            return { ".otio" };
        }

        math::BBox2f fitWindow(const imaging::Size& image, const imaging::Size& window)
        {
            math::BBox2f out;
            const float windowAspect = window.getAspect();
            const float imageAspect = image.getAspect();
            math::BBox2f bbox;
            if (windowAspect > imageAspect)
            {
                out = math::BBox2f(
                    window.w / 2.F - (window.h * imageAspect) / 2.F,
                    0.F,
                    window.h * imageAspect,
                    window.h);
            }
            else
            {
                out = math::BBox2f(
                    0.F,
                    window.h / 2.F - (window.w / imageAspect) / 2.F,
                    window.w,
                    window.w / imageAspect);
            }
            return out;
        }

        std::vector<otime::TimeRange> toRanges(std::vector<otime::RationalTime> frames)
        {
            std::vector<otime::TimeRange> out;
            if (!frames.empty())
            {
                std::sort(frames.begin(), frames.end());
                auto i = frames.begin();
                auto j = i;
                do
                {
                    auto k = j + 1;
                    if (k != frames.end() && (*k - *j).value() > 1)
                    {
                        out.push_back(otime::TimeRange::range_from_start_end_time_inclusive(*i, *j));
                        i = k;
                        j = k;
                    }
                    else if (k == frames.end())
                    {
                        out.push_back(otime::TimeRange::range_from_start_end_time_inclusive(*i, *j));
                        i = k;
                        j = k;
                    }
                    else
                    {
                        ++j;
                    }
                } while (j != frames.end());
            }
            return out;
        }

        namespace
        {
#if defined(TLR_ENABLE_PYTHON)
            class PyObjectRef
            {
            public:
                PyObjectRef(PyObject* o) :
                    o(o)
                {
                    if (!o)
                    {
                        throw std::runtime_error("Python error");
                    }
                }

                ~PyObjectRef()
                {
                    Py_XDECREF(o);
                }

                PyObject* o = nullptr;

                operator PyObject* () const { return o; }
            };
#endif

            otio::SerializableObject::Retainer<otio::Timeline> read(
                const std::string& fileName,
                otio::ErrorStatus* errorStatus)
            {
                otio::SerializableObject::Retainer<otio::Timeline> out;
#if defined(TLR_ENABLE_PYTHON)
                Py_Initialize();
                try
                {
                    auto pyModule = PyObjectRef(PyImport_ImportModule("opentimelineio.adapters"));

                    auto pyReadFromFile = PyObjectRef(PyObject_GetAttrString(pyModule, "read_from_file"));
                    auto pyReadFromFileArgs = PyObjectRef(PyTuple_New(1));
                    const std::string fileNameNormalized = file::normalize(fileName);
                    auto pyReadFromFileArg = PyUnicode_FromStringAndSize(fileNameNormalized.c_str(), fileNameNormalized.size());
                    if (!pyReadFromFileArg)
                    {
                        throw std::runtime_error("Cannot create arg");
                    }
                    PyTuple_SetItem(pyReadFromFileArgs, 0, pyReadFromFileArg);
                    auto pyTimeline = PyObjectRef(PyObject_CallObject(pyReadFromFile, pyReadFromFileArgs));

                    auto pyToJSONString = PyObjectRef(PyObject_GetAttrString(pyTimeline, "to_json_string"));
                    auto pyJSONString = PyObjectRef(PyObject_CallObject(pyToJSONString, NULL));
                    out = otio::SerializableObject::Retainer<otio::Timeline>(
                        dynamic_cast<otio::Timeline*>(otio::Timeline::from_json_string(
                            PyUnicode_AsUTF8AndSize(pyJSONString, NULL),
                            errorStatus)));
                }
                catch (const std::exception& e)
                {
                    errorStatus->outcome = otio::ErrorStatus::Outcome::FILE_OPEN_FAILED;
                    errorStatus->details = e.what();
                }
                if (PyErr_Occurred())
                {
                    PyErr_Print();
                }
                Py_Finalize();
#else
                out = dynamic_cast<otio::Timeline*>(otio::Timeline::from_json_file(fileName, errorStatus));
#endif
                return out;
            }

            std::string _getFileName(const otio::ImageSequenceReference* ref)
            {
                std::stringstream ss;
                ss << ref->target_url_base() <<
                    ref->name_prefix() <<
                    std::setfill('0') << std::setw(ref->frame_zero_padding()) << ref->start_frame() <<
                    ref->name_suffix();
                return ss.str();
            }

            std::string _getFileName(const otio::MediaReference* ref)
            {
                std::string out;
                if (auto externalRef = dynamic_cast<const otio::ExternalReference*>(ref))
                {
                    out = externalRef->target_url();
                }
                else if (auto imageSequenceRef = dynamic_cast<const otio::ImageSequenceReference*>(ref))
                {
                    out = _getFileName(imageSequenceRef);
                }
                return out;
            }
        }

        void Timeline::_init(const std::string& fileName)
        {
            _fileName = fileName;

            // Read the timeline.
            otio::ErrorStatus errorStatus;
            _timeline = read(_fileName, &errorStatus);
            if (errorStatus != otio::ErrorStatus::OK)
            {
                throw std::runtime_error(errorStatus.full_description);
            }
            _duration = _timeline.value->duration(&errorStatus);
            if (errorStatus != otio::ErrorStatus::OK)
            {
                throw std::runtime_error(errorStatus.full_description);
            }
            if (_timeline.value->global_start_time().has_value())
            {
                _globalStartTime = _timeline.value->global_start_time().value();
            }
            else
            {
                _globalStartTime = otime::RationalTime(0, _duration.rate());
            }

            // Flatten the timeline.
            _flattenedTimeline = otio::flatten_stack(_timeline.value->tracks(), &errorStatus);
            if (errorStatus != otio::ErrorStatus::OK)
            {
                throw std::runtime_error(errorStatus.full_description);
            }

            // Create the I/O system.
            _ioSystem = io::System::create();

            // Change the working directory.
            std::string path;
            file::split(_fileName, &path);
            file::changeDir(path);

            // The first clip defines the image information.
            for (const auto& child : _flattenedTimeline.value->children())
            {
                if (auto clip = dynamic_cast<otio::Clip*>(child.value))
                {
                    otio::ErrorStatus errorStatus;
                    auto range = clip->range_in_parent(&errorStatus);
                    if (errorStatus != otio::ErrorStatus::OK)
                    {
                        throw std::runtime_error(errorStatus.full_description);
                    }

                    if (auto read = _ioSystem->read(_getFileName(clip->media_reference())))
                    {
                        const auto info = read->getInfo().get();
                        if (!info.video.empty())
                        {
                            _imageInfo = info.video[0].info;
                            break;
                        }
                    }
                }
            }

            // Create observers.
            _playback = Observer::ValueSubject<Playback>::create(Playback::Stop);
            _loop = Observer::ValueSubject<Loop>::create(Loop::Loop);
            _currentTime = Observer::ValueSubject<otime::RationalTime>::create(_globalStartTime);
            _inOutRange = Observer::ValueSubject<otime::TimeRange>::create(
                otime::TimeRange(_globalStartTime, _duration));
            _frame = Observer::ValueSubject<io::VideoFrame>::create();
            _cachedFrames = Observer::ListSubject<otime::TimeRange>::create();
        }

        Timeline::Timeline()
        {}

        std::shared_ptr<Timeline> Timeline::create(const std::string& fileName)
        {
            auto out = std::shared_ptr<Timeline>(new Timeline);
            out->_init(fileName);
            return out;
        }

        void Timeline::setPlayback(Playback value)
        {
            switch (_loop->get())
            {
            case Loop::Once:
                switch (value)
                {
                case Playback::Forward:
                    if (_currentTime->get() == _inOutRange->get().end_time_inclusive())
                    {
                        seek(_inOutRange->get().start_time());
                    }
                    break;
                case Playback::Reverse:
                    if (_currentTime->get() == _inOutRange->get().start_time())
                    {
                        seek(_inOutRange->get().end_time_inclusive());
                    }
                    break;
                }
                break;
            case Loop::PingPong:
                switch (value)
                {
                case Playback::Forward:
                    if (_currentTime->get() == _inOutRange->get().end_time_inclusive())
                    {
                        value = Playback::Reverse;
                    }
                    break;
                case Playback::Reverse:
                    if (_currentTime->get() == _inOutRange->get().start_time())
                    {
                        value = Playback::Forward;
                    }
                    break;
                }
                break;
            default:
                break;
            }
            if (_playback->setIfChanged(value))
            {
                if (value != Playback::Stop)
                {
                    _startTime = std::chrono::steady_clock::now();
                    _playbackStartTime = _currentTime->get();
                    _frameCacheDirection = Playback::Forward == value ? FrameCacheDirection::Forward : FrameCacheDirection::Reverse;
                }
            }
        }

        void Timeline::setLoop(Loop value)
        {
            _loop->setIfChanged(value);
        }

        void Timeline::seek(const otime::RationalTime& time)
        {
            // Loop the time.
            otio::ErrorStatus errorStatus;
            auto range = _flattenedTimeline.value->available_range(&errorStatus);
            range = otime::TimeRange(_globalStartTime, range.duration());
            if (errorStatus != otio::ErrorStatus::OK)
            {
                throw std::runtime_error(errorStatus.full_description);
            }
            const auto tmp = loopTime(time, range);

            if (_currentTime->setIfChanged(tmp))
            {
                // Update playback.
                if (_playback->get() != Playback::Stop)
                {
                    _startTime = std::chrono::steady_clock::now();
                    _playbackStartTime = _currentTime->get();
                }

                // Cancel video frame requests.
                for (auto& j : _readers)
                {
                    j.second.read->cancelVideoFrames();
                    j.second.videoFrames.clear();
                }
            }
        }

        void Timeline::frame(Frame frame)
        {
            setPlayback(timeline::Playback::Stop);
            switch (frame)
            {
            case Frame::Start:
                seek(_inOutRange->get().start_time());
                break;
            case Frame::End:
                seek(_inOutRange->get().end_time_inclusive());
                break;
            case Frame::Prev:
                seek(otime::RationalTime(_currentTime->get().value() - 1, _duration.rate()));
                break;
            case Frame::Next:
                seek(otime::RationalTime(_currentTime->get().value() + 1, _duration.rate()));
                break;
            default:
                break;
            }
        }

        void Timeline::start()
        {
            frame(Frame::Start);
        }

        void Timeline::end()
        {
            frame(Frame::End);
        }

        void Timeline::prev()
        {
            frame(Frame::Prev);
        }

        void Timeline::next()
        {
            frame(Frame::Next);
        }

        void Timeline::setInOutRange(const otime::TimeRange& value)
        {
            _inOutRange->setIfChanged(value);
        }

        void Timeline::setInPoint()
        {
            const auto range = otime::TimeRange::range_from_start_end_time(
                _currentTime->get(),
                _inOutRange->get().end_time_exclusive());
            _inOutRange->setIfChanged(range);
        }

        void Timeline::resetInPoint()
        {
            const auto range = otime::TimeRange::range_from_start_end_time(
                _globalStartTime,
                _inOutRange->get().end_time_exclusive());
            _inOutRange->setIfChanged(range);
        }

        void Timeline::setOutPoint()
        {
            const auto range = otime::TimeRange::range_from_start_end_time_inclusive(
                _inOutRange->get().start_time(),
                _currentTime->get());
            _inOutRange->setIfChanged(range);
        }

        void Timeline::resetOutPoint()
        {
            _inOutRange->setIfChanged(otime::TimeRange(_inOutRange->get().start_time(), _duration));
        }

        void Timeline::setFrameCacheReadAhead(int value)
        {
            _frameCacheReadAhead = value;
        }

        void Timeline::setFrameCacheReadBehind(int value)
        {
            _frameCacheReadBehind = value;
        }

        void Timeline::tick()
        {
            // Calculate the current time.
            otio::ErrorStatus errorStatus;
            const auto playback = _playback->get();
            if (playback != Playback::Stop)
            {
                const auto now = std::chrono::steady_clock::now();
                const std::chrono::duration<float> diff = now - _startTime;
                auto currentTime = _playbackStartTime +
                    otime::RationalTime(floor(diff.count() * _duration.rate() * (Playback::Forward == playback ? 1.0 : -1.0)), _duration.rate());
                _currentTime->setIfChanged(_loopPlayback(currentTime));
            }

            //! Update the frame cache.
            _frameCacheUpdate();

            // Update the current frame.
            const auto i = _frameCache.find(_currentTime->get());
            _frame->setIfChanged(i != _frameCache.end() ? i->second : io::VideoFrame());
        }

        otime::TimeRange Timeline::_getRange(const otio::SerializableObject::Retainer<otio::Clip>& clip) const
        {
            otime::TimeRange out;
            otio::ErrorStatus errorStatus;
            const auto& optional = clip.value->trimmed_range_in_parent(&errorStatus);
            if (errorStatus != otio::ErrorStatus::OK)
            {
                throw std::runtime_error(errorStatus.full_description);
            }
            bool outOfRange = true;
            if (optional.has_value())
            {
                out = otime::TimeRange(
                    _globalStartTime + optional.value().start_time(),
                    optional.value().duration());
            }
            return out;
        }

        otime::RationalTime Timeline::_loopPlayback(const otime::RationalTime& time)
        {
            otime::RationalTime out = time;

            const auto range = _inOutRange->get();
            switch (_loop->get())
            {
            case Loop::Loop:
            {
                const auto tmp = loopTime(out, range);
                if (tmp != out)
                {
                    out = tmp;
                    _startTime = std::chrono::steady_clock::now();
                    _playbackStartTime = tmp;
                }
                break;
            }
            case Loop::Once:
                if (out < range.start_time())
                {
                    out = range.start_time();
                    _playback->setIfChanged(Playback::Stop);
                }
                else if (out > range.end_time_inclusive())
                {
                    out = range.end_time_inclusive();
                    _playback->setIfChanged(Playback::Stop);
                }
                break;
            case Loop::PingPong:
            {
                const auto playback = _playback->get();
                if (out < range.start_time() && Playback::Reverse == playback)
                {
                    out = range.start_time();
                    _playback->setIfChanged(Playback::Forward);
                    _startTime = std::chrono::steady_clock::now();
                    _playbackStartTime = out;
                }
                else if (out > range.end_time_inclusive() && Playback::Forward == playback)
                {
                    out = range.end_time_inclusive();
                    _playback->setIfChanged(Playback::Reverse);
                    _startTime = std::chrono::steady_clock::now();
                    _playbackStartTime = out;
                }
                break;
            }
            default:
                break;
            }

            return out;
        }

        void Timeline::_frameCacheUpdate()
        {
            // Get which frames should be cached.
            std::vector<otime::RationalTime> frames;
            auto time = _currentTime->get();
            const auto range = _inOutRange->get();
            for (size_t i = 0; i < (FrameCacheDirection::Forward == _frameCacheDirection ? _frameCacheReadBehind : _frameCacheReadAhead); ++i)
            {
                time = loopTime(time - otime::RationalTime(1, _duration.rate()), range);
            }
            for (size_t i = 0; i < _frameCacheReadBehind + _frameCacheReadAhead; ++i)
            {
                frames.push_back(time);
                time = loopTime(time + otime::RationalTime(1, _duration.rate()), range);
            }

            // Remove old frames from the cache.
            const auto ranges = toRanges(frames);
            auto frameCacheIt = _frameCache.begin();
            while (frameCacheIt != _frameCache.end())
            {
                bool old = true;
                for (const auto& i : ranges)
                {
                    if (i.contains(frameCacheIt->second.time))
                    {
                        old = false;
                        break;
                    }
                }
                if (old)
                {
                    frameCacheIt = _frameCache.erase(frameCacheIt);
                }
                else
                {
                    ++frameCacheIt;
                }
            }

            // Find uncached frames.
            std::vector<otime::RationalTime> uncached;
            for (const auto& i : frames)
            {
                const auto j = _frameCache.find(i);
                if (j == _frameCache.end())
                {
                    uncached.push_back(i);
                }
            }

            // Create I/O readers for uncached frames.
            for (const auto& i : uncached)
            {
                for (const auto& child : _flattenedTimeline.value->children())
                {
                    if (auto clip = dynamic_cast<otio::Clip*>(child.value))
                    {
                        const auto& range = _getRange(clip);
                        if (range.contains(i))
                        {
                            otio::ErrorStatus errorStatus;
                            auto time = _flattenedTimeline.value->transformed_time(i - _globalStartTime, clip, &errorStatus);
                            if (errorStatus != otio::ErrorStatus::OK)
                            {
                                throw std::runtime_error(errorStatus.full_description);
                            }

                            // Is there an existing I/O reader?
                            const auto j = _readers.find(clip);
                            if (j != _readers.end())
                            {
                                const auto k = j->second.videoFrames.find(i);
                                if (k == j->second.videoFrames.end())
                                {
                                    time = time.rescaled_to(j->second.info.video[0].duration);
                                    j->second.videoFrames[i] = j->second.read->getVideoFrame(otime::RationalTime(floor(time.value()), time.rate()));
                                }
                            }
                            else
                            {
                                // Create a new I/O reader.
                                const std::string fileName = _getFileName(clip->media_reference());
                                if (auto read = _ioSystem->read(fileName, otime::RationalTime(0, _duration.rate())))
                                {
                                    //std::cout << "read: " << fileName << std::endl;
                                    Reader reader;
                                    reader.read = read;
                                    reader.info = read->getInfo().get();
                                    time = time.rescaled_to(reader.info.video[0].duration);
                                    reader.videoFrames[i] = read->getVideoFrame(otime::RationalTime(floor(time.value()), time.rate()));
                                    _readers[clip] = std::move(reader);
                                }
                                else
                                {
                                    //! \todo How should this be handled?
                                }
                            }
                        }
                    }
                }
            }

            // Get frames from the I/O readers.
            auto readerIt = _readers.begin();
            while (readerIt != _readers.end())
            {
                auto videoFramesIt = readerIt->second.videoFrames.begin();
                while (videoFramesIt != readerIt->second.videoFrames.end())
                {
                    bool delVideoFrame = false;
                    if (videoFramesIt->second.valid() &&
                        videoFramesIt->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    {
                        delVideoFrame = true;
                        auto videoFrame = videoFramesIt->second.get();
                        auto i = std::find(uncached.begin(), uncached.end(), videoFramesIt->first);
                        if (i != uncached.end())
                        {
                            //std::cout << "frame: " << *i << " / " << videoFrame.time << std::endl;
                            videoFrame.time = *i;
                            _frameCache[*i] = videoFrame;
                        }
                    }
                    if (delVideoFrame)
                    {
                        videoFramesIt = readerIt->second.videoFrames.erase(videoFramesIt);
                    }
                    else
                    {
                        ++videoFramesIt;
                    }
                }

                // Destroy the I/O reader if all the frames have been cached
                // and it is outside of the cache range.
                const auto& range = _getRange(readerIt->first);
                bool outOfRange = true;
                for (const auto& i : ranges)
                {
                    if (i.intersects(range))
                    {
                        outOfRange = false;
                        break;
                    }
                }
                if (outOfRange && readerIt->second.videoFrames.empty())
                {
                    //std::cout << "destroy: " << readerIt->second.read->getFileName() << std::endl;
                    readerIt = _readers.erase(readerIt);
                }
                else
                {
                    ++readerIt;
                }
            }

            // Update cached frames.
            std::vector<otime::RationalTime> cachedFrames;
            for (const auto& i : _frameCache)
            {
                cachedFrames.push_back(i.second.time);
            }
            _cachedFrames->setIfChanged(toRanges(cachedFrames));
        }
    }

    TLR_ENUM_SERIALIZE_IMPL(timeline, Playback);
    TLR_ENUM_SERIALIZE_IMPL(timeline, Loop);
}