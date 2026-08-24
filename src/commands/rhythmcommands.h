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

#ifndef RHYTHMCOMMANDS_H
#define RHYTHMCOMMANDS_H

#include "rhythm/clipplacer.h"
#include "rhythm/notemap.h"

#include <QString>

class MultitrackModel;

namespace Rhythm {

/*!
 * \brief Apply a clip placement plan to the timeline.
 * \return the number of clips placed; 0 on failure.
 *
 * Creates any per-note video tracks the plan needs, then overwrites a copy of
 * the map's clip at each planned position with an affine filter framing it over
 * that note's key.
 *
 * The whole thing is one undo stack macro, so a plan that places hundreds of
 * clips and adds dozens of tracks is undone by a single Ctrl+Z.
 *
 * This lives apart from ClipPlacer so that planning stays free of the timeline
 * and remains unit testable.
 */
int placeClips(MultitrackModel &model,
               const NoteMap &map,
               const ClipPlacer::Plan &plan,
               QString *errorMessage = nullptr);

} // namespace Rhythm

#endif // RHYTHMCOMMANDS_H
