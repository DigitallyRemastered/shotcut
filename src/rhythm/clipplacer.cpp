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

#include "clipplacer.h"

#include "rhythm/midisource.h"

#include <QHash>
#include <QSet>
#include <algorithm>
#include <cmath>

QStringList ClipPlacer::Plan::tracks() const
{
    QStringList result;
    QSet<QString> seen;
    for (const auto &placement : placements) {
        if (!seen.contains(placement.track)) {
            seen.insert(placement.track);
            result << placement.track;
        }
    }
    return result;
}

int ClipPlacer::frameFor(double seconds, double fps)
{
    return int(std::llround(seconds * fps));
}

ClipPlacer::Plan ClipPlacer::plan(const NoteMap &map, const Request &request)
{
    Plan result;
    if (!map.isValid() || !request.midi || !request.midi->isValid())
        return result;
    if (request.fps <= 0.0 || request.lengthFrames <= 0 || request.clipFrames <= 0)
        return result;

    // How many times each note has already sounded, to spread repeats across
    // the available voices.
    QHash<int, int> hitCount;
    QSet<int> unmapped;

    for (const auto &note : request.midi->notes()) {
        if (request.channel >= 0 && note.channel != request.channel)
            continue;

        const NoteMap::Entry *entry = map.entry(note.note);
        if (!entry) {
            unmapped.insert(note.note);
            continue;
        }

        const int frame = frameFor(note.seconds, request.fps);
        if (frame < 0 || frame >= request.lengthFrames) {
            result.skippedOutOfRange++;
            continue;
        }

        const int hit = hitCount.value(note.note, 0);
        hitCount.insert(note.note, hit + 1);
        const int voice = (map.voices() > 0) ? hit % map.voices() : 0;

        Placement placement;
        placement.note = note.note;
        placement.voice = voice;
        placement.track = NoteMap::trackName(entry->track, voice);
        placement.frame = frame;
        // Trim a strike that would overhang the end rather than dropping it.
        placement.durationFrames = qMin(request.clipFrames, request.lengthFrames - frame);
        placement.rect = entry->rect;
        placement.opacity = entry->opacity;
        result.placements.append(placement);
    }

    result.unmappedNotes = QList<int>(unmapped.cbegin(), unmapped.cend());
    std::sort(result.unmappedNotes.begin(), result.unmappedNotes.end());

    // Notes arrive in time order already; keep placements that way so applying
    // the plan walks the timeline forwards.
    std::stable_sort(result.placements.begin(),
                     result.placements.end(),
                     [](const Placement &a, const Placement &b) { return a.frame < b.frame; });
    return result;
}
