/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MARKERGENERATOR_H
#define MARKERGENERATOR_H

#include "models/markersmodel.h"

#include <QColor>
#include <QList>
#include <QString>

/*!
 * \brief Generates evenly spaced markers on a musical beat grid.
 *
 * Pure computation with no dependency on the timeline, the undo stack or any
 * widget, so it can be exercised on its own.
 */
class MarkerGenerator
{
public:
    enum ColorMode {
        RainbowHue, //!< Hue ramps 0..1 across the whole grid.
        FixedColor, //!< Every marker uses Params::color.
    };

    struct Params
    {
        double bpm{120.0};        //!< Beats per minute. Must be > 0.
        double offsetBeats{0.0};  //!< Grid start, in beats. May be fractional.
        int lengthFrames{0};      //!< Exclusive upper bound for marker frames.
        double fps{25.0};         //!< Timeline frame rate. Must be > 0.
        int everyNthBeat{1};      //!< 1 = every beat, 4 = every bar in 4/4.
        ColorMode colorMode{RainbowHue};
        QColor color{Qt::red};    //!< Used when colorMode == FixedColor.
        QString textPrefix{QStringLiteral("Beat")};
    };

    /*!
     * \brief Build the marker list for \a params.
     *
     * Returns an empty list rather than throwing when the parameters cannot
     * describe a grid (non-positive bpm, fps or length).
     */
    static QList<Markers::Marker> generate(const Params &params);

    /*!
     * \brief Number of markers generate() would produce, without building them.
     */
    static int count(const Params &params);

private:
    static QColor colorAt(const Params &params, int index, int total);
};

#endif // MARKERGENERATOR_H
