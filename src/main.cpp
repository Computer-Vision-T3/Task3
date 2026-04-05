#include <QApplication>
#include <QFontDatabase>
#include <QScreen>
#include "frontend/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setAttribute(Qt::AA_EnableHighDpiScaling);

    // ── Typography ───────────────────────────────────────────────────
    QFont appFont("DM Sans", 13);
    appFont.setHintingPreference(QFont::PreferFullHinting);
    app.setFont(appFont);

    // ── Global Stylesheet ────────────────────────────────────────────
    // Palette:
    //   Pearl White   #FDFCFA  (surfaces)
    //   Ivory         #F5F3EF  (page bg)
    //   Warm Slate    #2C2825  (primary text)
    //   Warm Mid      #7A7268  (secondary text)
    //   Muted         #C4BDB4  (disabled/hints)
    //   Divider       #E8E2DA
    //   Purple 500    #5B4FCF  (accent)
    //   Purple 700    #7C3AED  (primary btn)
    //   Purple Tint   #EDE8FF  (active bg, badge)
    //   Sage Green    #2D9B6F  (success)
    //   Sage Tint     #E8F5F0
    //   Rust Red      #D32F2F  (error)
    //   Rust Tint     #FFEBEE
    //   Gold Amber    #B45309  (timing badge)
    //   Gold Tint     #FEF3C7

    QString styleSheet = R"(
/* ══════════════════════════════════════════════════════
   FEATURE STUDIO — REFINED WARM LIGHT THEME
   Task 3: Feature Detection & Matching
   ══════════════════════════════════════════════════════ */

* { outline: none; box-sizing: border-box; }

QMainWindow, QWidget {
    background-color: #F5F3EF;
    color: #2C2825;
    font-family: 'DM Sans', 'Segoe UI', system-ui, sans-serif;
    font-size: 13px;
}

/* ── Sidebar / Left Rail ─────────────────────────────── */
QWidget#leftRail {
    background-color: #FDFCFA;
    border-right: 1px solid #E8E2DA;
}
QWidget#taskListContainer { background-color: transparent; }

/* Task nav buttons */
QPushButton#taskItem {
    background-color: transparent;
    color: #7A7268;
    border: none;
    border-radius: 10px;
    padding: 10px 14px;
    text-align: left;
    font-size: 12px;
    font-weight: 500;
}
QPushButton#taskItem:hover {
    background-color: #F0EDE8;
    color: #2C2825;
}
QPushButton#taskItem[active="true"] {
    background-color: #EDE8FF;
    color: #5B4FCF;
    font-weight: 700;
}

/* ── Top Header Bar ──────────────────────────────────── */
QWidget#headerBar {
    background-color: #FDFCFA;
    border-bottom: 1px solid #E8E2DA;
    min-height: 56px;
    max-height: 56px;
}

/* ── Parameter Strip ─────────────────────────────────── */
QWidget#paramStrip {
    background-color: #FAFAF8;
    border-bottom: 1px solid #EDEAE4;
    min-height: 110px;
    max-height: 110px;
}

/* ── Canvas / Content Area ───────────────────────────── */
QWidget#canvasArea { background-color: #F5F3EF; }
QWidget#rootWidget { background-color: #F5F3EF; }

/* ── Image Panels ─────────────────────────────────────── */
QWidget#imagePanel {
    background-color: #FFFFFF;
    border: 1px solid #E4DFD8;
    border-radius: 16px;
}
QWidget#imagePanelHeader {
    background-color: transparent;
    border-bottom: 1px solid #F0ECE6;
}
QLabel#panelTitle {
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    color: #A09890;
}
QLabel#panelBadge {
    background-color: #EDE8FF;
    color: #5B4FCF;
    border-radius: 4px;
    padding: 2px 7px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.05em;
}
QLabel#panelBadgeInput {
    background-color: #E8F5F0;
    color: #2D9B6F;
    border-radius: 4px;
    padding: 2px 7px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.05em;
}
QLabel#panelBadgeInput2 {
    background-color: #FEF3C7;
    color: #B45309;
    border-radius: 4px;
    padding: 2px 7px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.05em;
}
QLabel#imageDisplay {
    background-color: #F9F7F4;
    border-radius: 10px;
    color: #C4BDB4;
    font-size: 12px;
}
QLabel#keyPointCountLabel {
    background-color: #EDE8FF;
    color: #5B4FCF;
    border-radius: 6px;
    padding: 3px 9px;
    font-size: 11px;
    font-weight: 700;
}
QLabel#timingLabel {
    background-color: #FEF3C7;
    color: #B45309;
    border-radius: 6px;
    padding: 3px 9px;
    font-size: 11px;
    font-weight: 700;
}

/* ── Dividers ──────────────────────────────────────────── */
QFrame#divider {
    background-color: #E8E2DA;
    border: none;
}

/* ── Primary Action Button ───────────────────────────── */
QPushButton#primaryBtn {
    background-color: #7C3AED;
    color: #FFFFFF;
    border: 1px solid #6D28D9;
    border-radius: 10px;
    padding: 9px 24px;
    font-weight: 800;
    font-size: 13px;
    letter-spacing: 0.02em;
    min-height: 34px;
}
QPushButton#primaryBtn:hover  { background-color: #8B5CF6; border-color: #7C3AED; }
QPushButton#primaryBtn:pressed { background-color: #6D28D9; border-color: #5B21B6; }
QPushButton#primaryBtn:disabled { background-color: #C9C3EF; color: #FDFCFA; border-color: #C9C3EF; }

/* ── Secondary Button ─────────────────────────────────── */
QPushButton#secondaryBtn {
    background-color: #FFFFFF;
    color: #5B4FCF;
    border: 1.5px solid #CCC8F0;
    border-radius: 10px;
    padding: 8px 18px;
    font-weight: 600;
    font-size: 13px;
}
QPushButton#secondaryBtn:hover  { background-color: #EDE8FF; border-color: #5B4FCF; }
QPushButton#secondaryBtn:pressed { background-color: #DDD8FF; }

/* ── Ghost / Tertiary Button ──────────────────────────── */
QPushButton#ghostBtn {
    background-color: transparent;
    color: #A09890;
    border: 1.5px solid #DDD8D2;
    border-radius: 10px;
    padding: 8px 18px;
    font-weight: 600;
    font-size: 13px;
}
QPushButton#ghostBtn:hover { background-color: #F5F0EB; color: #5B4FCF; border-color: #CCC8F0; }

/* ── Load Image Button ────────────────────────────────── */
QPushButton#loadBtn {
    background-color: #F4F1FF;
    color: #4D42B8;
    border: 1.5px solid #BDB4F3;
    border-radius: 10px;
    padding: 10px 16px;
    font-weight: 700;
    font-size: 12px;
    min-height: 28px;
}
QPushButton#loadBtn:hover  { background-color: #E8E2FF; color: #3D348F; border-color: #8E84E8; }
QPushButton#loadBtn:pressed { background-color: #DDD5FF; color: #2F2870; }

/* ── Clear Keypoints Button ──────────────────────────── */
QPushButton#clearKpBtn {
    background-color: #FFF8F0;
    color: #9A5C2A;
    border: 1.5px solid #F0D5B3;
    border-radius: 10px;
    padding: 6px 12px;
    font-weight: 700;
    font-size: 11px;
    min-height: 26px;
}
QPushButton#clearKpBtn:hover { background-color: #FCEEDD; border-color: #D4934A; }

/* ── Dropdowns ────────────────────────────────────────── */
QComboBox {
    background-color: #FFFFFF;
    border: 1.5px solid #DDD8D2;
    border-radius: 9px;
    padding: 6px 32px 6px 12px;
    color: #2C2825;
    font-weight: 500;
    font-size: 13px;
    min-height: 34px;
    selection-background-color: #EDE8FF;
}
QComboBox:hover { border-color: #9B92E8; }
QComboBox:focus { border-color: #5B4FCF; }
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 28px;
    border-left: 1px solid #E8E2DA;
    border-radius: 0 9px 9px 0;
}
QComboBox::down-arrow {
    width: 0; height: 0;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #A09890;
    margin-right: 8px;
}
QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    border: 1.5px solid #DDD8D2;
    border-radius: 10px;
    padding: 4px;
    selection-background-color: #EDE8FF;
    selection-color: #5B4FCF;
    outline: none;
}
QComboBox QAbstractItemView::item {
    padding: 8px 12px;
    border-radius: 7px;
    min-height: 30px;
}

/* ── Spin Boxes ───────────────────────────────────────── */
QSpinBox, QDoubleSpinBox {
    background-color: #FFFFFF;
    border: 1.5px solid #DDD8D2;
    border-radius: 9px;
    padding: 6px 10px;
    color: #2C2825;
    font-weight: 500;
    min-height: 34px;
    min-width: 70px;
}
QSpinBox:hover, QDoubleSpinBox:hover { border-color: #9B92E8; }
QSpinBox:focus, QDoubleSpinBox:focus { border-color: #5B4FCF; }
QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background-color: #F5F3EF;
    border: none;
    width: 22px;
}
QSpinBox::up-button, QDoubleSpinBox::up-button {
    border-radius: 0 9px 0 0;
    border-left: 1px solid #E8E2DA;
}
QSpinBox::down-button, QDoubleSpinBox::down-button {
    border-radius: 0 0 9px 0;
    border-left: 1px solid #E8E2DA;
    border-top: 1px solid #E8E2DA;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: #EDE8FF;
}

/* ── Sliders ──────────────────────────────────────────── */
QSlider::groove:horizontal {
    height: 4px;
    background: #E8E2DA;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #5B4FCF;
    border: 2px solid #FFFFFF;
    width: 16px; height: 16px;
    margin: -6px 0;
    border-radius: 8px;
}
QSlider::handle:horizontal:hover {
    background: #4D42B8;
    width: 18px; height: 18px;
    margin: -7px 0;
}
QSlider::sub-page:horizontal {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #9B92E8,stop:1 #5B4FCF);
    border-radius: 2px;
}

/* ── Labels ───────────────────────────────────────────── */
QLabel { background-color: transparent; }
QLabel#sectionLabel {
    font-size: 10px;
    font-weight: 800;
    letter-spacing: 0.12em;
    color: #C4BDB4;
    padding: 0 4px;
}
QLabel#statusIndicator {
    background-color: #E8F5F0;
    color: #2D9B6F;
    border-radius: 6px;
    padding: 4px 10px;
    font-size: 11px;
    font-weight: 700;
}
QLabel#paramLabel {
    font-size: 12px;
    font-weight: 600;
    color: #7A7268;
}
QLabel#valueLabel {
    font-size: 12px;
    font-weight: 700;
    color: #5B4FCF;
    min-width: 28px;
}

/* ── Info / Results Sidebar ───────────────────────────── */
QTextBrowser {
    background-color: #FDFCFA;
    border: none;
    border-left: 1px solid #E8E2DA;
    color: #2C2825;
    font-size: 13px;
    padding: 20px 18px;
    selection-background-color: #EDE8FF;
}
QTextBrowser:focus { border: none; }

/* ── Splitter ─────────────────────────────────────────── */
QSplitter::handle { background-color: #E8E2DA; }
QSplitter::handle:horizontal { width: 1px; }
QSplitter::handle:vertical   { height: 1px; }
QSplitter::handle:hover      { background-color: #9B92E8; }

/* ── Scrollbars ───────────────────────────────────────── */
QScrollBar:vertical {
    background: transparent;
    width: 6px; margin: 0;
}
QScrollBar::handle:vertical {
    background: #D4CEC6;
    border-radius: 3px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover { background: #A09890; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

QScrollBar:horizontal {
    background: transparent;
    height: 6px; margin: 0;
}
QScrollBar::handle:horizontal {
    background: #D4CEC6;
    border-radius: 3px;
    min-width: 30px;
}
QScrollBar::handle:horizontal:hover { background: #A09890; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── Tooltips ─────────────────────────────────────────── */
QToolTip {
    background-color: #FDFCFA;
    color: #2C2825;
    border: 1px solid #CFC7F4;
    border-radius: 8px;
    padding: 9px 12px;
    font-size: 12px;
    font-weight: 600;
    opacity: 240;
}

/* ── Progress Bar ─────────────────────────────────────── */
QProgressBar {
    background-color: #E8E2DA;
    border-radius: 4px;
    border: none;
    height: 4px;
    text-align: center;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #9B92E8,stop:1 #5B4FCF);
    border-radius: 4px;
}

/* ── Tab Widget (for results) ─────────────────────────── */
QTabWidget::pane {
    border: 1px solid #E4DFD8;
    border-radius: 12px;
    background-color: #FFFFFF;
}
QTabBar::tab {
    background-color: transparent;
    color: #7A7268;
    border: none;
    padding: 8px 18px;
    font-size: 12px;
    font-weight: 600;
    border-radius: 8px 8px 0 0;
}
QTabBar::tab:selected {
    background-color: #EDE8FF;
    color: #5B4FCF;
    font-weight: 700;
}
QTabBar::tab:hover:!selected { background-color: #F0EDE8; color: #2C2825; }

/* ── Check Boxes ──────────────────────────────────────── */
QCheckBox {
    color: #7A7268;
    font-size: 12px;
    font-weight: 500;
    spacing: 6px;
}
QCheckBox::indicator {
    width: 16px; height: 16px;
    border: 1.5px solid #DDD8D2;
    border-radius: 4px;
    background-color: #FFFFFF;
}
QCheckBox::indicator:checked {
    background-color: #5B4FCF;
    border-color: #5B4FCF;
}
QCheckBox::indicator:hover { border-color: #9B92E8; }
)";

    app.setStyleSheet(styleSheet);

    MainWindow window;
    window.setWindowTitle("Feature Studio — Task 3: Feature Detection & Matching");

    // Center on screen
    QScreen* screen = QApplication::primaryScreen();
    QRect sg = screen->availableGeometry();
    window.resize(1440, 900);
    window.move((sg.width() - 1440) / 2, (sg.height() - 900) / 2);
    window.show();

    return app.exec();
}