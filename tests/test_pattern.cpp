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

#include "rhythm/pattern.h"

#include <QtTest>

class TestPattern : public QObject
{
    Q_OBJECT

private:
    //! The pulse the MATLAB tool defaulted to: jump up, settle back.
    static Pattern pulse()
    {
        Pattern p;
        p.pulseFrames = {0, 4, 6};
        p.values = {1.0, 2.0, 1.0};
        return p;
    }

private slots:
    void validityRules()
    {
        QVERIFY(pulse().isValid());

        Pattern empty;
        QVERIFY(!empty.isValid());

        Pattern mismatched;
        mismatched.pulseFrames = {0, 4};
        mismatched.values = {1.0};
        QVERIFY(!mismatched.isValid());

        Pattern unsorted;
        unsorted.pulseFrames = {0, 6, 4};
        unsorted.values = {1.0, 2.0, 1.0};
        QVERIFY(!unsorted.isValid());

        Pattern repeated;
        repeated.pulseFrames = {0, 4, 4};
        repeated.values = {1.0, 2.0, 1.0};
        QVERIFY(!repeated.isValid());
    }

    void spanIsFirstToLast()
    {
        QCOMPARE(pulse().span(), 6);
        Pattern single;
        single.pulseFrames = {0};
        single.values = {1.0};
        QCOMPARE(single.span(), 0);
    }

    void stampsPatternAtEachInstant()
    {
        const auto result = PatternExpander::expand(pulse(), {100, 200}, 1000);
        QCOMPARE(result.size(), 6);
        QCOMPARE(result.at(0).frame, 100);
        QCOMPARE(result.at(1).frame, 104);
        QCOMPARE(result.at(2).frame, 106);
        QCOMPARE(result.at(3).frame, 200);
        QCOMPARE(result.at(4).frame, 204);
        QCOMPARE(result.at(5).frame, 206);
        QCOMPARE(result.at(1).value, 2.0);
    }

    void dropsFramesOutsideTheRange()
    {
        // The pulse tail runs past the end; only what fits survives, and
        // nothing is clamped onto the boundary.
        const auto result = PatternExpander::expand(pulse(), {98}, 103);
        QCOMPARE(result.size(), 2);
        QCOMPARE(result.at(0).frame, 98);
        QCOMPARE(result.at(1).frame, 102);
    }

    void dropsNegativeFrames()
    {
        Pattern leading;
        leading.pulseFrames = {-6, 0};
        leading.values = {1.0, 2.0};
        const auto result = PatternExpander::expand(leading, {2}, 1000);
        QCOMPARE(result.size(), 1);
        QCOMPARE(result.at(0).frame, 2);
    }

    void laterInstantWinsOnOverlap()
    {
        // Instants 4 apart with a 6-frame pulse: frame 4 is written by the
        // first instant's offset 4 and the second instant's offset 0. The
        // second must win so the rhythm stays audible.
        const auto result = PatternExpander::expand(pulse(), {0, 4}, 1000);
        for (int i = 1; i < result.size(); i++)
            QVERIFY(result.at(i).frame > result.at(i - 1).frame);

        bool found = false;
        for (const auto &keyframe : result) {
            if (keyframe.frame == 4) {
                QCOMPARE(keyframe.value, 1.0); // offset 0 of the second instant
                found = true;
            }
        }
        QVERIFY(found);
    }

    void resultIsAscendingAndDistinct()
    {
        QVector<int> times;
        for (int i = 0; i < 200; i++)
            times << i * 5; // deliberately tighter than the 6-frame pulse
        const auto result = PatternExpander::expand(pulse(), times, 2000);
        QVERIFY(!result.isEmpty());
        for (int i = 1; i < result.size(); i++)
            QVERIFY(result.at(i).frame > result.at(i - 1).frame);
    }

    void emptyInputsYieldNothing()
    {
        QVERIFY(PatternExpander::expand(pulse(), {}, 1000).isEmpty());
        QVERIFY(PatternExpander::expand(Pattern(), {100}, 1000).isEmpty());
        QVERIFY(PatternExpander::expand(pulse(), {100}, 0).isEmpty());
    }

    void detectsOverlap()
    {
        QVERIFY(!PatternExpander::overlaps(pulse(), {0, 100, 200}));
        QVERIFY(PatternExpander::overlaps(pulse(), {0, 4, 8}));
        // Fewer than two instants cannot overlap.
        QVERIFY(!PatternExpander::overlaps(pulse(), {0}));
    }
};

QTEST_GUILESS_MAIN(TestPattern)

#include "test_pattern.moc"
