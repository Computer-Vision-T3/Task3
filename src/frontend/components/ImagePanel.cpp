#include "ImagePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QUrl>
#include <cmath>

// ── Constructor ──────────────────────────────────────────────────────────────
ImagePanel::ImagePanel(const QString& title, PanelRole role, QWidget* parent)
    : QWidget(parent), m_role(role)
{
    setObjectName("imagePanel");
    setAcceptDrops(true);
    buildUi();
    m_titleLabel->setText(title);
}

// ── Build UI ─────────────────────────────────────────────────────────────────
void ImagePanel::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // ── Header row ───────────────────────────────────────────────────
    QWidget* header = new QWidget(this);
    header->setObjectName("imagePanelHeader");
    QHBoxLayout* hdr = new QHBoxLayout(header);
    hdr->setContentsMargins(0, 0, 0, 8);
    hdr->setSpacing(6);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("panelTitle");

    m_badgeLabel = new QLabel(this);
    switch (m_role) {
        case PanelRole::InputA:
            m_badgeLabel->setObjectName("panelBadgeInput");
            m_badgeLabel->setText("IMAGE A");
            break;
        case PanelRole::InputB:
            m_badgeLabel->setObjectName("panelBadgeInput2");
            m_badgeLabel->setText("IMAGE B");
            break;
        case PanelRole::Output:
            m_badgeLabel->setObjectName("panelBadge");
            m_badgeLabel->setText("OUTPUT");
            break;
    }

    m_resLabel = new QLabel(this);
    m_resLabel->setStyleSheet("color: #C4BDB4; font-size: 10px; font-weight: 600;");

    m_kpCountLabel = new QLabel(this);
    m_kpCountLabel->setObjectName("keyPointCountLabel");
    m_kpCountLabel->hide();

    m_timingLabel = new QLabel(this);
    m_timingLabel->setObjectName("timingLabel");
    m_timingLabel->hide();

    hdr->addWidget(m_titleLabel);
    hdr->addWidget(m_badgeLabel);
    hdr->addStretch();
    hdr->addWidget(m_kpCountLabel);
    hdr->addWidget(m_timingLabel);
    hdr->addWidget(m_resLabel);

    // Load button (inputs only)
    if (m_role != PanelRole::Output) {
        m_loadBtn = new QPushButton("Load Image", this);
        m_loadBtn->setObjectName("loadBtn");
        m_loadBtn->setCursor(Qt::PointingHandCursor);
        m_loadBtn->setToolTip("Open image file (or drag & drop)");
        connect(m_loadBtn, &QPushButton::clicked, this, &ImagePanel::handleLoad);
        hdr->addWidget(m_loadBtn);

        m_clearKpBtn = new QPushButton("Clear KP", this);
        m_clearKpBtn->setObjectName("clearKpBtn");
        m_clearKpBtn->setCursor(Qt::PointingHandCursor);
        m_clearKpBtn->setToolTip("Remove keypoint overlay");
        m_clearKpBtn->hide();
        connect(m_clearKpBtn, &QPushButton::clicked, this, &ImagePanel::handleClearKeypoints);
        hdr->addWidget(m_clearKpBtn);
    }

    // ── Image display area ───────────────────────────────────────────
    m_imageDisplay = new QLabel(this);
    m_imageDisplay->setObjectName("imageDisplay");
    m_imageDisplay->setAlignment(Qt::AlignCenter);
    m_imageDisplay->setMinimumSize(200, 200);
    m_imageDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (m_role == PanelRole::Output) {
        m_imageDisplay->setText("Waiting for processing…");
    } else {
        m_imageDisplay->setText("Drop image here\nor click Load Image");
    }

    root->addWidget(header);
    root->addWidget(m_imageDisplay, 1);
}

// ── Public API ────────────────────────────────────────────────────────────────
cv::Mat ImagePanel::getImage() const { return m_image.clone(); }
bool    ImagePanel::hasImage() const { return !m_image.empty(); }

void ImagePanel::displayImage(const cv::Mat& img) {
    if (img.empty()) { clear(); return; }
    m_image = img.clone();
    if (m_resLabel) {
        m_resLabel->setText(QString("%1 × %2").arg(img.cols).arg(img.rows));
    }
    refreshDisplay();
    emit imageLoaded(m_image);
}

void ImagePanel::clear() {
    m_image.release();
    m_keypoints.clear();
    m_timingMs = -1.0;
    if (m_resLabel)    m_resLabel->clear();
    if (m_kpCountLabel){ m_kpCountLabel->hide(); }
    if (m_timingLabel) { m_timingLabel->hide(); }
    if (m_clearKpBtn)  { m_clearKpBtn->hide(); }
    m_imageDisplay->clear();
    m_imageDisplay->setText(m_role == PanelRole::Output
        ? "Waiting for processing…"
        : "Drop image here\nor click Load Image");
}

void ImagePanel::setKeyPoints(const std::vector<cv::KeyPoint>& kps) {
    m_keypoints = kps;
    if (m_kpCountLabel && !kps.empty()) {
        m_kpCountLabel->setText(QString("⬡ %1 kp").arg((int)kps.size()));
        m_kpCountLabel->show();
    }
    if (m_clearKpBtn && !kps.empty()) m_clearKpBtn->show();
    refreshDisplay();
}

void ImagePanel::clearKeyPoints() {
    m_keypoints.clear();
    if (m_kpCountLabel) m_kpCountLabel->hide();
    if (m_clearKpBtn)   m_clearKpBtn->hide();
    refreshDisplay();
}

void ImagePanel::setTimingMs(double ms) {
    m_timingMs = ms;
    if (m_timingLabel) {
        if (ms < 1000.0)
            m_timingLabel->setText(QString("⏱ %1 ms").arg(ms, 0, 'f', 1));
        else
            m_timingLabel->setText(QString("⏱ %1 s").arg(ms / 1000.0, 0, 'f', 2));
        m_timingLabel->show();
    }
}

void ImagePanel::clearTiming() {
    m_timingMs = -1.0;
    if (m_timingLabel) m_timingLabel->hide();
}

void ImagePanel::setKeyPointCount(int n) {
    if (m_kpCountLabel) {
        m_kpCountLabel->setText(QString("⬡ %1 kp").arg(n));
        m_kpCountLabel->setVisible(n > 0);
    }
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void ImagePanel::handleLoad() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Image", "",
        "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.webp);;All Files (*)"
    );
    if (!path.isEmpty()) loadFromPath(path);
}

void ImagePanel::handleClearKeypoints() {
    clearKeyPoints();
}

// ── Drag & Drop ───────────────────────────────────────────────────────────────
void ImagePanel::dragEnterEvent(QDragEnterEvent* e) {
    if (m_role != PanelRole::Output && e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void ImagePanel::dropEvent(QDropEvent* e) {
    if (m_role == PanelRole::Output) return;
    const QList<QUrl> urls = e->mimeData()->urls();
    if (!urls.isEmpty()) loadFromPath(urls.first().toLocalFile());
}

void ImagePanel::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    refreshDisplay();
}

// ── Private helpers ───────────────────────────────────────────────────────────
void ImagePanel::loadFromPath(const QString& path) {
    cv::Mat img = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (img.empty()) {
        QMessageBox::warning(this, "Load Error",
            QString("Could not open:\n%1").arg(path));
        return;
    }
    m_keypoints.clear();
    if (m_kpCountLabel) m_kpCountLabel->hide();
    if (m_clearKpBtn)   m_clearKpBtn->hide();
    if (m_timingLabel)  m_timingLabel->hide();
    m_timingMs = -1.0;
    displayImage(img);
}

void ImagePanel::refreshDisplay() {
    if (m_image.empty() || !m_imageDisplay) return;
    QPixmap px = renderComposite();
    px = px.scaled(m_imageDisplay->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageDisplay->setPixmap(px);
}

QPixmap ImagePanel::renderComposite() const {
    // Convert base image
    QImage base = matToQImage(m_image);
    QPixmap pm = QPixmap::fromImage(base);

    if (m_keypoints.empty()) return pm;

    // Render keypoints on top
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Color-code by size: small=teal, medium=violet, large=coral
    for (const auto& kp : m_keypoints) {
        float sz = std::max(2.0f, kp.size * 0.5f);
        sz = std::min(sz, 15.0f);

        QColor fill, stroke;
        if (kp.size < 10.0f) {
            fill = QColor(45, 155, 111, 200);    // sage green
            stroke = QColor(20, 100, 70, 255);
        } else if (kp.size < 25.0f) {
            fill = QColor(91, 79, 207, 200);      // purple
            stroke = QColor(60, 50, 160, 255);
        } else {
            fill = QColor(216, 90, 48, 200);      // coral
            stroke = QColor(150, 60, 25, 255);
        }

        p.setPen(QPen(stroke, 1.2f));
        p.setBrush(QBrush(fill));
        p.drawEllipse(QPointF(kp.pt.x, kp.pt.y), sz, sz);

        // Orientation line
        if (kp.angle >= 0.0f) {
            float rad = kp.angle * static_cast<float>(M_PI) / 180.0f;
            float ex = kp.pt.x + sz * std::cos(rad);
            float ey = kp.pt.y + sz * std::sin(rad);
            p.setPen(QPen(stroke, 1.0f));
            p.drawLine(QPointF(kp.pt.x, kp.pt.y), QPointF(ex, ey));
        }
    }
    p.end();
    return pm;
}

QImage ImagePanel::matToQImage(const cv::Mat& mat) const {
    if (mat.type() == CV_8UC1) {
        QImage img(mat.data, mat.cols, mat.rows,
                   static_cast<int>(mat.step), QImage::Format_Grayscale8);
        return img.copy();
    } else if (mat.type() == CV_8UC3) {
        QImage img(mat.data, mat.cols, mat.rows,
                   static_cast<int>(mat.step), QImage::Format_BGR888);
        return img.copy();
    } else if (mat.type() == CV_8UC4) {
        QImage img(mat.data, mat.cols, mat.rows,
                   static_cast<int>(mat.step), QImage::Format_ARGB32);
        return img.copy();
    }
    return {};
}