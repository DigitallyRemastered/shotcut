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

#ifndef KEYFRAMEGENERATOR_H
#define KEYFRAMEGENERATOR_H

#include "rhythm/pattern.h"

#include <QString>
#include <QVector>

class QmlFilter;

/*!
 * \brief Writes a rhythmic pattern onto one filter parameter as keyframes.
 *
 * Deliberately knows nothing about where the instants came from or how the
 * filter got attached; it is handed a filter, a parameter name and a list of
 * keyframes to write.
 */
class KeyframeGenerator
{
public:
    enum Mode {
        /*! Write the pattern's values straight onto the parameter. Suits hue,
         *  saturation, brightness, rotation - anything with an absolute value. */
        Absolute,
        /*! Treat each value as a multiplier on whatever the parameter already
         *  animates to at that frame. This is what makes a zoom pulse ride on
         *  top of a pan the user keyframed by hand, instead of flattening it. */
        RelativeToExisting,
    };

    /*! Which flavour of MLT property is being written. */
    enum ValueKind {
        Scalar, //!< A plain number.
        Rect,   //!< An "x y w h opacity" rectangle, scaled about its centre.
    };

    struct Request
    {
        QString property;             //!< MLT property name, e.g. "transition.rect".
        Pattern pattern;
        QVector<int> times;           //!< Instants, in frames.
        Mode mode{Absolute};
        ValueKind valueKind{Scalar};
        int lengthFrames{0};          //!< Exclusive upper bound for keyframe positions.
    };

    /*!
     * \brief Apply \a request to \a filter as a single undoable change.
     * \return how many keyframes were written; 0 if nothing applied.
     *
     * Wraps the whole batch in one undo command, so a generation that writes
     * hundreds of keyframes is undone by one Ctrl+Z rather than hundreds.
     */
    static int apply(QmlFilter *filter, const Request &request);

    //! Keyframes \a request would write, without touching the filter.
    static QVector<PatternExpander::Keyframe> preview(const Request &request);

private:
    static int applyScalar(QmlFilter *filter,
                           const Request &request,
                           const QVector<PatternExpander::Keyframe> &keyframes);
    static int applyRect(QmlFilter *filter,
                         const Request &request,
                         const QVector<PatternExpander::Keyframe> &keyframes);
};

#endif // KEYFRAMEGENERATOR_H
