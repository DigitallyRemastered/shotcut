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

#include "markergenerator.h"

#include <QObject>
#include <cmath>

namespace {

// Guard against parameters that cannot describe a grid at all.
bool isUsable(const MarkerGenerator::Params &p)
{
    return p.bpm > 0.0 && p.fps > 0.0 && p.lengthFrames > 0 && p.everyNthBeat > 0;
}

// Frame number of the nth generated marker. Beats are converted to frames only
// here, so rounding happens exactly once per marker.
int frameForIndex(const MarkerGenerator::Params &p, int index)
{
    const double beat = p.offsetBeats + double(index) * double(p.everyNthBeat);
    const double seconds = beat * 60.0 / p.bpm;
    return int(std::llround(seconds * p.fps));
}

} // namespace

int MarkerGenerator::count(const Params &params)
{
    if (!isUsable(params))
        return 0;

    // Solve for the first index whose frame lands at or past the end, rather
    // than looping, so a silly bpm cannot spin here.
    const double framesPerStep = (60.0 / params.bpm) * params.fps * double(params.everyNthBeat);
    if (framesPerStep <= 0.0)
        return 0;
    const double firstFrame = (params.offsetBeats * 60.0 / params.bpm) * params.fps;
    if (firstFrame >= double(params.lengthFrames))
        return 0;
    const double span = double(params.lengthFrames) - firstFrame;
    int n = int(std::ceil(span / framesPerStep));

    // Rounding in frameForIndex can put the last one a frame over; trim it.
    while (n > 0 && frameForIndex(params, n - 1) >= params.lengthFrames)
        --n;
    return n < 0 ? 0 : n;
}

QColor MarkerGenerator::colorAt(const Params &params, int index, int total)
{
    if (params.colorMode == FixedColor)
        return params.color;

    // Ramp hue across the whole grid. Saturation and lightness match the values
    // the MATLAB tool used, which read well against the timeline background.
    const double hue = (total > 1) ? double(index) / double(total - 1) : 0.0;
    return QColor::fromHslF(std::clamp(hue, 0.0, 1.0), 0.7, 0.75);
}

QList<Markers::Marker> MarkerGenerator::generate(const Params &params)
{
    QList<Markers::Marker> markers;
    const int total = count(params);
    if (total <= 0)
        return markers;

    markers.reserve(total);
    for (int i = 0; i < total; i++) {
        const int frame = frameForIndex(params, i);
        if (frame < 0 || frame >= params.lengthFrames)
            continue;
        Markers::Marker marker;
        marker.text = QStringLiteral("%1 %2").arg(params.textPrefix).arg(i + 1);
        marker.start = frame;
        marker.end = frame; // A point marker, not a range.
        marker.color = colorAt(params, i, total);
        markers << marker;
    }
    return markers;
}
