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

#include "rhythm/beatgrid.h"

#include <QObject>
#include <algorithm>

int MarkerGenerator::count(const Params &params)
{
    return BeatGrid::count(params.grid);
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
    const QVector<int> positions = BeatGrid::frames(params.grid);
    const int total = positions.size();
    if (total <= 0)
        return markers;

    markers.reserve(total);
    for (int i = 0; i < total; i++) {
        const int frame = positions.at(i);
        Markers::Marker marker;
        marker.text = QStringLiteral("%1 %2").arg(params.textPrefix).arg(i + 1);
        marker.start = frame;
        marker.end = frame; // A point marker, not a range.
        marker.color = colorAt(params, i, total);
        markers << marker;
    }
    return markers;
}
