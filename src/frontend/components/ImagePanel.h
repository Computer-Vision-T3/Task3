#ifndef IMAGEPANEL_H
#define IMAGEPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <opencv2/opencv.hpp>
#include <vector>

// ── Enum for panel role ──────────────────────────────────────────────
enum class PanelRole { InputA, InputB, Output };

class ImagePanel : public QWidget {
    Q_OBJECT

public:
    explicit ImagePanel(const QString& title,
                        PanelRole role = PanelRole::InputA,
                        QWidget* parent = nullptr);

    // ── Image access ─────────────────────────────────────────────────
    cv::Mat getImage() const;
    bool    hasImage() const;
    void    displayImage(const cv::Mat& img);
    void    clear();

    // ── Keypoints overlay ────────────────────────────────────────────
    void setKeyPoints(const std::vector<cv::KeyPoint>& kps);
    void clearKeyPoints();
    std::vector<cv::KeyPoint> getKeyPoints() const { return m_keypoints; }

    // ── Timing badge ─────────────────────────────────────────────────
    void setTimingMs(double ms);
    void clearTiming();

    // ── Stat counters ─────────────────────────────────────────────────
    void setKeyPointCount(int n);

    PanelRole role() const { return m_role; }

signals:
    void imageLoaded(const cv::Mat& img);

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private slots:
    void handleLoad();
    void handleClearKeypoints();

private:
    // ── Helpers ──────────────────────────────────────────────────────
    void     buildUi();
    void     refreshDisplay();
    QPixmap  renderComposite() const;
    QImage   matToQImage(const cv::Mat& mat) const;
    void     loadFromPath(const QString& path);

    // ── State ─────────────────────────────────────────────────────────
    PanelRole                  m_role;
    cv::Mat                    m_image;
    std::vector<cv::KeyPoint>  m_keypoints;
    double                     m_timingMs = -1.0;

    // ── Widgets ───────────────────────────────────────────────────────
    QLabel*      m_titleLabel        = nullptr;
    QLabel*      m_badgeLabel        = nullptr;
    QLabel*      m_resLabel          = nullptr;
    QLabel*      m_kpCountLabel      = nullptr;
    QLabel*      m_timingLabel       = nullptr;
    QLabel*      m_imageDisplay      = nullptr;
    QPushButton* m_loadBtn           = nullptr;
    QPushButton* m_clearKpBtn        = nullptr;
};

#endif // IMAGEPANEL_H