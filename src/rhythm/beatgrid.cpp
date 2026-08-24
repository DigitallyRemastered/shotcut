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

#include "beatgrid.h"

#include <QtGlobal>

#include <cmath>

namespace BeatGrid {

namespace {

//! Frames advanced per grid position. Positive whenever isUsable().
double framesPerStep(const Params &params)
{
    return (60.0 / params.bpm) * params.fps * double(params.everyNthBeat);
}

/*!
 * \brief Half-open index range [begin, end) of positions inside the timeline.
 *
 * count() and frames() both go through this, so they can never disagree about
 * how many positions there are - which they would if each filtered separately
 * and the grid started before frame zero.
 *
 * The bounds are solved arithmetically and then nudged, rather than found by
 * stepping, so an extreme tempo cannot spin here.
 */
bool indexRange(const Params &params, int &begin, int &end)
{
    begin = end = 0;
    if (!isUsable(params))
        return false;

    const double step = framesPerStep(params);
    if (step <= 0.0)
        return false;

    const double firstFrame = (params.offsetBeats * 60.0 / params.bpm) * params.fps;
    if (firstFrame >= double(params.lengthFrames))
        return false;

    // First index landing at or after frame 0. Only bites on a negative offset.
    begin = (firstFrame < 0.0) ? int(std::floor(-firstFrame / step)) : 0;
    while (begin > 0 && frameAt(params, begin - 1) >= 0)
        --begin;
    while (frameAt(params, begin) < 0)
        ++begin;

    // One past the last index still inside the timeline.
    end = begin + int(std::ceil((double(params.lengthFrames) - frameAt(params, begin)) / step)) + 1;
    while (end > begin && frameAt(params, end - 1) >= params.lengthFrames)
        --end;

    return end > begin;
}

/*!
 * \brief Visit every distinct in-range frame, in order, and return how many.
 *
 * count() and frames() share this so they cannot disagree.
 *
 * Two guards keep it bounded. When the grid is finer than one frame - a tempo
 * so high, or a frame rate so low, that consecutive beats round onto the same
 * frame - every frame in range carries a position, so it is emitted directly
 * instead of stepping through beats that would all collapse. Otherwise a step
 * is at least one frame, so the walk cannot run longer than the timeline.
 */
template<typename Sink>
int walk(const Params &params, Sink sink)
{
    int begin = 0;
    int end = 0;
    if (!indexRange(params, begin, end))
        return 0;

    const int firstFrame = qMax(0, frameAt(params, begin));
    if (framesPerStep(params) <= 1.0) {
        for (int frame = firstFrame; frame < params.lengthFrames; frame++)
            sink(frame);
        return params.lengthFrames - firstFrame;
    }

    int emitted = 0;
    int previous = -1;
    for (int i = begin; i < end; i++) {
        const int frame = frameAt(params, i);
        if (frame == previous)
            continue; // rounding collapsed two positions onto one frame
        previous = frame;
        sink(frame);
        ++emitted;
    }
    return emitted;
}

} // namespace

bool isUsable(const Params &params)
{
    return params.bpm > 0.0 && params.fps > 0.0 && params.lengthFrames > 0
           && params.everyNthBeat > 0;
}

int frameAt(const Params &params, int index)
{
    const double beat = params.offsetBeats + double(index) * double(params.everyNthBeat);
    const double seconds = beat * 60.0 / params.bpm;
    return int(std::llround(seconds * params.fps));
}

int count(const Params &params)
{
    return walk(params, [](int) {});
}

QVector<int> frames(const Params &params)
{
    QVector<int> result;
    walk(params, [&result](int frame) { result.append(frame); });
    return result;
}

} // namespace BeatGrid
