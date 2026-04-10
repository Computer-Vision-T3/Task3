#include "ParameterBox.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

ParameterBox::ParameterBox(QWidget* parent) : QWidget(parent) {
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(14);
    m_layout->setColumnStretch(5, 1); // right spacer
}

// ── Clear all parameter widgets ───────────────────────────────────────────────
void ParameterBox::clearLayout() {
    QLayoutItem* child;
    while ((child = m_layout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }
}

// ── Rebuild parameters per task ───────────────────────────────────────────────
void ParameterBox::updateForTask(int taskIndex) {
    clearLayout();

    switch (taskIndex) {
    case 1: // ── Harris Corner Detector ──────────────────────────────────────
        addDoubleSpinBox("harrisK",     "Harris K",     0.01, 0.30, 0.05, 0.01, 0, 0);
        addSpinBox      ("harrisBlock", "Block Size",   2,    9,    3,          0, 1);
        addSpinBox      ("harrisAper",  "Aperture",     3,    7,    3,          0, 2);
        addSpinBox      ("harrisThresh","Threshold",    0,    255,  100,        0, 3);
        addCheckBox     ("harrisColor", "Color mode",   false,                  0, 4);
        break;

    case 2: // ── Lambda / Shi-Tomasi Detector ────────────────────────────────
        addSpinBox      ("lambdaMaxKp", "Max KeyPoints",10,   5000, 500,        0, 0);
        addDoubleSpinBox("lambdaQuality","Quality Level",0.001,0.5, 0.01, 0.001,0, 1);
        addSpinBox      ("lambdaDist",  "Min Distance", 1,    100,  10,         0, 2);
        addSpinBox      ("lambdaBlock", "Block Size",   2,    9,    3,          0, 3);
        addCheckBox     ("lambdaColor", "Color mode",   false,                  0, 4);
        break;

    case 3: // ── SIFT Descriptor ─────────────────────────────────────────────
        addSpinBox      ("siftNFeat",   "Features To Drop (0=All)", 0, 5000, 0,  0, 0);
        addSpinBox      ("siftNOctave", "Octave Layers",1,    8,    3,          0, 1);
        addDoubleSpinBox("siftContrast","Contrast Thr.", 0.01, 0.5, 0.04,0.005, 0, 2);
        addDoubleSpinBox("siftEdge",    "Edge Thresh",  1.0,  50.0, 10.0, 1.0, 0, 3);
        addDoubleSpinBox("siftSigma",   "Sigma",        0.5,  5.0,  1.6,  0.1, 0, 4);
        addCheckBox     ("siftColor",   "Color mode",   false,                  0, 5); // <--- ADD THIS LINE
        break;

    case 4: // ── SSD Feature Matching ────────────────────────────────────────
        addSpinBox      ("ssdTopK",     "Top-K Matches",1,    500,  50,         0, 0);
        addDoubleSpinBox("ssdRatio",    "Ratio Test",   0.50, 1.00, 0.75, 0.01, 0, 1);
        addCheckBox     ("ssdCrossCheck","Cross-check",  false,                  0, 2);
        addCombo        ("ssdViz",      "Visualise",
                         {"Rich Lines","Simple Lines","Points Only"},           0, 3);
        break;

    case 5: // ── NCC Feature Matching ────────────────────────────────────────
        addSpinBox      ("nccTopK",     "Top-K Matches",1,    500,  50,         0, 0);
        addDoubleSpinBox("nccThresh",   "NCC Threshold",0.10, 1.00, 0.70, 0.01, 0, 1);
        addCheckBox     ("nccCrossCheck","Cross-check",  false,                  0, 2);
        addCombo        ("nccViz",      "Visualise",
                         {"Rich Lines","Simple Lines","Points Only"},           0, 3);
        break;

    default: break;
    }
}

// ── Value accessors ───────────────────────────────────────────────────────────
int ParameterBox::intValue(const QString& id, int fallback) const {
    if (auto* w = findChild<QSpinBox*>(id))   return w->value();
    if (auto* w = findChild<QSlider*>(id))    return w->value();
    return fallback;
}
double ParameterBox::dblValue(const QString& id, double fallback) const {
    if (auto* w = findChild<QDoubleSpinBox*>(id)) return w->value();
    if (auto* w = findChild<QSlider*>(id))        return w->value() / 100.0;
    return fallback;
}
bool ParameterBox::boolValue(const QString& id, bool fallback) const {
    if (auto* w = findChild<QCheckBox*>(id)) return w->isChecked();
    return fallback;
}
int ParameterBox::comboIndex(const QString& id, int fallback) const {
    if (auto* w = findChild<QComboBox*>(id)) return w->currentIndex();
    return fallback;
}

// ── Widget builder helpers ────────────────────────────────────────────────────
void ParameterBox::addSpinBox(const QString& id, const QString& label,
                               int min, int max, int def, int row, int col) {
    QWidget* c = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(c);
    v->setContentsMargins(0,0,0,0); v->setSpacing(4);

    QLabel* lbl = new QLabel(label, c);
    lbl->setObjectName("paramLabel");

    QSpinBox* sb = new QSpinBox(c);
    sb->setObjectName(id);
    sb->setRange(min, max);
    sb->setValue(def);

    v->addWidget(lbl);
    v->addWidget(sb);
    m_layout->addWidget(c, row, col);
}

void ParameterBox::addDoubleSpinBox(const QString& id, const QString& label,
                                     double min, double max, double def,
                                     double step, int row, int col) {
    QWidget* c = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(c);
    v->setContentsMargins(0,0,0,0); v->setSpacing(4);

    QLabel* lbl = new QLabel(label, c);
    lbl->setObjectName("paramLabel");

    QDoubleSpinBox* sb = new QDoubleSpinBox(c);
    sb->setObjectName(id);
    sb->setRange(min, max);
    sb->setSingleStep(step);
    sb->setDecimals(3);
    sb->setValue(def);

    v->addWidget(lbl);
    v->addWidget(sb);
    m_layout->addWidget(c, row, col);
}

void ParameterBox::addSlider(const QString& id, const QString& label,
                              int min, int max, int def,
                              int row, int col, bool isFloat) {
    QWidget* c = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(c);
    v->setContentsMargins(0,0,0,0); v->setSpacing(4);

    QWidget* lblRow = new QWidget(c);
    QHBoxLayout* h = new QHBoxLayout(lblRow);
    h->setContentsMargins(0,0,0,0);

    QLabel* lbl = new QLabel(label, lblRow);
    lbl->setObjectName("paramLabel");

    QLabel* val = new QLabel(isFloat
        ? QString::number(def / 10.0, 'f', 1)
        : QString::number(def), lblRow);
    val->setObjectName("valueLabel");
    val->setAlignment(Qt::AlignRight);

    h->addWidget(lbl);
    h->addWidget(val);

    QSlider* sl = new QSlider(Qt::Horizontal, c);
    sl->setObjectName(id);
    sl->setRange(min, max);
    sl->setValue(def);
    connect(sl, &QSlider::valueChanged, [val, isFloat](int v) {
        val->setText(isFloat ? QString::number(v / 10.0, 'f', 1) : QString::number(v));
    });

    v->addWidget(lblRow);
    v->addWidget(sl);
    m_layout->addWidget(c, row, col);
}

void ParameterBox::addCombo(const QString& id, const QString& label,
                             const QStringList& items, int row, int col) {
    QWidget* c = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(c);
    v->setContentsMargins(0,0,0,0); v->setSpacing(4);

    QLabel* lbl = new QLabel(label, c);
    lbl->setObjectName("paramLabel");

    QComboBox* cb = new QComboBox(c);
    cb->setObjectName(id);
    cb->addItems(items);

    v->addWidget(lbl);
    v->addWidget(cb);
    m_layout->addWidget(c, row, col);
}

void ParameterBox::addCheckBox(const QString& id, const QString& label,
                                bool def, int row, int col) {
    QCheckBox* cb = new QCheckBox(label, this);
    cb->setObjectName(id);
    cb->setChecked(def);
    m_layout->addWidget(cb, row, col, Qt::AlignBottom);
}

void ParameterBox::addButton(const QString& text, int targetTask, int row, int col) {
    QPushButton* btn = new QPushButton(text, this);
    btn->setObjectName("loadBtn");
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, [this, targetTask]() {
        emit quickActionRequested(targetTask);
    });
    m_layout->addWidget(btn, row, col, Qt::AlignBottom);
}