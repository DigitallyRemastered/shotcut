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

#include <QtTest>

Q_DECLARE_METATYPE(BeatGrid::Params)

class TestBeatGrid : public QObject
{
    Q_OBJECT

private:
    static BeatGrid::Params make(
        double bpm, double offsetBeats, int lengthFrames, double fps, int everyNthBeat = 1)
    {
        BeatGrid::Params p;
        p.bpm = bpm;
        p.offsetBeats = offsetBeats;
        p.lengthFrames = lengthFrames;
        p.fps = fps;
        p.everyNthBeat = everyNthBeat;
        return p;
    }

private slots:
    void positionsAreCorrect_data()
    {
        QTest::addColumn<BeatGrid::Params>("params");
        QTest::addColumn<int>("expectedCount");
        QTest::addColumn<int>("expectedFirst");
        QTest::addColumn<int>("expectedLast");

        // 12.5 frames per beat: positions alternate between whole and half
        // frames, so rounding has to be consistent to stay evenly spaced.
        QTest::newRow("120bpm 25fps") << make(120, 0, 250, 25) << 20 << 0 << 238;
        // Exactly 30 frames per beat, no rounding involved.
        QTest::newRow("60bpm 30fps") << make(60, 0, 300, 30) << 10 << 0 << 270;
        QTest::newRow("bars in 4/4") << make(120, 0, 250, 25, 4) << 5 << 0 << 200;
        QTest::newRow("fractional offset") << make(120, 0.5, 250, 25) << 20 << 6 << 244;
        QTest::newRow("ntsc 128bpm") << make(128, 0, 1800, 29.97) << 129 << 0 << 1798;
        QTest::newRow("fast 400bpm") << make(400, 0, 600, 60) << 67 << 0 << 594;
        QTest::newRow("single frame") << make(120, 0, 1, 25) << 1 << 0 << 0;
    }

    void positionsAreCorrect()
    {
        QFETCH(BeatGrid::Params, params);
        QFETCH(int, expectedCount);
        QFETCH(int, expectedFirst);
        QFETCH(int, expectedLast);

        const auto frames = BeatGrid::frames(params);
        QCOMPARE(BeatGrid::count(params), expectedCount);
        QCOMPARE(frames.size(), expectedCount);
        QCOMPARE(frames.first(), expectedFirst);
        QCOMPARE(frames.last(), expectedLast);
    }

    void framesAreInRangeAndAscending()
    {
        const auto params = make(137, 0.25, 5000, 29.97);
        const auto frames = BeatGrid::frames(params);
        QVERIFY(!frames.isEmpty());
        for (int i = 0; i < frames.size(); i++) {
            QVERIFY(frames.at(i) >= 0);
            QVERIFY(frames.at(i) < params.lengthFrames);
            if (i > 0)
                QVERIFY(frames.at(i) > frames.at(i - 1));
        }
    }

    void spacingStaysEven()
    {
        // With 12.5 frames per beat the gaps must alternate 12/13, never drift.
        const auto frames = BeatGrid::frames(make(120, 0, 250, 25));
        for (int i = 1; i < frames.size(); i++) {
            const int gap = frames.at(i) - frames.at(i - 1);
            QVERIFY2(gap == 12 || gap == 13, qPrintable(QStringLiteral("gap %1").arg(gap)));
        }
        // Cumulative position must not drift from the ideal.
        for (int i = 0; i < frames.size(); i++)
            QVERIFY(qAbs(frames.at(i) - i * 12.5) <= 0.5);
    }

    void degenerateParamsYieldNothing_data()
    {
        QTest::addColumn<BeatGrid::Params>("params");
        QTest::newRow("zero length") << make(120, 0, 0, 25);
        QTest::newRow("negative length") << make(120, 0, -10, 25);
        QTest::newRow("zero bpm") << make(0, 0, 250, 25);
        QTest::newRow("negative bpm") << make(-5, 0, 250, 25);
        QTest::newRow("zero fps") << make(120, 0, 250, 0);
        QTest::newRow("zero stride") << make(120, 0, 250, 25, 0);
        QTest::newRow("negative stride") << make(120, 0, 250, 25, -4);
        QTest::newRow("offset past end") << make(120, 100, 250, 25);
    }

    void degenerateParamsYieldNothing()
    {
        QFETCH(BeatGrid::Params, params);
        // Must return promptly rather than loop.
        QCOMPARE(BeatGrid::count(params), 0);
        QVERIFY(BeatGrid::frames(params).isEmpty());
    }

    void negativeOffsetSkipsPositionsBeforeZero()
    {
        // The grid starts before the timeline; only in-range frames survive.
        const auto params = make(120, -2.0, 250, 25);
        const auto frames = BeatGrid::frames(params);
        QVERIFY(!frames.isEmpty());
        for (int frame : frames)
            QVERIFY(frame >= 0);
    }


    void gridFinerThanAFrameCollapsesToDistinctFrames()
    {
        // 354 BPM at 1.14 fps is well under one frame per beat, so consecutive
        // beats round onto the same frame. Duplicate positions would be invalid
        // as markers or keyframes, so they must collapse.
        const auto params = make(354.17, 0, 200, 1.14, 2);
        const auto frames = BeatGrid::frames(params);
        QCOMPARE(BeatGrid::count(params), frames.size());
        for (int i = 1; i < frames.size(); i++)
            QVERIFY(frames.at(i) > frames.at(i - 1));
        for (int frame : frames)
            QVERIFY(frame >= 0 && frame < params.lengthFrames);
    }

    void countAlwaysMatchesFramesSize_data()
    {
        QTest::addColumn<BeatGrid::Params>("params");
        QTest::newRow("ordinary") << make(120, 0, 250, 25);
        QTest::newRow("negative offset") << make(120, -2.0, 250, 25);
        QTest::newRow("large negative offset") << make(120, -1000.0, 250, 25);
        QTest::newRow("sub-frame grid") << make(354.17, 54.84, 16510, 1.14, 2);
        QTest::newRow("very slow fps") << make(200, 0, 500, 2.0);
        QTest::newRow("wide stride") << make(90, 0.75, 4000, 60, 16);
    }

    void countAlwaysMatchesFramesSize()
    {
        QFETCH(BeatGrid::Params, params);
        // The two must never disagree; they previously did when the grid
        // started before frame zero.
        QCOMPARE(BeatGrid::count(params), BeatGrid::frames(params).size());
    }

    void isUsableMatchesCount()
    {
        QVERIFY(BeatGrid::isUsable(make(120, 0, 250, 25)));
        QVERIFY(!BeatGrid::isUsable(make(0, 0, 250, 25)));
        QVERIFY(!BeatGrid::isUsable(make(120, 0, 0, 25)));
    }
};

QTEST_GUILESS_MAIN(TestBeatGrid)

#include "test_beat_grid.moc"
