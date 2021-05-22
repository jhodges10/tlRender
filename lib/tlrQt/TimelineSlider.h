// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrQt/TimeObject.h>
#include <tlrQt/TimelinePlayer.h>
#include <tlrQt/TimelineThumbnailProvider.h>

#include <QWidget>

namespace tlr
{
    namespace qt
    {
        //! Timeline slider.
        class TimelineSlider : public QWidget
        {
            Q_OBJECT

        public:
            TimelineSlider(QWidget* parent = nullptr);

            //! Set the time object.
            void setTimeObject(TimeObject*);

            //! Set the timeline player.
            void setTimelinePlayer(TimelinePlayer*);

        public Q_SLOTS:
            //! Set the time units.
            void setUnits(qt::TimeObject::Units);

        protected:
            void resizeEvent(QResizeEvent*) override;
            void paintEvent(QPaintEvent*) override;
            void mousePressEvent(QMouseEvent*) override;
            void mouseReleaseEvent(QMouseEvent*) override;
            void mouseMoveEvent(QMouseEvent*) override;

        private Q_SLOTS:
            void _currentTimeCallback(const otime::RationalTime&);
            void _inOutRangeCallback(const otime::TimeRange&);
            void _cachedFramesCallback(const std::vector<otime::TimeRange>&);
            void _thumbnailsCallback(const QList<QPair<otime::RationalTime, QPixmap> >&);

        private:
            otime::RationalTime _posToTime(int) const;
            int _timeToPos(const otime::RationalTime&) const;

            void _thumbnailsUpdate();

            TimelinePlayer* _timelinePlayer = nullptr;
            std::vector<otime::TimeRange> _clipRanges;
            TimelineThumbnailProvider* _thumbnailProvider = nullptr;
            std::map<otime::RationalTime, QPixmap> _thumbnails;
            TimeObject::Units _units = TimeObject::Units::Timecode;
            TimeObject* _timeObject = nullptr;
        };
    }
}
