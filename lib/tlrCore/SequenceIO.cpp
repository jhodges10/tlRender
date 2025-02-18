// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#include <tlrCore/SequenceIO.h>

#include <tlrCore/Assert.h>
#include <tlrCore/Cache.h>
#include <tlrCore/File.h>

#include <atomic>
#include <condition_variable>
#include <iomanip>
#include <queue>
#include <list>
#include <mutex>
#include <sstream>
#include <thread>

namespace tlr
{
    namespace avio
    {
        struct ISequenceRead::Private
        {
            std::string path;
            std::string baseName;
            std::string number;
            int pad = 0;
            std::string extension;

            std::promise<Info> infoPromise;

            struct VideoFrameRequest
            {
                VideoFrameRequest() {}
                VideoFrameRequest(VideoFrameRequest&&) = default;

                otime::RationalTime time = invalidTime;
                std::promise<VideoFrame> promise;
            };
            std::list<VideoFrameRequest> videoFrameRequests;
            std::condition_variable requestCV;
            std::mutex requestMutex;
            memory::Cache<std::string, VideoFrame> videoFrameCache;

            std::thread thread;
            std::atomic<bool> running;
            std::atomic<bool> stopped;
        };

        void ISequenceRead::_init(
            const std::string& fileName,
            const Options& options)
        {
            IRead::_init(fileName, options);

            TLR_PRIVATE_P();

            file::split(fileName, &p.path, &p.baseName, &p.number, &p.extension);
            p.pad = !p.number.empty() ? ('0' == p.number[0] ? p.number.size() : 0) : 0;

            p.videoFrameCache.setMax(1);

            p.running = true;
            p.stopped = false;
            p.thread = std::thread(
                [this, fileName]
                {
                    TLR_PRIVATE_P();
                    try
                    {
                        p.infoPromise.set_value(_getInfo(fileName));
                        _run();
                    }
                    catch (const std::exception&)
                    {
                        p.infoPromise.set_value(Info());
                    }
                    p.stopped = true;
                    std::list<Private::VideoFrameRequest> videoFrameRequests;
                    {
                        std::unique_lock<std::mutex> lock(p.requestMutex);
                        videoFrameRequests.swap(p.videoFrameRequests);
                    }
                    for (auto& i : videoFrameRequests)
                    {
                        i.promise.set_value(VideoFrame());
                    }
                });
        }

        ISequenceRead::ISequenceRead() :
            _p(new Private)
        {}

        ISequenceRead::~ISequenceRead()
        {
            TLR_PRIVATE_P();
            p.running = false;
            if (p.thread.joinable())
            {
                p.thread.join();
            }
        }

        std::future<Info> ISequenceRead::getInfo()
        {
            return _p->infoPromise.get_future();
        }

        std::future<VideoFrame> ISequenceRead::readVideoFrame(const otime::RationalTime& time)
        {
            TLR_PRIVATE_P();
            Private::VideoFrameRequest request;
            request.time = time;
            auto future = request.promise.get_future();
            if (!p.stopped)
            {
                {
                    std::unique_lock<std::mutex> lock(p.requestMutex);
                    p.videoFrameRequests.push_back(std::move(request));
                }
                p.requestCV.notify_one();
            }
            else
            {
                request.promise.set_value(VideoFrame());
            }
            return future;
        }

        bool ISequenceRead::hasVideoFrames()
        {
            TLR_PRIVATE_P();
            std::unique_lock<std::mutex> lock(p.requestMutex);
            return !p.videoFrameRequests.empty();
        }

        void ISequenceRead::cancelVideoFrames()
        {
            TLR_PRIVATE_P();
            std::unique_lock<std::mutex> lock(p.requestMutex);
            p.videoFrameRequests.clear();
        }

        void ISequenceRead::stop()
        {
            _p->running = false;
        }

        bool ISequenceRead::hasStopped() const
        {
            return _p->stopped;
        }

        void ISequenceRead::_run()
        {
            TLR_PRIVATE_P();
            while (p.running)
            {
                struct Result
                {
                    std::string fileName;
                    otime::RationalTime time = invalidTime;
                    std::future<VideoFrame> future;
                    std::promise<VideoFrame> promise;
                };
                std::vector<Result> results;
                {
                    std::unique_lock<std::mutex> lock(p.requestMutex);
                    p.requestCV.wait_for(
                        lock,
                        sequenceRequestTimeout,
                        [this]
                        {
                            return !_p->videoFrameRequests.empty();
                        });
                    for (size_t i = 0; i < sequenceThreadCount && !p.videoFrameRequests.empty(); ++i)
                    {
                        Result result;
                        result.time = p.videoFrameRequests.front().time;
                        result.promise = std::move(p.videoFrameRequests.front().promise);
                        results.push_back(std::move(result));
                        p.videoFrameRequests.pop_front();
                    }
                }

                auto it = results.begin();
                while (it != results.end())
                {
                    //std::cout << "request: " << it->time << std::endl;
                    std::stringstream ss;
                    if (!p.number.empty())
                    {
                        ss << p.path << p.baseName << std::setfill('0') << std::setw(p.pad) << static_cast<int>(it->time.value()) << p.extension;
                    }
                    else
                    {
                        ss << _fileName;
                    }
                    it->fileName = ss.str();
                    VideoFrame videoFrame;
                    if (p.videoFrameCache.get(it->fileName, videoFrame))
                    {
                        it->promise.set_value(videoFrame);
                        it = results.erase(it);
                    }
                    else
                    {
                        const auto fileName = it->fileName;
                        const auto time = it->time;
                        it->future = std::async(
                            std::launch::async,
                            [this, fileName, time]
                            {
                                VideoFrame out;
                                try
                                {
                                    out = _readVideoFrame(fileName, time);
                                }
                                catch (const std::exception&)
                                {}
                                return out;
                            });
                        ++it;
                    }
                }
                for (auto& i : results)
                {
                    auto videoFrame = i.future.get();
                    i.promise.set_value(videoFrame);
                    p.videoFrameCache.add(i.fileName, videoFrame);
                }
            }
        }

        struct ISequenceWrite::Private
        {
            std::string path;
            std::string baseName;
            std::string number;
            int pad = 0;
            std::string extension;

            double defaultSpeed = sequenceDefaultSpeed;
        };

        void ISequenceWrite::_init(
            const std::string& fileName,
            const Info& info,
            const Options& options)
        {
            IWrite::_init(fileName, options, info);

            TLR_PRIVATE_P();

            file::split(fileName, &p.path, &p.baseName, &p.number, &p.extension);
            p.pad = !p.number.empty() ? ('0' == p.number[0] ? p.number.size() : 0) : 0;

            const auto i = options.find("DefaultSpeed");
            if (i != options.end())
            {
                std::stringstream ss(i->second);
                ss >> p.defaultSpeed;
            }
        }

        ISequenceWrite::ISequenceWrite() :
            _p(new Private)
        {}

        ISequenceWrite::~ISequenceWrite()
        {}

        void ISequenceWrite::writeVideoFrame(
            const otime::RationalTime& time,
            const std::shared_ptr<imaging::Image>& image)
        {
            TLR_PRIVATE_P();
            std::stringstream ss;
            ss << p.path << p.baseName << std::setfill('0') << std::setw(p.pad) << static_cast<int>(time.value()) << p.extension;
            _writeVideoFrame(ss.str(), time, image);
        }
    }
}
