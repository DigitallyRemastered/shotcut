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

#include "rhythm/beatgrid.h"
#include "rhythm/markergenerator.h"

#include <QSet>
#include <QtTest>

class TestMarkerGenerator : public QObject
{
    Q_OBJECT

private:
    static MarkerGenerator::Params make(
        double bpm, double offsetBeats, int lengthFrames, double fps, int everyNthBeat = 1)
    {
        MarkerGenerator::Params p;
        p.grid.bpm = bpm;
        p.grid.offsetBeats = offsetBeats;
        p.grid.lengthFrames = lengthFrames;
        p.grid.fps = fps;
        p.grid.everyNthBeat = everyNthBeat;
        return p;
    }

private slots:
    void gridPositionsFeedThroughToMarkers()
    {
        // BeatGrid is covered in its own test; here we only check the markers
        // land on exactly the frames it reports.
        const auto params = make(120.0, 0.0, 250, 25.0);
        const auto frames = BeatGrid::frames(params.grid);
        const auto markers = MarkerGenerator::generate(params);
        QCOMPARE(markers.size(), frames.size());
        for (int i = 0; i < markers.size(); i++)
            QCOMPARE(markers.at(i).start, frames.at(i));
    }

    void markersStayInRangeAndAscend()
    {
        const auto params = make(137.0, 0.25, 5000, 29.97);
        const auto markers = MarkerGenerator::generate(params);
        QVERIFY(!markers.isEmpty());
        for (int i = 0; i < markers.size(); i++) {
            QVERIFY(markers.at(i).start >= 0);
            QVERIFY(markers.at(i).start < params.grid.lengthFrames);
            // Point markers, not ranges.
            QCOMPARE(markers.at(i).start, markers.at(i).end);
            if (i > 0)
                QVERIFY(markers.at(i).start > markers.at(i - 1).start);
        }
    }

    void degenerateParamsProduceNothing_data()
    {
        QTest::addColumn<MarkerGenerator::Params>("params");
        QTest::newRow("zero length") << make(120.0, 0.0, 0, 25.0);
        QTest::newRow("zero bpm") << make(0.0, 0.0, 250, 25.0);
        QTest::newRow("negative bpm") << make(-5.0, 0.0, 250, 25.0);
        QTest::newRow("zero fps") << make(120.0, 0.0, 250, 0.0);
        QTest::newRow("zero stride") << make(120.0, 0.0, 250, 25.0, 0);
        QTest::newRow("offset past end") << make(120.0, 100.0, 250, 25.0);
    }

    void degenerateParamsProduceNothing()
    {
        QFETCH(MarkerGenerator::Params, params);
        // Must return empty promptly rather than looping.
        QCOMPARE(MarkerGenerator::count(params), 0);
        QVERIFY(MarkerGenerator::generate(params).isEmpty());
    }

    void rainbowRampsHueAcrossTheGrid()
    {
        auto params = make(120.0, 0.0, 250, 25.0);
        params.colorMode = MarkerGenerator::RainbowHue;
        const auto markers = MarkerGenerator::generate(params);
        QVERIFY(markers.size() > 2);
        // Hue is circular, so a ramp that ends on 1.0 wraps back to its own
        // starting red. Every marker must be a different colour.
        QSet<QRgb> seen;
        for (const auto &marker : markers) {
            QVERIFY(marker.color.isValid());
            seen.insert(marker.color.rgb());
        }
        QCOMPARE(seen.size(), markers.size());
        QVERIFY(markers.first().color != markers.last().color);
    }

    void fixedColorAppliesToEveryMarker()
    {
        auto params = make(120.0, 0.0, 250, 25.0);
        params.colorMode = MarkerGenerator::FixedColor;
        params.color = QColor(Qt::green);
        const auto markers = MarkerGenerator::generate(params);
        QVERIFY(!markers.isEmpty());
        for (const auto &marker : markers)
            QCOMPARE(marker.color, QColor(Qt::green));
    }

    void textUsesPrefixAndOneBasedIndex()
    {
        auto params = make(120.0, 0.0, 100, 25.0);
        params.textPrefix = QStringLiteral("Bar");
        const auto markers = MarkerGenerator::generate(params);
        QVERIFY(!markers.isEmpty());
        QCOMPARE(markers.first().text, QStringLiteral("Bar 1"));
        QCOMPARE(markers.last().text, QStringLiteral("Bar %1").arg(markers.size()));
    }
};

Q_DECLARE_METATYPE(MarkerGenerator::Params)

QTEST_GUILESS_MAIN(TestMarkerGenerator)

#include "test_marker_generator.moc"
