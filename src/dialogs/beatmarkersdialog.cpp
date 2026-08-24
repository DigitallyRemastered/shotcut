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

#include "beatmarkersdialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

BeatMarkersDialog::BeatMarkersDialog(int lengthFrames, double fps, QWidget *parent)
    : QDialog(parent)
    , m_color(Qt::red)
    , m_lengthFrames(lengthFrames)
    , m_fps(fps)
{
    setWindowTitle(tr("Generate Markers on a Beat Grid"));

    QGridLayout *grid = new QGridLayout();
    int row = 0;

    grid->addWidget(new QLabel(tr("Tempo")), row, 0, Qt::AlignRight);
    m_bpm = new QDoubleSpinBox(this);
    m_bpm->setRange(1.0, 400.0);
    m_bpm->setDecimals(3);
    m_bpm->setValue(120.0);
    m_bpm->setSuffix(tr(" BPM"));
    grid->addWidget(m_bpm, row++, 1);

    grid->addWidget(new QLabel(tr("Beat Offset")), row, 0, Qt::AlignRight);
    m_offset = new QDoubleSpinBox(this);
    m_offset->setRange(-1000.0, 1000.0);
    m_offset->setDecimals(3);
    m_offset->setValue(0.0);
    m_offset->setToolTip(tr("Shift the whole grid, in beats. Fractions are allowed."));
    grid->addWidget(m_offset, row++, 1);

    grid->addWidget(new QLabel(tr("Every")), row, 0, Qt::AlignRight);
    m_everyNth = new QSpinBox(this);
    m_everyNth->setRange(1, 64);
    m_everyNth->setValue(1);
    m_everyNth->setPrefix(tr("every "));
    m_everyNth->setSuffix(tr(" beat(s)"));
    m_everyNth->setToolTip(tr("Use 4 to mark each bar in 4/4 time."));
    grid->addWidget(m_everyNth, row++, 1);

    grid->addWidget(new QLabel(tr("Color")), row, 0, Qt::AlignRight);
    m_colorMode = new QComboBox(this);
    m_colorMode->addItem(tr("Rainbow"), int(MarkerGenerator::RainbowHue));
    m_colorMode->addItem(tr("Single Color"), int(MarkerGenerator::FixedColor));
    grid->addWidget(m_colorMode, row, 1);
    m_colorButton = new QPushButton(tr("Choose..."), this);
    m_colorButton->setEnabled(false);
    grid->addWidget(m_colorButton, row++, 2);

    grid->addWidget(new QLabel(tr("Name Prefix")), row, 0, Qt::AlignRight);
    m_prefix = new QLineEdit(this);
    m_prefix->setText(tr("Beat"));
    grid->addWidget(m_prefix, row++, 1);

    m_replace = new QCheckBox(tr("Replace existing markers"), this);
    m_replace->setChecked(true);
    m_replace->setToolTip(tr("When off, the generated markers are added to the existing ones."));
    grid->addWidget(m_replace, row++, 1, 1, 2);

    m_summary = new QLabel(this);
    grid->addWidget(m_summary, row++, 0, 1, 3);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(grid);
    layout->addWidget(buttons);

    connect(m_colorButton, &QPushButton::clicked, this, &BeatMarkersDialog::onChooseColor);
    connect(m_bpm,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &BeatMarkersDialog::onValuesChanged);
    connect(m_offset,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &BeatMarkersDialog::onValuesChanged);
    connect(m_everyNth,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &BeatMarkersDialog::onValuesChanged);
    connect(m_colorMode,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &BeatMarkersDialog::onValuesChanged);

    onValuesChanged();
}

void BeatMarkersDialog::onChooseColor()
{
    const QColor chosen = QColorDialog::getColor(m_color, this, tr("Marker Color"));
    if (chosen.isValid()) {
        m_color = chosen;
        onValuesChanged();
    }
}

void BeatMarkersDialog::onValuesChanged()
{
    const bool fixed = m_colorMode->currentData().toInt() == int(MarkerGenerator::FixedColor);
    m_colorButton->setEnabled(fixed);
    if (fixed)
        m_colorButton->setText(m_color.name());
    else
        m_colorButton->setText(tr("Choose..."));

    const int n = MarkerGenerator::count(params());
    m_summary->setText(tr("%n marker(s) will be created.", nullptr, n));
}

MarkerGenerator::Params BeatMarkersDialog::params() const
{
    MarkerGenerator::Params p;
    p.grid.bpm = m_bpm->value();
    p.grid.offsetBeats = m_offset->value();
    p.grid.everyNthBeat = m_everyNth->value();
    p.grid.lengthFrames = m_lengthFrames;
    p.grid.fps = m_fps;
    p.colorMode = MarkerGenerator::ColorMode(m_colorMode->currentData().toInt());
    p.color = m_color;
    p.textPrefix = m_prefix->text();
    return p;
}

bool BeatMarkersDialog::replaceExisting() const
{
    return m_replace->isChecked();
}
