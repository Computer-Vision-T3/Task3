#include "TopTaskBar.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

TopTaskBar::TopTaskBar(QWidget* parent) : QWidget(parent) {
    setObjectName("paramStrip");

    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(20, 12, 20, 12);
    root->setSpacing(20);

    // ── Module selector ──────────────────────────────────────────────
    QWidget* opWidget = new QWidget(this);
    QVBoxLayout* opLay = new QVBoxLayout(opWidget);
    opLay->setContentsMargins(0,0,0,0);
    opLay->setSpacing(5);

    QLabel* opLabel = new QLabel("Active Module", opWidget);
    opLabel->setObjectName("paramLabel");

    m_opSelector = new QComboBox(opWidget);
    m_opSelector->setObjectName("operationSelector");
    m_opSelector->setMinimumWidth(230);
    m_opSelector->addItems({
        "1. Harris Corner Detector",
        "2. Lambda (Shi-Tomasi) Detector",
        "3. SIFT Descriptor",
        "4. SSD Feature Matching",
        "5. NCC Feature Matching"
    });

    opLay->addWidget(opLabel);
    opLay->addWidget(m_opSelector);
    opLay->addStretch();
    root->addWidget(opWidget);

    // ── Thin vertical divider ────────────────────────────────────────
    QFrame* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFixedWidth(1);
    sep1->setStyleSheet("background-color: #E8E2DA;");
    root->addWidget(sep1);

    // ── Dynamic parameter area ───────────────────────────────────────
    m_paramBox = new ParameterBox(this);
    root->addWidget(m_paramBox, 1);

    // ── Thin vertical divider ────────────────────────────────────────
    QFrame* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFixedWidth(1);
    sep2->setStyleSheet("background-color: #E8E2DA;");
    root->addWidget(sep2);

    // ── Action buttons + status ──────────────────────────────────────
    QWidget* actionWidget = new QWidget(this);
    QVBoxLayout* actionLay = new QVBoxLayout(actionWidget);
    actionLay->setContentsMargins(0,0,0,0);
    actionLay->setSpacing(6);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_saveBtn = new QPushButton("Save", actionWidget);
    m_saveBtn->setObjectName("ghostBtn");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setToolTip("Save output image");

    m_clearBtn = new QPushButton("Clear", actionWidget);
    m_clearBtn->setObjectName("secondaryBtn");
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setToolTip("Clear outputs");

    m_applyBtn = new QPushButton("Run Module", actionWidget);
    m_applyBtn->setObjectName("primaryBtn");
    m_applyBtn->setCursor(Qt::PointingHandCursor);
    m_applyBtn->setToolTip("Execute the selected algorithm");

    btnRow->addWidget(m_saveBtn);
    btnRow->addWidget(m_clearBtn);
    btnRow->addWidget(m_applyBtn);

    // Thin progress bar (hidden during idle)
    m_progressBar = new QProgressBar(actionWidget);
    m_progressBar->setRange(0, 0);   // indeterminate
    m_progressBar->setFixedHeight(4);
    m_progressBar->hide();

    m_statusLabel = new QLabel("Ready", actionWidget);
    m_statusLabel->setObjectName("statusIndicator");
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    actionLay->addLayout(btnRow);
    actionLay->addWidget(m_progressBar);
    actionLay->addWidget(m_statusLabel);

    root->addWidget(actionWidget);

    // ── Connections ──────────────────────────────────────────────────
    connect(m_opSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TopTaskBar::onSelectionChanged);
    connect(m_applyBtn, &QPushButton::clicked, this, &TopTaskBar::applyRequested);
    connect(m_clearBtn, &QPushButton::clicked, this, &TopTaskBar::clearRequested);
    connect(m_saveBtn,  &QPushButton::clicked, this, &TopTaskBar::saveRequested);

    // Init
    onSelectionChanged(0);
}

int TopTaskBar::getSelectedOperation() const {
    return m_opSelector->currentIndex() + 1;
}

void TopTaskBar::onSelectionChanged(int index) {
    m_paramBox->updateForTask(index + 1);
    emit taskChanged(index + 1);
    setStatus("Ready", true);
}

void TopTaskBar::setProcessing(bool on) {
    m_applyBtn->setEnabled(!on);
    m_clearBtn->setEnabled(!on);
    m_saveBtn->setEnabled(!on);
    m_opSelector->setEnabled(!on);
    m_paramBox->setEnabled(!on);
    m_progressBar->setVisible(on);

    if (on) {
        m_applyBtn->setText("Processing…");
        setStatus("Computing…", true);
        m_statusLabel->setStyleSheet(
            "color: #5B4FCF; background-color: #EDE8FF;"
            "border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
    } else {
        m_applyBtn->setText("Run Module");
    }
}

void TopTaskBar::setStatus(const QString& msg, bool success) {
    m_statusLabel->setText(msg);
    if (success) {
        if (msg == "Ready") {
            m_statusLabel->setStyleSheet(
                "color: #7A7268; background-color: transparent;"
                "border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
        } else {
            m_statusLabel->setStyleSheet(
                "color: #2D9B6F; background-color: #E8F5F0;"
                "border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
        }
    } else {
        m_statusLabel->setStyleSheet(
            "color: #D32F2F; background-color: #FFEBEE;"
            "border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
    }
}