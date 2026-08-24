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

#include "keyframegenerator.h"

#include "qmltypes/qmlfilter.h"

#include <QObject>
#include <QRectF>

QVector<PatternExpander::Keyframe> KeyframeGenerator::preview(const Request &request)
{
    return PatternExpander::expand(request.pattern, request.times, request.lengthFrames);
}

int KeyframeGenerator::apply(QmlFilter *filter, const Request &request)
{
    if (!filter || request.property.isEmpty())
        return 0;

    const auto keyframes = preview(request);
    if (keyframes.isEmpty())
        return 0;

    // One undo entry for the whole batch, however many keyframes it writes.
    filter->startUndoParameterCommand(QObject::tr("Generate keyframes"));
    const int written = (request.valueKind == Rect) ? applyRect(filter, request, keyframes)
                                                    : applyScalar(filter, request, keyframes);
    filter->endUndoCommand();
    return written;
}

int KeyframeGenerator::applyScalar(QmlFilter *filter,
                                   const Request &request,
                                   const QVector<PatternExpander::Keyframe> &keyframes)
{
    const auto type = mlt_keyframe_type(request.pattern.interpolation);
    int written = 0;
    for (const auto &keyframe : keyframes) {
        double value = keyframe.value;
        if (request.mode == RelativeToExisting) {
            // getDouble evaluates the existing animation at this frame, so the
            // pulse multiplies whatever the parameter was already doing.
            value *= filter->getDouble(request.property, keyframe.frame);
        }
        filter->set(request.property, value, keyframe.frame, type);
        ++written;
    }
    return written;
}

int KeyframeGenerator::applyRect(QmlFilter *filter,
                                 const Request &request,
                                 const QVector<PatternExpander::Keyframe> &keyframes)
{
    const auto type = mlt_keyframe_type(request.pattern.interpolation);

    // Sample the existing animation before writing anything. Reading as we go
    // would let each new keyframe change what the next one samples, so a pulse
    // would compound instead of repeating.
    QVector<QRectF> sampled;
    sampled.reserve(keyframes.size());
    for (const auto &keyframe : keyframes)
        sampled.append(filter->getRect(request.property, keyframe.frame));

    int written = 0;
    for (int i = 0; i < keyframes.size(); i++) {
        const QRectF &base = sampled.at(i);
        if (base.isEmpty())
            continue; // nothing sensible to scale

        QRectF rect = base;
        if (request.mode == RelativeToExisting) {
            // Scale about the centre so the framing stays put and only the
            // size pulses. This replaces the centroid arithmetic the MATLAB
            // tool did by hand; for an axis-aligned rectangle the centroid is
            // simply the centre.
            const double factor = keyframes.at(i).value;
            const QPointF centre = base.center();
            rect.setWidth(base.width() * factor);
            rect.setHeight(base.height() * factor);
            rect.moveCenter(centre);
        } else {
            rect.setWidth(keyframes.at(i).value);
            rect.setHeight(keyframes.at(i).value);
            rect.moveCenter(base.center());
        }

        filter->set(request.property,
                    rect.x(),
                    rect.y(),
                    rect.width(),
                    rect.height(),
                    1.0,
                    keyframes.at(i).frame,
                    type);
        ++written;
    }
    return written;
}
