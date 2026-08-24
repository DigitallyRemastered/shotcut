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

#include "rhythmdock.h"

#include "Logger.h"
#include "controllers/filtercontroller.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmetadata.h"
#include "rhythm/keyframegenerator.h"
#include "settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaEnum>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

//! Parse "0, 4, 6" into {0, 4, 6}. Anything unparseable makes the whole list empty.
template<typename T>
QVector<T> parseList(const QString &text, bool *ok)
{
    QVector<T> result;
    *ok = true;
    const auto parts = text.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
    for (const auto &part : parts) {
        bool converted = false;
        const double value = part.toDouble(&converted);
        if (!converted) {
            *ok = false;
            return {};
        }
        result.append(T(value));
    }
    if (result.isEmpty())
        *ok = false;
    return result;
}

} // namespace

RhythmDock::RhythmDock(QWidget *parent)
    : QDockWidget(parent)
{
    LOG_DEBUG() << "begin";
    setObjectName("RhythmDock");
    setWindowTitle(tr("Rhythm"));
    setWhatsThis(tr("Drive a filter parameter from a tempo or a MIDI performance."));
    buildUi();
    // Nothing global here: see showEvent().
    LOG_DEBUG() << "end";
}

void RhythmDock::showEvent(QShowEvent *event)
{
    QDockWidget::showEvent(event);
    onInputsChanged();
}

void RhythmDock::buildUi()
{
    QScrollArea *scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    QWidget *content = new QWidget;
    scroll->setWidget(content);
    setWidget(scroll);

    QVBoxLayout *layout = new QVBoxLayout(content);

    // --- Timing ---------------------------------------------------------
    QGroupBox *timing = new QGroupBox(tr("Timing"));
    QFormLayout *timingForm = new QFormLayout(timing);

    m_useBeats = new QRadioButton(tr("Tempo grid"));
    m_useMidi = new QRadioButton(tr("MIDI note onsets"));
    m_useBeats->setChecked(true);
    timingForm->addRow(m_useBeats);

    m_bpm = new QDoubleSpinBox;
    m_bpm->setRange(1.0, 400.0);
    m_bpm->setDecimals(3);
    m_bpm->setValue(120.0);
    m_bpm->setSuffix(tr(" BPM"));
    timingForm->addRow(tr("Tempo"), m_bpm);

    m_offset = new QDoubleSpinBox;
    m_offset->setRange(-1000.0, 1000.0);
    m_offset->setDecimals(3);
    m_offset->setToolTip(tr("Shift the grid, in beats. Fractions are allowed."));
    timingForm->addRow(tr("Beat offset"), m_offset);

    m_everyNth = new QSpinBox;
    m_everyNth->setRange(1, 64);
    m_everyNth->setValue(1);
    m_everyNth->setToolTip(tr("Use 4 to fire once a bar in 4/4 time."));
    timingForm->addRow(tr("Every Nth beat"), m_everyNth);

    timingForm->addRow(m_useMidi);
    m_midiBrowse = new QPushButton(tr("Open MIDI File..."));
    timingForm->addRow(m_midiBrowse);
    m_midiFile = new QLabel(tr("none"));
    m_midiFile->setWordWrap(true);
    timingForm->addRow(tr("File"), m_midiFile);
    m_midiChannel = new QComboBox;
    timingForm->addRow(tr("Channel"), m_midiChannel);
    layout->addWidget(timing);

    // --- Pattern --------------------------------------------------------
    QGroupBox *pattern = new QGroupBox(tr("Pattern"));
    QFormLayout *patternForm = new QFormLayout(pattern);

    m_pulseFrames = new QLineEdit("0, 4, 6");
    m_pulseFrames->setToolTip(tr("Frame offsets from each instant, ascending."));
    patternForm->addRow(tr("Pulse frames"), m_pulseFrames);

    m_values = new QLineEdit("1, 2, 1");
    m_values->setToolTip(tr("One value per pulse frame."));
    patternForm->addRow(tr("Values"), m_values);

    m_interpolation = new QComboBox;
    const auto metaEnum = QMetaEnum::fromType<KeyframesModel::InterpolationType>();
    for (int i = 0; i < metaEnum.keyCount(); i++)
        m_interpolation->addItem(QString::fromLatin1(metaEnum.key(i)), metaEnum.value(i));
    m_interpolation->setCurrentIndex(
        m_interpolation->findData(int(KeyframesModel::SmoothNaturalInterpolation)));
    patternForm->addRow(tr("Interpolation"), m_interpolation);
    layout->addWidget(pattern);

    // --- Target ---------------------------------------------------------
    QGroupBox *target = new QGroupBox(tr("Target"));
    QFormLayout *targetForm = new QFormLayout(target);

    m_filterName = new QLabel(tr("No filter selected"));
    m_filterName->setWordWrap(true);
    targetForm->addRow(tr("Filter"), m_filterName);

    m_parameter = new QComboBox;
    targetForm->addRow(tr("Parameter"), m_parameter);

    m_mode = new QComboBox;
    m_mode->addItem(tr("Absolute"), int(KeyframeGenerator::Absolute));
    m_mode->addItem(tr("Multiply existing"), int(KeyframeGenerator::RelativeToExisting));
    m_mode->setToolTip(
        tr("Multiply existing keeps animation you already made and pulses on top of it."));
    targetForm->addRow(tr("Mode"), m_mode);
    layout->addWidget(target);

    m_summary = new QLabel;
    m_summary->setWordWrap(true);
    layout->addWidget(m_summary);

    m_apply = new QPushButton(tr("Apply"));
    layout->addWidget(m_apply);
    layout->addStretch(1);

    connect(m_midiBrowse, &QPushButton::clicked, this, &RhythmDock::onBrowseMidi);
    connect(m_apply, &QPushButton::clicked, this, &RhythmDock::onApply);
    for (auto *widget : {m_useBeats, m_useMidi})
        connect(widget, &QRadioButton::toggled, this, &RhythmDock::onInputsChanged);
    for (auto *widget : {m_bpm, m_offset})
        connect(widget,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this,
                &RhythmDock::onInputsChanged);
    connect(m_everyNth,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &RhythmDock::onInputsChanged);
    for (auto *widget : {m_pulseFrames, m_values})
        connect(widget, &QLineEdit::textChanged, this, &RhythmDock::onInputsChanged);
    connect(m_midiChannel,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &RhythmDock::onInputsChanged);

    updateEnabledStates();
}

void RhythmDock::updateEnabledStates()
{
    const bool beats = m_useBeats->isChecked();
    for (auto *widget : {static_cast<QWidget *>(m_bpm),
                         static_cast<QWidget *>(m_offset),
                         static_cast<QWidget *>(m_everyNth)})
        widget->setEnabled(beats);
    m_midiBrowse->setEnabled(!beats);
    m_midiChannel->setEnabled(!beats);
}

int RhythmDock::timelineLength() const
{
    // Only ever reached from a slot or showEvent, never during construction.
    Mlt::Producer *multitrack = MAIN.multitrack();
    return (multitrack && multitrack->is_valid()) ? multitrack->get_length() : 0;
}

RhythmSource RhythmDock::buildSource() const
{
    RhythmSource source;
    if (m_useMidi->isChecked()) {
        source.setKind(RhythmSource::Midi);
        const int channel = m_midiChannel->count() ? m_midiChannel->currentData().toInt() : -1;
        source.setMidi(&m_midi, channel);
    } else {
        source.setKind(RhythmSource::Beats);
        BeatGrid::Params grid;
        grid.bpm = m_bpm->value();
        grid.offsetBeats = m_offset->value();
        grid.everyNthBeat = m_everyNth->value();
        grid.fps = MLT.profile().fps();
        source.setBeatGrid(grid);
    }
    return source;
}

Pattern RhythmDock::buildPattern() const
{
    bool framesOk = false;
    bool valuesOk = false;
    Pattern pattern;
    pattern.pulseFrames = parseList<int>(m_pulseFrames->text(), &framesOk);
    pattern.values = parseList<double>(m_values->text(), &valuesOk);
    pattern.interpolation = KeyframesModel::InterpolationType(
        m_interpolation->currentData().toInt());
    if (!framesOk || !valuesOk)
        return Pattern();
    return pattern;
}

void RhythmDock::onInputsChanged()
{
    updateEnabledStates();
    refreshSummary();
}

void RhythmDock::refreshSummary()
{
    const int length = timelineLength();
    const Pattern pattern = buildPattern();
    if (!pattern.isValid()) {
        m_summary->setText(tr("Pulse frames and values must be the same length, "
                              "with frames ascending."));
        m_apply->setEnabled(false);
        return;
    }
    if (!m_filter || m_parameter->count() == 0) {
        m_summary->setText(tr("Select a filter with keyframable parameters."));
        m_apply->setEnabled(false);
        return;
    }

    const auto times = buildSource().frames(length, MLT.profile().fps());
    const auto keyframes = PatternExpander::expand(pattern, times, length);
    QString text = tr("%n instant(s), %1 keyframes.", nullptr, int(times.size()))
                       .arg(keyframes.size());
    if (PatternExpander::overlaps(pattern, times))
        text += QStringLiteral(" ") + tr("The pulse is longer than the gap between "
                                         "instants, so pulses overlap.");
    m_summary->setText(text);
    m_apply->setEnabled(!keyframes.isEmpty());
}

void RhythmDock::onBrowseMidi()
{
    const QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Open MIDI File"),
                                                      Settings.openPath(),
                                                      tr("MIDI files (*.mid *.midi *.kar)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!m_midi.load(path, &error)) {
        QMessageBox::warning(this, tr("Rhythm"), error);
        m_midiFile->setText(tr("none"));
        m_midiChannel->clear();
        refreshSummary();
        return;
    }

    m_midiFile->setText(QFileInfo(path).fileName());
    m_midiChannel->clear();
    m_midiChannel->addItem(tr("All channels"), -1);
    for (int channel : m_midi.channels())
        m_midiChannel->addItem(tr("Channel %1").arg(channel + 1), channel);
    m_useMidi->setChecked(true);
    onInputsChanged();
}

void RhythmDock::onCurrentFilterChanged(QmlFilter *filter, QmlMetadata *metadata, int index)
{
    Q_UNUSED(index)
    m_filter = filter;
    m_metadata = metadata;
    m_parameter->clear();

    if (!filter || !metadata) {
        m_filterName->setText(tr("No filter selected"));
        refreshSummary();
        return;
    }

    m_filterName->setText(metadata->name());
    // Offer exactly the parameters the filter itself declares keyframable.
    const auto *keyframes = metadata->keyframes();
    for (int i = 0; i < keyframes->parameterCount(); i++) {
        const auto *parameter = keyframes->parameter(i);
        if (!parameter || parameter->isColor())
            continue; // a colour is not a scalar the pattern can drive
        m_parameter->addItem(parameter->name(),
                             QStringList{parameter->property(),
                                         parameter->isRectangle() ? QStringLiteral("rect")
                                                                  : QStringLiteral("scalar")});
    }
    refreshSummary();
}

void RhythmDock::onApply()
{
    if (!m_filter || m_parameter->count() == 0)
        return;

    const Pattern pattern = buildPattern();
    if (!pattern.isValid())
        return;

    const int length = timelineLength();
    const auto data = m_parameter->currentData().toStringList();
    if (data.size() != 2)
        return;

    KeyframeGenerator::Request request;
    request.property = data.at(0);
    request.valueKind = (data.at(1) == QStringLiteral("rect")) ? KeyframeGenerator::Rect
                                                               : KeyframeGenerator::Scalar;
    request.pattern = pattern;
    request.times = buildSource().frames(length, MLT.profile().fps());
    request.mode = KeyframeGenerator::Mode(m_mode->currentData().toInt());
    request.lengthFrames = length;

    const int written = KeyframeGenerator::apply(m_filter, request);
    LOG_INFO() << "wrote" << written << "keyframes to" << request.property;
    refreshSummary();
}
