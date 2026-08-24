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

#ifndef CLIPPLACER_H
#define CLIPPLACER_H

#include "rhythm/notemap.h"

#include <QList>
#include <QRectF>
#include <QString>
#include <QVector>

class MidiSource;

/*!
 * \brief Places a struck-note clip on the timeline for every MIDI note.
 *
 * Turns a performance into an animation of a physical instrument: each note
 * gets a copy of the same clip, positioned over that note's key by the note
 * map, at the moment the note sounds.
 *
 * Planning is separated from applying. plan() is pure arithmetic over the MIDI
 * and the map, so the whole placement can be computed, counted and tested
 * without a timeline; apply() only walks the plan.
 */
class ClipPlacer
{
public:
    struct Request
    {
        const MidiSource *midi{nullptr};
        int channel{-1};        //!< -1 for every channel.
        double fps{25.0};
        int lengthFrames{0};    //!< Exclusive upper bound.
        int clipFrames{0};      //!< Length of one strike of the source clip.
    };

    struct Placement
    {
        int note{0};
        QString track;          //!< Full per-voice track name.
        int voice{0};
        int frame{0};           //!< Where the clip starts.
        int durationFrames{0};
        QRectF rect;
        double opacity{1.0};
    };

    struct Plan
    {
        QVector<Placement> placements;
        QList<int> unmappedNotes;   //!< Sounded, but absent from the map.
        int skippedOutOfRange{0};   //!< Notes landing past the end of the timeline.

        //! Distinct track names the plan needs, in first-use order.
        QStringList tracks() const;
        bool isEmpty() const { return placements.isEmpty(); }
    };

    /*!
     * \brief Work out every placement without touching the timeline.
     *
     * Repeats of the same note round-robin across the map's voices, so a note
     * struck again before its previous clip has finished lands on a different
     * track instead of overwriting itself.
     */
    static Plan plan(const NoteMap &map, const Request &request);

private:
    static int frameFor(double seconds, double fps);
};

#endif // CLIPPLACER_H
