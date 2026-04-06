#include "AppController.h"
#include "../MainWindow.h"
#include "../components/TopTaskBar.h"
#include "../components/ImagePanel.h"
#include "../components/ParameterBox.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>

// ── Backend includes ─────────────────────────────────────────────────────────
// Students implement these; we call through them here.
// Stub signatures are provided in backend/ so this compiles before they are written.
#include "../../backend/Module1_HarrisDetector/HarrisDetector.h"
#include "../../backend/Module2_LambdaDetector/LambdaDetector.h"
#include "../../backend/Module3_SIFTDescriptor/SIFTDescriptor.h"
#include "../../backend/Module4_SSDMatching/SSDMatcher.h"
#include "../../backend/Module5_NCCMatching/NCCMatcher.h"

// ── Constructor ──────────────────────────────────────────────────────────────
AppController::AppController(MainWindow* window, QObject* parent)
    : QObject(parent), m_window(window)
{
    auto* bar = m_window->getTopTaskBar();

    connect(bar, &TopTaskBar::taskChanged,   this, &AppController::handleTaskChange);
    connect(bar, &TopTaskBar::applyRequested,this, &AppController::handleApply);
    connect(bar, &TopTaskBar::clearRequested,this, &AppController::handleClear);
    connect(bar, &TopTaskBar::saveRequested, this, &AppController::handleSave);

    // Keep state manager in sync with panel images
    connect(m_window->getPanelA(), &ImagePanel::imageLoaded,
            this, [this](const cv::Mat& img){ m_state.setImageA(img); });
    connect(m_window->getPanelB(), &ImagePanel::imageLoaded,
            this, [this](const cv::Mat& img){ m_state.setImageB(img); });
}

// ── Task change ───────────────────────────────────────────────────────────────
void AppController::handleTaskChange(int taskIndex) {
    m_currentTask = taskIndex;
    m_window->updateLayoutForTask(taskIndex);
}

// ── Main dispatch ─────────────────────────────────────────────────────────────
void AppController::handleApply() {
    // Guard: need at least image A
    if (!m_state.hasImageA()) {
        m_window->setStatusMessage("Load an image first!", false);
        return;
    }

    // For matching tasks also need image B
    bool needsB = (m_currentTask == 4 || m_currentTask == 5);
    if (needsB && !m_state.hasImageB()) {
        m_window->setStatusMessage("Load both Image A and Image B!", false);
        return;
    }

    m_window->getTopTaskBar()->setProcessing(true);

    try {
        switch (m_currentTask) {
        case 1: runHarris(); break;
        case 2: runLambda(); break;
        case 3: runSIFT();   break;
        case 4: runSSD();    break;
        case 5: runNCC();    break;
        default: break;
        }
    } catch (const std::exception& ex) {
        m_window->setStatusMessage(
            QString("Error: %1").arg(ex.what()), false);
    } catch (...) {
        m_window->setStatusMessage("Unknown error in backend", false);
    }

    m_window->getTopTaskBar()->setProcessing(false);
}

// ── Module 1 — Harris ────────────────────────────────────────────────────────
void AppController::runHarris() {
    auto* pBox = m_window->getTopTaskBar()->getParameterBox();

    double k      = pBox->dblValue("harrisK",      0.05);
    int    block  = pBox->intValue("harrisBlock",   3);
    int    apert  = pBox->intValue("harrisAper",    3);
    int    thresh = pBox->intValue("harrisThresh",  100);
    bool   color  = pBox->boolValue("harrisColor",  false);

    // Aperture must be odd
    if (apert % 2 == 0) apert++;

    cv::Mat src = m_state.getImageA();
    double timingMs = 0.0;

    auto [result, keypoints] = HarrisDetector::detect(src, k, block, apert, thresh, color, timingMs);

    m_state.setOutput(result);
    m_window->getPanelOut()->displayImage(result);
    m_window->getPanelOut()->setKeyPoints(keypoints);
    m_window->getPanelOut()->setTimingMs(timingMs);
    m_window->getPanelA()->setTimingMs(timingMs);

    showDetectionReport("Harris Corner", (int)keypoints.size(), timingMs, color);
    m_window->setStatusMessage(
        QString("Harris: %1 corners in %2 ms ✓").arg(keypoints.size()).arg(timingMs, 0, 'f', 1),
        true);
}

// ── Module 2 — Lambda / Shi-Tomasi ───────────────────────────────────────────
void AppController::runLambda() {
    auto* pBox = m_window->getTopTaskBar()->getParameterBox();

    int    maxKp   = pBox->intValue("lambdaMaxKp",   500);
    double quality = pBox->dblValue("lambdaQuality", 0.01);
    int    minDist = pBox->intValue("lambdaDist",    10);
    int    block   = pBox->intValue("lambdaBlock",   3);
    bool   color   = pBox->boolValue("lambdaColor",  false);

    cv::Mat src = m_state.getImageA();
    double timingMs = 0.0;

    auto [result, keypoints] = LambdaDetector::detect(src, maxKp, quality, minDist, block, color, timingMs);

    m_state.setOutput(result);
    m_window->getPanelOut()->displayImage(result);
    m_window->getPanelOut()->setKeyPoints(keypoints);
    m_window->getPanelOut()->setTimingMs(timingMs);
    m_window->getPanelA()->setTimingMs(timingMs);

    showDetectionReport("Lambda (Shi-Tomasi)", (int)keypoints.size(), timingMs, color);
    m_window->setStatusMessage(
        QString("Lambda: %1 features in %2 ms ✓").arg(keypoints.size()).arg(timingMs, 0, 'f', 1),
        true);
}

// ── Module 3 — SIFT ──────────────────────────────────────────────────────────
void AppController::runSIFT() {
    auto* pBox = m_window->getTopTaskBar()->getParameterBox();

    int    nFeat    = pBox->intValue("siftNFeat",    0);
    int    nOctave  = pBox->intValue("siftNOctave",  3);
    double contrast = pBox->dblValue("siftContrast", 0.04);
    double edge     = pBox->dblValue("siftEdge",     10.0);
    double sigma    = pBox->dblValue("siftSigma",    1.6);
    bool   color    = pBox->boolValue("siftColor",   false); // <--- Read the UI checkbox

    cv::Mat src = m_state.getImageA();
    double timingMs = 0.0;

    // Call SIFT with the newly added color boolean
    auto [result, keypoints, descriptors] = SIFTDescriptor::describe(
        src, nFeat, nOctave, contrast, edge, sigma, color, timingMs);

    m_state.setOutput(result);
    m_window->getPanelOut()->displayImage(result);
    m_window->getPanelOut()->setKeyPoints(keypoints);
    m_window->getPanelOut()->setTimingMs(timingMs);
    m_window->getPanelA()->setTimingMs(timingMs);

    QString extra = QString("Descriptor size: %1 × %2")
        .arg(descriptors.rows).arg(descriptors.cols);
        
    // Pass the 'color' boolean instead of 'false' so the HTML sidebar updates properly
    showDetectionReport("SIFT", (int)keypoints.size(), timingMs, color, extra);
    m_window->setStatusMessage(
        QString("SIFT: %1 kp in %2 ms ✓").arg(keypoints.size()).arg(timingMs, 0, 'f', 1),
        true);
}

// ── Module 4 — SSD Matching ──────────────────────────────────────────────────
void AppController::runSSD() {
    auto* pBox = m_window->getTopTaskBar()->getParameterBox();

    int    topK       = pBox->intValue("ssdTopK",      50);
    double ratio      = pBox->dblValue("ssdRatio",     0.75);
    bool   crossCheck = pBox->boolValue("ssdCrossCheck",false);
    int    vizMode    = pBox->comboIndex("ssdViz",      0);

    cv::Mat imgA = m_state.getImageA();
    cv::Mat imgB = m_state.getImageB();
    double timingMs = 0.0;

    auto [result, matches] = SSDMatcher::match(
        imgA, imgB, topK, ratio, crossCheck, vizMode, timingMs);

    m_state.setOutput(result);
    m_window->getPanelOut()->displayImage(result);
    m_window->getPanelOut()->setTimingMs(timingMs);

    showMatchingReport("SSD", (int)matches.size(), timingMs);
    m_window->setStatusMessage(
        QString("SSD: %1 matches in %2 ms ✓").arg(matches.size()).arg(timingMs, 0, 'f', 1),
        true);
}

// ── Module 5 — NCC Matching ──────────────────────────────────────────────────
void AppController::runNCC() {
    auto* pBox = m_window->getTopTaskBar()->getParameterBox();

    int    topK       = pBox->intValue("nccTopK",      50);
    double thresh     = pBox->dblValue("nccThresh",    0.70);
    bool   crossCheck = pBox->boolValue("nccCrossCheck",false);
    int    vizMode    = pBox->comboIndex("nccViz",      0);

    cv::Mat imgA = m_state.getImageA();
    cv::Mat imgB = m_state.getImageB();
    double timingMs = 0.0;

    auto [result, matches] = NCCMatcher::match(
        imgA, imgB, topK, thresh, crossCheck, vizMode, timingMs);

    m_state.setOutput(result);
    m_window->getPanelOut()->displayImage(result);
    m_window->getPanelOut()->setTimingMs(timingMs);

    showMatchingReport("NCC", (int)matches.size(), timingMs);
    m_window->setStatusMessage(
        QString("NCC: %1 matches in %2 ms ✓").arg(matches.size()).arg(timingMs, 0, 'f', 1),
        true);
}

// ── Clear ─────────────────────────────────────────────────────────────────────
void AppController::handleClear() {
    m_window->getPanelOut()->clear();
    m_window->getPanelA()->clearKeyPoints();
    m_window->getPanelA()->clearTiming();
    m_window->getPanelB()->clearKeyPoints();
    m_window->getPanelB()->clearTiming();
    m_state.clearOutput();
    m_window->setStatusMessage("Outputs cleared", true);
    m_window->updateLayoutForTask(m_currentTask);
}

// ── Save ──────────────────────────────────────────────────────────────────────
void AppController::handleSave() {
    cv::Mat out = m_state.getOutput();
    if (out.empty()) {
        m_window->setStatusMessage("Nothing to save", false);
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        m_window, "Save Result", "",
        "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");
    if (!path.isEmpty()) {
        cv::imwrite(path.toStdString(), out);
        m_window->setStatusMessage("Saved ✓", true);
    }
}

// ── HTML report builders ──────────────────────────────────────────────────────
void AppController::showDetectionReport(const QString& methodName,
                                         int kpCount, double timingMs,
                                         bool isColor, const QString& extra) {
    QString timingStr = timingMs < 1000.0
        ? QString("%1 ms").arg(timingMs, 0, 'f', 2)
        : QString("%1 s").arg(timingMs / 1000.0, 0, 'f', 3);

    QString colorStr = isColor ? "Color (3-ch)" : "Grayscale";

    QString extraHtml;
    if (!extra.isEmpty()) {
        extraHtml = QString(R"(
        <div class='card'>
            <h3>Descriptor Info</h3>
            <p>%1</p>
        </div>)").arg(extra);
    }

    QString html = QString(R"(
<style>
  body { font-family: 'DM Sans', sans-serif; color: #2C2825; margin: 0; padding: 0; }
  .card { background: #FFFFFF; border: 1px solid #E6E0F7; border-radius: 12px;
          padding: 14px; margin-bottom: 10px; }
  h3 { font-size: 14px; font-weight: 800; color: #2C2825; margin: 0 0 6px; }
  p  { font-size: 12px; color: #7A7268; line-height: 1.7; margin: 4px 0; }
  .badge      { display:inline-block; background:#EDE8FF; color:#5B4FCF;
                border-radius:6px; padding:3px 9px; font-size:11px; font-weight:800; margin:2px; }
  .badge-gold { display:inline-block; background:#FEF3C7; color:#B45309;
                border-radius:6px; padding:3px 9px; font-size:11px; font-weight:800; margin:2px; }
  .badge-sage { display:inline-block; background:#E8F5F0; color:#2D9B6F;
                border-radius:6px; padding:3px 9px; font-size:11px; font-weight:800; margin:2px; }
  .row { display:flex; gap:6px; flex-wrap:wrap; margin-top:8px; }
</style>
<div class='card'>
  <h3>%1</h3>
  <p>Feature detection results</p>
</div>
<div class='card'>
  <h3>Metrics</h3>
  <div class='row'>
    <span class='badge'>⬡ %2 keypoints</span>
    <span class='badge-gold'>⏱ %3</span>
    <span class='badge-sage'>%4</span>
  </div>
</div>
%5
<div class='card'>
  <h3>Color Legend</h3>
  <p>● <span style='color:#2D9B6F;font-weight:700;'>Green</span> — small (&lt;10 px)</p>
  <p>● <span style='color:#5B4FCF;font-weight:700;'>Purple</span> — medium (10–25 px)</p>
  <p>● <span style='color:#D85A30;font-weight:700;'>Coral</span> — large (&gt;25 px)</p>
  <p style='margin-top:8px;font-size:11px;color:#C4BDB4;'>Line indicates keypoint orientation.</p>
</div>
)").arg(methodName)
   .arg(kpCount)
   .arg(timingStr)
   .arg(colorStr)
   .arg(extraHtml);

    if (auto* sb = m_window->getInfoSidebar())
        sb->setHtml(html);
}

void AppController::showMatchingReport(const QString& methodName,
                                        int matchCount, double timingMs,
                                        const QString& extra) {
    QString timingStr = timingMs < 1000.0
        ? QString("%1 ms").arg(timingMs, 0, 'f', 2)
        : QString("%1 s").arg(timingMs / 1000.0, 0, 'f', 3);

    QString extraHtml;
    if (!extra.isEmpty())
        extraHtml = QString("<div class='card'><p>%1</p></div>").arg(extra);

    QString html = QString(R"(
<style>
  body { font-family: 'DM Sans', sans-serif; color: #2C2825; margin: 0; padding: 0; }
  .card { background: #FFFFFF; border: 1px solid #E6E0F7; border-radius: 12px;
          padding: 14px; margin-bottom: 10px; }
  h3 { font-size: 14px; font-weight: 800; color: #2C2825; margin: 0 0 6px; }
  p  { font-size: 12px; color: #7A7268; line-height: 1.7; margin: 4px 0; }
  .badge      { display:inline-block; background:#EDE8FF; color:#5B4FCF;
                border-radius:6px; padding:3px 9px; font-size:11px; font-weight:800; margin:2px; }
  .badge-gold { display:inline-block; background:#FEF3C7; color:#B45309;
                border-radius:6px; padding:3px 9px; font-size:11px; font-weight:800; margin:2px; }
  .row { display:flex; gap:6px; flex-wrap:wrap; margin-top:8px; }
</style>
<div class='card'>
  <h3>%1 Matching</h3>
  <p>Pairwise descriptor matching results</p>
</div>
<div class='card'>
  <h3>Metrics</h3>
  <div class='row'>
    <span class='badge'>⟺ %2 matches</span>
    <span class='badge-gold'>⏱ %3</span>
  </div>
</div>
%4
<div class='card'>
  <h3>About %1</h3>
  <p>%5</p>
</div>
)").arg(methodName)
   .arg(matchCount)
   .arg(timingStr)
   .arg(extraHtml)
   .arg(methodName == "SSD"
       ? "Sum of Squared Differences measures descriptor similarity by summing pixel-wise squared differences. Lower scores = better matches."
       : "Normalized Cross-Correlation measures the cosine similarity between descriptor vectors. Scores near 1.0 indicate strong matches.");

    if (auto* sb = m_window->getInfoSidebar())
        sb->setHtml(html);
}