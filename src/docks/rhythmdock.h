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

#ifndef RHYTHMDOCK_H
#define RHYTHMDOCK_H

#include "rhythm/midisource.h"
#include "rhythm/pattern.h"
#include "rhythm/rhythmsource.h"

#include <QDockWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QmlFilter;
class QmlMetadata;

/*!
 * \brief Drives a filter parameter from a musical rhythm.
 *
 * Combines a time source (tempo grid or MIDI onsets) with a repeating value
 * pattern, and writes the result onto a parameter of the filter currently
 * selected in the Filters panel.
 */
class RhythmDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit RhythmDock(QWidget *parent = nullptr);

public slots:
    void onCurrentFilterChanged(QmlFilter *filter, QmlMetadata *metadata, int index);

private slots:
    void onBrowseMidi();
    void onApply();
    void onInputsChanged();

protected:
    /*!
     * \brief First refresh happens here, not in the constructor.
     *
     * This dock is built from inside MainWindow's constructor, and
     * MainWindow::singleton() lazily constructs the instance, so anything
     * reaching for MAIN during construction re-enters an in-progress
     * function-local static and deadlocks on its guard. By the time the dock
     * is first shown, the singleton exists.
     */
    void showEvent(QShowEvent *event) override;

private:
    void buildUi();
    void updateEnabledStates();
    RhythmSource buildSource() const;
    Pattern buildPattern() const;
    int timelineLength() const;
    void refreshSummary();

    QRadioButton *m_useBeats{nullptr};
    QRadioButton *m_useMidi{nullptr};
    QDoubleSpinBox *m_bpm{nullptr};
    QDoubleSpinBox *m_offset{nullptr};
    QSpinBox *m_everyNth{nullptr};
    QPushButton *m_midiBrowse{nullptr};
    QLabel *m_midiFile{nullptr};
    QComboBox *m_midiChannel{nullptr};

    QLineEdit *m_pulseFrames{nullptr};
    QLineEdit *m_values{nullptr};
    QComboBox *m_interpolation{nullptr};

    QLabel *m_filterName{nullptr};
    QComboBox *m_parameter{nullptr};
    QComboBox *m_mode{nullptr};
    QLabel *m_summary{nullptr};
    QPushButton *m_apply{nullptr};

    MidiSource m_midi;
    QmlFilter *m_filter{nullptr};
    QmlMetadata *m_metadata{nullptr};
};

#endif // RHYTHMDOCK_H
