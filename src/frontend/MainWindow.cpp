#include "MainWindow.h"
#include "controllers/AppController.h"
#include <QApplication>
#include <QScreen>
#include <QFrame>
#include <QScrollArea>
#include <QStyle>
#include <QPropertyAnimation>

// ── Task metadata ────────────────────────────────────────────────────────────
struct TaskMeta {
    QString icon;
    QString shortName;
    QString group;
    QString tooltip;
};

static const QList<TaskMeta> TASKS = {
    { "◈", "Harris Detector",    "Feature Detection",  "Harris corner operator" },
    { "◈", "Lambda Detector",    "Feature Detection",  "Shi-Tomasi / λ operator" },
    { "◉", "SIFT Descriptor",    "Descriptors",        "Scale-invariant descriptors" },
    { "⟺", "SSD Matching",       "Feature Matching",   "Sum of Squared Differences" },
    { "⟺", "NCC Matching",       "Feature Matching",   "Normalized Cross-Correlation" },
};

// ── Constructor ──────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName("MainWindow");

    QWidget* root = new QWidget(this);
    root->setObjectName("rootWidget");
    setCentralWidget(root);

    QHBoxLayout* rootLay = new QHBoxLayout(root);
    rootLay->setContentsMargins(0,0,0,0);
    rootLay->setSpacing(0);

    // Left rail
    buildLeftRail();
    rootLay->addWidget(m_leftRail);

    // Main content column
    m_mainArea = new QWidget(this);
    m_mainArea->setStyleSheet("background-color: #F5F3EF;");
    QVBoxLayout* mainLay = new QVBoxLayout(m_mainArea);
    mainLay->setContentsMargins(0,0,0,0);
    mainLay->setSpacing(0);

    buildHeaderBar();
    mainLay->addWidget(m_headerBar);

    QFrame* sep1 = new QFrame(m_mainArea);
    sep1->setObjectName("divider");
    sep1->setFrameShape(QFrame::HLine);
    sep1->setFixedHeight(1);
    mainLay->addWidget(sep1);

    m_taskBar = new TopTaskBar(m_mainArea);
    mainLay->addWidget(m_taskBar);

    QFrame* sep2 = new QFrame(m_mainArea);
    sep2->setObjectName("divider");
    sep2->setFrameShape(QFrame::HLine);
    sep2->setFixedHeight(1);
    mainLay->addWidget(sep2);

    buildCanvasArea();
    mainLay->addWidget(m_canvasArea, 1);

    rootLay->addWidget(m_mainArea, 1);

    // Controller
    m_controller = new AppController(this);

    // Initial state
    setActiveTask(1);
    updateLayoutForTask(1);

    // Sync task changes
    connect(m_taskBar, &TopTaskBar::taskChanged, this, [this](int idx) {
        setActiveTask(idx);
        updateLayoutForTask(idx);
    });
}

MainWindow::~MainWindow() {}

// ── Left Rail ────────────────────────────────────────────────────────────────
void MainWindow::buildLeftRail() {
    m_leftRail = new QWidget(this);
    m_leftRail->setObjectName("leftRail");
    m_leftRail->setFixedWidth(216);

    QVBoxLayout* lay = new QVBoxLayout(m_leftRail);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);

    // Logo area
    QWidget* logo = new QWidget(m_leftRail);
    logo->setFixedHeight(64);
    logo->setStyleSheet("background-color: #FDFCFA; border-bottom: 1px solid #E8E2DA;");
    QVBoxLayout* logoLay = new QVBoxLayout(logo);
    logoLay->setContentsMargins(18,0,18,0);
    logoLay->setAlignment(Qt::AlignVCenter);

    QLabel* logoTop = new QLabel("FEATURE", logo);
    logoTop->setStyleSheet("font-size: 18px; font-weight: 900; letter-spacing: -0.02em; color: #2C2825;");

    QLabel* logoBot = new QLabel("STUDIO", logo);
    logoBot->setStyleSheet("font-size: 10px; font-weight: 700; letter-spacing: 0.30em; color: #5B4FCF; margin-top: -4px;");

    logoLay->addWidget(logoTop);
    logoLay->addWidget(logoBot);
    lay->addWidget(logo);

    // Task list (scrollable)
    QScrollArea* scroll = new QScrollArea(m_leftRail);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet("background: transparent; border: none;");

    QWidget* listWidget = new QWidget();
    listWidget->setObjectName("taskListContainer");
    QVBoxLayout* listLay = new QVBoxLayout(listWidget);
    listLay->setContentsMargins(10,14,10,14);
    listLay->setSpacing(2);

    QString currentGroup;
    for (int i = 0; i < TASKS.size(); ++i) {
        const TaskMeta& t = TASKS[i];

        if (t.group != currentGroup) {
            if (!currentGroup.isEmpty()) {
                QFrame* g = new QFrame(listWidget);
                g->setFixedHeight(8);
                g->setStyleSheet("background: transparent;");
                listLay->addWidget(g);
            }
            currentGroup = t.group;

            QLabel* grpLabel = new QLabel(t.group.toUpper(), listWidget);
            grpLabel->setObjectName("sectionLabel");
            grpLabel->setContentsMargins(4,0,0,4);
            listLay->addWidget(grpLabel);
        }

        QPushButton* btn = new QPushButton(listWidget);
        btn->setObjectName("taskItem");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setFixedHeight(38);
        btn->setText(QString("  %1   %2").arg(i + 1).arg(t.shortName));
        btn->setToolTip(t.tooltip);
        btn->setProperty("active", false);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            if (auto* sel = m_taskBar->findChild<QComboBox*>("operationSelector"))
                sel->setCurrentIndex(i);
        });

        m_taskButtons.append(btn);
        listLay->addWidget(btn);
    }

    // Legend section
    QFrame* legSep = new QFrame(listWidget);
    legSep->setFixedHeight(16);
    legSep->setStyleSheet("background: transparent;");
    listLay->addWidget(legSep);

    QLabel* legTitle = new QLabel("KEYPOINT COLORS", listWidget);
    legTitle->setObjectName("sectionLabel");
    legTitle->setContentsMargins(4,0,0,4);
    listLay->addWidget(legTitle);

    auto makeLegend = [&](const QString& colorHex, const QString& text) {
        QWidget* row = new QWidget(listWidget);
        QHBoxLayout* rh = new QHBoxLayout(row);
        rh->setContentsMargins(4,0,4,0);
        rh->setSpacing(8);

        QLabel* dot = new QLabel(row);
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString(
            "background-color: %1; border-radius: 5px;").arg(colorHex));

        QLabel* lbl = new QLabel(text, row);
        lbl->setStyleSheet("font-size: 11px; color: #7A7268;");

        rh->addWidget(dot);
        rh->addWidget(lbl);
        rh->addStretch();
        listLay->addWidget(row);
    };

    makeLegend("#2D9B6F", "Small  (< 10 px)");
    makeLegend("#5B4FCF", "Medium (10–25 px)");
    makeLegend("#D85A30", "Large  (> 25 px)");

    listLay->addStretch();
    scroll->setWidget(listWidget);
    lay->addWidget(scroll, 1);

    // Footer
    QWidget* footer = new QWidget(m_leftRail);
    footer->setFixedHeight(52);
    footer->setStyleSheet("background-color: #FDFCFA; border-top: 1px solid #E8E2DA;");
    QVBoxLayout* footLay = new QVBoxLayout(footer);
    footLay->setContentsMargins(18,0,18,0);
    footLay->setAlignment(Qt::AlignVCenter);

    QLabel* tech = new QLabel("OpenCV · Qt5 · C++17", footer);
    tech->setStyleSheet("font-size: 10px; color: #C4BDB4; font-weight: 500;");
    QLabel* ver = new QLabel("Task 3: Feature Detection & Matching", footer);
    ver->setStyleSheet("font-size: 10px; color: #D4CEC6; font-weight: 400;");
    footLay->addWidget(tech);
    footLay->addWidget(ver);
    lay->addWidget(footer);
}

// ── Header Bar ───────────────────────────────────────────────────────────────
void MainWindow::buildHeaderBar() {
    m_headerBar = new QWidget(this);
    m_headerBar->setObjectName("headerBar");
    m_headerBar->setFixedHeight(56);

    QHBoxLayout* h = new QHBoxLayout(m_headerBar);
    h->setContentsMargins(20,0,20,0);
    h->setSpacing(10);

    QLabel* ws = new QLabel("Workspace", m_headerBar);
    ws->setStyleSheet("font-size: 13px; font-weight: 600; color: #2C2825;");

    QLabel* div = new QLabel("›", m_headerBar);
    div->setStyleSheet("font-size: 14px; color: #C4BDB4;");

    QLabel* activeLabel = new QLabel("Harris Detector", m_headerBar);
    activeLabel->setObjectName("activeBreadcrumb");
    activeLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #5B4FCF;");

    h->addWidget(ws);
    h->addWidget(div);
    h->addWidget(activeLabel);
    h->addStretch();

    // Timing info (updated from controller)
    QLabel* hint = new QLabel("↵ Run   ⌫ Clear   ⇧S Save", m_headerBar);
    hint->setStyleSheet("font-size: 10px; color: #C4BDB4; letter-spacing: 0.05em;");
    h->addWidget(hint);

    // Accent dot
    QLabel* dot = new QLabel(m_headerBar);
    dot->setFixedSize(8, 8);
    dot->setStyleSheet("background-color: #5B4FCF; border-radius: 4px;");
    h->addWidget(dot);
}

// ── Canvas Area ──────────────────────────────────────────────────────────────
void MainWindow::buildCanvasArea() {
    m_canvasArea = new QWidget(this);
    m_canvasArea->setObjectName("canvasArea");

    QHBoxLayout* lay = new QHBoxLayout(m_canvasArea);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);

    // Main horizontal splitter: [input side] | [divider] | [output side]
    m_contentSplitter = new QSplitter(Qt::Horizontal, m_canvasArea);
    m_contentSplitter->setHandleWidth(1);
    m_contentSplitter->setChildrenCollapsible(false);

    // Left: input panels (vertical splitter for A/B)
    m_inputContainer = new QWidget(m_canvasArea);
    m_inputContainer->setStyleSheet("background-color: #F5F3EF;");
    m_inputSplitter = new QSplitter(Qt::Vertical, m_inputContainer);
    m_inputSplitter->setHandleWidth(6);
    m_inputSplitter->setChildrenCollapsible(false);

    QVBoxLayout* inputLay = new QVBoxLayout(m_inputContainer);
    inputLay->setContentsMargins(12,12,6,12);
    inputLay->setSpacing(0);
    inputLay->addWidget(m_inputSplitter);

    // Image panels
    m_panelA = new ImagePanel("Source Image",  PanelRole::InputA, m_inputSplitter);
    m_panelB = new ImagePanel("Query Image",   PanelRole::InputB, m_inputSplitter);
    m_inputSplitter->addWidget(m_panelA);
    m_inputSplitter->addWidget(m_panelB);
    m_panelB->hide(); // Only shown for matching tasks

    // Thin center divider
    QWidget* divW = new QWidget(m_canvasArea);
    divW->setFixedWidth(1);
    divW->setStyleSheet("background-color: #E8E2DA;");

    // Right: output panel + info sidebar
    m_outputContainer = new QWidget(m_canvasArea);
    m_outputContainer->setStyleSheet("background-color: #F5F3EF;");
    QHBoxLayout* outLay = new QHBoxLayout(m_outputContainer);
    outLay->setContentsMargins(6,12,12,12);
    outLay->setSpacing(0);

    m_panelOut = new ImagePanel("Result", PanelRole::Output, m_outputContainer);
    outLay->addWidget(m_panelOut, 1);

    // Info / results sidebar
    m_infoSidebar = new QTextBrowser(m_outputContainer);
    m_infoSidebar->setFixedWidth(290);
    m_infoSidebar->setOpenExternalLinks(false);
    m_infoSidebar->hide();
    outLay->addWidget(m_infoSidebar);

    m_contentSplitter->addWidget(m_inputContainer);
    m_contentSplitter->addWidget(divW);
    m_contentSplitter->addWidget(m_outputContainer);
    m_contentSplitter->setStretchFactor(0, 2);
    m_contentSplitter->setStretchFactor(1, 0);
    m_contentSplitter->setStretchFactor(2, 3);

    lay->addWidget(m_contentSplitter, 1);
}

// ── Active Task Highlight ─────────────────────────────────────────────────────
void MainWindow::setActiveTask(int taskIndex) {
    for (int i = 0; i < m_taskButtons.size(); ++i) {
        bool active = (i == taskIndex - 1);
        m_taskButtons[i]->setProperty("active", active);
        m_taskButtons[i]->style()->unpolish(m_taskButtons[i]);
        m_taskButtons[i]->style()->polish(m_taskButtons[i]);
    }

    // Update breadcrumb
    if (auto* bc = m_headerBar->findChild<QLabel*>("activeBreadcrumb")) {
        if (taskIndex >= 1 && taskIndex <= TASKS.size())
            bc->setText(TASKS[taskIndex - 1].shortName);
    }
}

// ── Layout Updates ────────────────────────────────────────────────────────────
void MainWindow::updateLayoutForTask(int taskIndex) {
    bool twoInputs = taskNeedsTwoInputs(taskIndex);

    // Show/hide panel B
    m_panelB->setVisible(twoInputs);

    // Adjust splitter sizes
    if (twoInputs) {
        m_inputSplitter->setSizes({ 10000, 10000 }); // equal
    } else {
        m_inputSplitter->setSizes({ 10000, 0 });
    }

    // Panel titles per task
    switch (taskIndex) {
    case 1:
        m_panelA->findChild<QLabel*>("panelTitle") &&
            m_panelA->findChild<QLabel*>("panelTitle")->setText("Source Image");
        m_panelOut->findChild<QLabel*>("panelTitle") &&
            m_panelOut->findChild<QLabel*>("panelTitle")->setText("Harris Response");
        m_infoSidebar->show();
        break;
    case 2:
        m_panelOut->findChild<QLabel*>("panelTitle") &&
            m_panelOut->findChild<QLabel*>("panelTitle")->setText("Lambda Response");
        m_infoSidebar->show();
        break;
    case 3:
        m_panelOut->findChild<QLabel*>("panelTitle") &&
            m_panelOut->findChild<QLabel*>("panelTitle")->setText("SIFT Keypoints");
        m_infoSidebar->show();
        break;
    case 4:
        m_panelA->findChild<QLabel*>("panelTitle") &&
            m_panelA->findChild<QLabel*>("panelTitle")->setText("Image A");
        m_panelB->findChild<QLabel*>("panelTitle") &&
            m_panelB->findChild<QLabel*>("panelTitle")->setText("Image B");
        m_panelOut->findChild<QLabel*>("panelTitle") &&
            m_panelOut->findChild<QLabel*>("panelTitle")->setText("SSD Matches");
        m_infoSidebar->show();
        break;
    case 5:
        m_panelA->findChild<QLabel*>("panelTitle") &&
            m_panelA->findChild<QLabel*>("panelTitle")->setText("Image A");
        m_panelB->findChild<QLabel*>("panelTitle") &&
            m_panelB->findChild<QLabel*>("panelTitle")->setText("Image B");
        m_panelOut->findChild<QLabel*>("panelTitle") &&
            m_panelOut->findChild<QLabel*>("panelTitle")->setText("NCC Matches");
        m_infoSidebar->show();
        break;
    default: break;
    }

    // Default sidebar content
    m_infoSidebar->setHtml(R"(
        <style>
            body { font-family: 'DM Sans', sans-serif; color: #2C2825; margin: 0; padding: 0; }
            .card { background: #FFFFFF; border: 1px solid #E6E0F7; border-radius: 12px;
                    padding: 14px; margin-bottom: 10px; }
            h3 { font-size: 14px; font-weight: 800; color: #2C2825; margin: 0 0 4px; }
            p { font-size: 12px; color: #7A7268; line-height: 1.6; margin: 6px 0 0; }
            .badge { display: inline-block; background: #EDE8FF; color: #5B4FCF;
                     border-radius: 6px; padding: 3px 9px; font-size: 11px; font-weight: 800; }
            .badge-gold { display: inline-block; background: #FEF3C7; color: #B45309;
                          border-radius: 6px; padding: 3px 9px; font-size: 11px; font-weight: 800; }
        </style>
        <div class='card'>
            <h3>Results</h3>
            <p>Click <b>Run Module</b> to compute features and see results here.</p>
        </div>
        <div class='card'>
            <p class='badge'>Keypoints: —</p><br><br>
            <p class='badge-gold'>Time: —</p>
        </div>
    )");
}

void MainWindow::setStatusMessage(const QString& msg, bool success) {
    m_taskBar->setStatus(msg, success);
}