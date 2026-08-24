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

#include "rhythm/markergenerator.h"

#include <QtTest>

class TestMarkerGenerator : public QObject
{
    Q_OBJECT

private:
    static MarkerGenerator::Params make(
        double bpm, double offsetBeats, int lengthFrames, double fps, int everyNthBeat = 1)
    {
        MarkerGenerator::Params p;
        p.bpm = bpm;
        p.offsetBeats = offsetBeats;
        p.lengthFrames = lengthFrames;
        p.fps = fps;
        p.everyNthBeat = everyNthBeat;
        return p;
    }

private slots:
    void gridIsEvenlySpaced_data()
    {
        QTest::addColumn<double>("bpm");
        QTest::addColumn<double>("offset");
        QTest::addColumn<int>("length");
        QTest::addColumn<double>("fps");
        QTest::addColumn<int>("everyNth");
        QTest::addColumn<int>("expectedCount");
        QTest::addColumn<int>("expectedFirst");
        QTest::addColumn<int>("expectedLast");

        // 120 BPM at 25 fps is 12.5 frames per beat, so frames alternate
        // between exact and half values and must round consistently.
        QTest::newRow("120bpm 25fps") << 120.0 << 0.0 << 250 << 25.0 << 1 << 20 << 0 << 238;
        QTest::newRow("60bpm 30fps exact") << 60.0 << 0.0 << 300 << 30.0 << 1 << 10 << 0 << 270;
        QTest::newRow("bars in 4/4") << 120.0 << 0.0 << 250 << 25.0 << 4 << 5 << 0 << 200;
        QTest::newRow("fractional offset") << 120.0 << 0.5 << 250 << 25.0 << 1 << 20 << 6 << 244;
        QTest::newRow("ntsc 128bpm") << 128.0 << 0.0 << 1800 << 29.97 << 1 << 129 << 0 << 1798;
        QTest::newRow("single frame") << 120.0 << 0.0 << 1 << 25.0 << 1 << 1 << 0 << 0;
    }

    void gridIsEvenlySpaced()
    {
        QFETCH(double, bpm);
        QFETCH(double, offset);
        QFETCH(int, length);
        QFETCH(double, fps);
        QFETCH(int, everyNth);
        QFETCH(int, expectedCount);
        QFETCH(int, expectedFirst);
        QFETCH(int, expectedLast);

        const auto params = make(bpm, offset, length, fps, everyNth);
        const auto markers = MarkerGenerator::generate(params);

        QCOMPARE(MarkerGenerator::count(params), expectedCount);
        QCOMPARE(markers.size(), expectedCount);
        QCOMPARE(markers.first().start, expectedFirst);
        QCOMPARE(markers.last().start, expectedLast);
    }

    void markersStayInRangeAndAscend()
    {
        const auto params = make(137.0, 0.25, 5000, 29.97);
        const auto markers = MarkerGenerator::generate(params);
        QVERIFY(!markers.isEmpty());
        for (int i = 0; i < markers.size(); i++) {
            QVERIFY(markers.at(i).start >= 0);
            QVERIFY(markers.at(i).start < params.lengthFrames);
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
        QVERIFY(markers.first().color.isValid());
        QVERIFY(markers.last().color.isValid());
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
