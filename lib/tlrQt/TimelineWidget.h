// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrQt/TimeObject.h>
#include <tlrQt/TimelinePlayer.h>

#include <tlrGL/Render.h>

#include <QWidget>

namespace tlr
{
    namespace qt
    {
        //! Timeline widget.
        class TimelineWidget : public QWidget
        {
            Q_OBJECT

        public:
            TimelineWidget(QWidget* parent = nullptr);

            //! Set the time object.
            void setTimeObject(TimeObject*);

            //! Set the color configuration.
            void setColorConfig(const gl::ColorConfig&);

            //! Set the timeline player.
            void setTimelinePlayer(TimelinePlayer*);

        private:
            TLR_PRIVATE();
        };
    }
}
