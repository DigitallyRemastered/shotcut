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

#include "pattern.h"

#include <algorithm>
#include <limits>

bool Pattern::isValid() const
{
    if (pulseFrames.isEmpty() || pulseFrames.size() != values.size())
        return false;
    for (int i = 1; i < pulseFrames.size(); i++) {
        if (pulseFrames.at(i) <= pulseFrames.at(i - 1))
            return false;
    }
    return true;
}

int Pattern::span() const
{
    if (pulseFrames.size() < 2)
        return 0;
    return pulseFrames.last() - pulseFrames.first();
}

namespace PatternExpander {

QVector<Keyframe> expand(const Pattern &pattern, const QVector<int> &times, int lengthFrames)
{
    QVector<Keyframe> result;
    if (!pattern.isValid() || times.isEmpty() || lengthFrames <= 0)
        return result;

    // Emission order matters: a later instant must be able to overwrite an
    // earlier one, so build the flat list first and resolve collisions after.
    struct Stamped
    {
        int frame;
        double value;
        int order;
    };
    QVector<Stamped> stamped;
    stamped.reserve(times.size() * pattern.pulseFrames.size());

    int order = 0;
    for (int time : times) {
        for (int i = 0; i < pattern.pulseFrames.size(); i++) {
            const qint64 frame = qint64(time) + pattern.pulseFrames.at(i);
            if (frame < 0 || frame >= lengthFrames)
                continue;
            stamped.append({int(frame), pattern.values.at(i), order++});
        }
    }
    if (stamped.isEmpty())
        return result;

    // Ascending by frame; for equal frames the later emission comes last.
    std::sort(stamped.begin(), stamped.end(), [](const Stamped &a, const Stamped &b) {
        return a.frame != b.frame ? a.frame < b.frame : a.order < b.order;
    });

    result.reserve(stamped.size());
    for (const auto &entry : stamped) {
        if (!result.isEmpty() && result.last().frame == entry.frame)
            result.last().value = entry.value; // later instant wins
        else
            result.append({entry.frame, entry.value});
    }
    return result;
}

bool overlaps(const Pattern &pattern, const QVector<int> &times)
{
    if (!pattern.isValid() || times.size() < 2)
        return false;

    int smallestGap = std::numeric_limits<int>::max();
    for (int i = 1; i < times.size(); i++)
        smallestGap = qMin(smallestGap, times.at(i) - times.at(i - 1));
    return pattern.span() > smallestGap;
}

} // namespace PatternExpander
