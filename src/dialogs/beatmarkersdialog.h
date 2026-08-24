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

#ifndef BEATMARKERSDIALOG_H
#define BEATMARKERSDIALOG_H

#include "rhythm/markergenerator.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

/*!
 * \brief Collects the parameters for generating markers on a musical beat grid.
 */
class BeatMarkersDialog : public QDialog
{
    Q_OBJECT

public:
    BeatMarkersDialog(int lengthFrames, double fps, QWidget *parent = nullptr);

    MarkerGenerator::Params params() const;
    bool replaceExisting() const;

private slots:
    void onChooseColor();
    void onValuesChanged();

private:
    QDoubleSpinBox *m_bpm;
    QDoubleSpinBox *m_offset;
    QSpinBox *m_everyNth;
    QComboBox *m_colorMode;
    QPushButton *m_colorButton;
    QLineEdit *m_prefix;
    QCheckBox *m_replace;
    QLabel *m_summary;

    QColor m_color;
    int m_lengthFrames;
    double m_fps;
};

#endif // BEATMARKERSDIALOG_H
