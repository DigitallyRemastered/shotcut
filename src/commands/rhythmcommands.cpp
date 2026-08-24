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

#include "rhythmcommands.h"

#include "Logger.h"
#include "commands/timelinecommands.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/multitrackmodel.h"

#include <QFileInfo>
#include <QHash>
#include <QStringList>
#include <QUndoStack>

namespace Rhythm {

namespace {

//! Index of the video track called \a name, or -1.
int findTrack(MultitrackModel &model, const QString &name)
{
    for (int i = 0; i < model.trackList().size(); i++) {
        if (model.getTrackName(i) == name)
            return i;
    }
    return -1;
}

QStringList trackNames(MultitrackModel &model)
{
    QStringList names;
    names.reserve(model.trackList().size());
    for (int i = 0; i < model.trackList().size(); i++)
        names << model.getTrackName(i);
    return names;
}

/*!
 * \brief Index of the track a just-executed AddTrackCommand created.
 *
 * A new track is auto-named by Shotcut, so it cannot be found by looking for a
 * blank name. Exactly one track was inserted, so the first position where the
 * before and after name lists diverge is where it landed.
 */
int insertedTrackIndex(const QStringList &before, const QStringList &after)
{
    for (int i = 0; i < after.size(); i++) {
        if (i >= before.size() || after.at(i) != before.at(i))
            return i;
    }
    return -1;
}

//! MLT rect syntax: "x y w h opacity".
QString rectString(const QRectF &rect, double opacity)
{
    return QStringLiteral("%1 %2 %3 %4 %5")
        .arg(rect.x())
        .arg(rect.y())
        .arg(rect.width())
        .arg(rect.height())
        .arg(opacity);
}

} // namespace

int placeClips(MultitrackModel &model,
               const NoteMap &map,
               const ClipPlacer::Plan &plan,
               QString *errorMessage)
{
    const auto fail = [&](const QString &why) {
        LOG_ERROR() << why;
        if (errorMessage)
            *errorMessage = why;
        return 0;
    };

    if (plan.isEmpty())
        return fail(QObject::tr("Nothing to place"));
    if (!QFileInfo::exists(map.sourceClip()))
        return fail(QObject::tr("Clip not found: %1").arg(map.sourceClip()));

    // Build the clip XML once per note, since every strike of a note is the
    // same producer framed the same way; only the position differs.
    QHash<int, QString> xmlForNote;

    MAIN.undoStack()->beginMacro(QObject::tr("Place clips from MIDI"));

    // Tracks first: adding one shifts the indices of the others, so resolve
    // every name to an index only after they all exist.
    QHash<QString, int> trackIndex;
    for (const QString &name : plan.tracks()) {
        if (findTrack(model, name) >= 0)
            continue;
        const QStringList before = trackNames(model);
        MAIN.undoStack()->push(new Timeline::AddTrackCommand(model, true /* video */));
        const int added = insertedTrackIndex(before, trackNames(model));
        if (added < 0) {
            MAIN.undoStack()->endMacro();
            return fail(QObject::tr("Unable to add a track for %1").arg(name));
        }
        MAIN.undoStack()->push(new Timeline::NameTrackCommand(model, added, name));
    }
    for (const QString &name : plan.tracks())
        trackIndex.insert(name, findTrack(model, name));

    int placed = 0;
    for (const auto &placement : plan.placements) {
        const int track = trackIndex.value(placement.track, -1);
        if (track < 0)
            continue;

        if (!xmlForNote.contains(placement.note)) {
            Mlt::Producer clip(MLT.profile(), qUtf8Printable(map.sourceClip()));
            if (!clip.is_valid()) {
                MAIN.undoStack()->endMacro();
                return fail(QObject::tr("Unable to open %1").arg(map.sourceClip()));
            }
            clip.set_in_and_out(0, qMax(0, placement.durationFrames - 1));

            // Frame the strike over this note's key.
            Mlt::Filter affine(MLT.profile(), "affine");
            if (affine.is_valid()) {
                affine.set("transition.rect",
                           qUtf8Printable(rectString(placement.rect, placement.opacity)));
                affine.set("transition.fill", 1);
                affine.set("transition.distort", 0);
                affine.set("shotcut:filter", "affineSizePosition");
                clip.attach(affine);
            }
            xmlForNote.insert(placement.note, MLT.XML(&clip));
        }

        MAIN.undoStack()->push(new Timeline::OverwriteCommand(model,
                                                              track,
                                                              placement.frame,
                                                              xmlForNote.value(placement.note),
                                                              false /* seek */));
        ++placed;
    }

    MAIN.undoStack()->endMacro();
    LOG_INFO() << "placed" << placed << "clips on" << plan.tracks().size() << "tracks";
    return placed;
}

} // namespace Rhythm
