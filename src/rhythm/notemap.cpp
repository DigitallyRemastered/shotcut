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

#include "notemap.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QObject>
#include <algorithm>

void NoteMap::clear()
{
    m_name.clear();
    m_sourceClip.clear();
    m_voices = 1;
    m_entries.clear();
}

QString NoteMap::trackName(const QString &base, int voice)
{
    // Voice 0 keeps the bare name so a single-voice map reads naturally.
    if (voice <= 0)
        return base;
    return base + QChar('a' + char(voice % 26));
}

bool NoteMap::load(const QString &path, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        clear();
        if (errorMessage)
            *errorMessage = QObject::tr("Unable to open %1").arg(path);
        return false;
    }
    if (!loadJson(file.readAll(), errorMessage))
        return false;

    // A relative clip path is resolved against the map file, so a map and its
    // media can be moved together.
    if (!m_sourceClip.isEmpty() && QFileInfo(m_sourceClip).isRelative())
        m_sourceClip = QDir(QFileInfo(path).absolutePath()).absoluteFilePath(m_sourceClip);
    return true;
}

bool NoteMap::loadJson(const QByteArray &json, QString *errorMessage)
{
    clear();
    const auto fail = [&](const QString &why) {
        clear();
        if (errorMessage)
            *errorMessage = why;
        return false;
    };

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return fail(QObject::tr("Invalid JSON: %1").arg(parseError.errorString()));
    if (!document.isObject())
        return fail(QObject::tr("Note map must be a JSON object"));

    const QJsonObject root = document.object();
    m_name = root.value("name").toString();
    m_sourceClip = root.value("sourceClip").toString();
    if (m_sourceClip.isEmpty())
        return fail(QObject::tr("Note map has no sourceClip"));

    m_voices = root.value("voices").toInt(1);
    if (m_voices < 1)
        m_voices = 1;

    const QJsonValue notesValue = root.value("notes");
    if (!notesValue.isArray())
        return fail(QObject::tr("Note map has no notes array"));

    const QJsonArray notes = notesValue.toArray();
    for (const QJsonValue &value : notes) {
        if (!value.isObject())
            return fail(QObject::tr("Each note must be a JSON object"));
        const QJsonObject object = value.toObject();

        Entry entry;
        entry.note = object.value("note").toInt(-1);
        if (entry.note < 0 || entry.note > 127)
            return fail(QObject::tr("Note number must be 0 to 127"));

        entry.track = object.value("track").toString();
        if (entry.track.isEmpty())
            return fail(QObject::tr("Note %1 has no track name").arg(entry.note));

        const QJsonArray rect = object.value("rect").toArray();
        if (rect.size() < 4)
            return fail(QObject::tr("Note %1 needs a rect of x, y, width, height")
                            .arg(entry.note));
        entry.rect = QRectF(rect.at(0).toDouble(),
                            rect.at(1).toDouble(),
                            rect.at(2).toDouble(),
                            rect.at(3).toDouble());
        if (entry.rect.width() <= 0.0 || entry.rect.height() <= 0.0)
            return fail(QObject::tr("Note %1 has an empty rect").arg(entry.note));

        // Opacity may be the fifth rect element, matching MLT's rect syntax, or
        // its own field.
        entry.opacity = (rect.size() > 4) ? rect.at(4).toDouble(1.0)
                                          : object.value("opacity").toDouble(1.0);

        if (m_entries.contains(entry.note))
            return fail(QObject::tr("Note %1 appears twice").arg(entry.note));
        m_entries.insert(entry.note, entry);
    }

    if (m_entries.isEmpty())
        return fail(QObject::tr("Note map contains no notes"));
    return true;
}

const NoteMap::Entry *NoteMap::entry(int note) const
{
    const auto it = m_entries.constFind(note);
    return (it == m_entries.constEnd()) ? nullptr : &it.value();
}

QList<int> NoteMap::notes() const
{
    QList<int> result = m_entries.keys();
    std::sort(result.begin(), result.end());
    return result;
}
