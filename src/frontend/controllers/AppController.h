#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QString>
#include "../MainWindow.h"
#include "ImageStateManager.h"

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow* window, QObject* parent = nullptr);

private slots:
    void handleTaskChange(int taskIndex);
    void handleApply();
    void handleClear();
    void handleSave();

private:
    // ── Helpers that call backend modules ─────────────────────────────
    void runHarris();
    void runLambda();
    void runSIFT();
    void runSSD();
    void runNCC();

    // ── Result display ─────────────────────────────────────────────────
    void showDetectionReport(const QString& methodName,
                             int kpCount,
                             double timingMs,
                             bool isColor,
                             const QString& extra = {});
    void showMatchingReport(const QString& methodName,
                            int matchCount,
                            double timingMs,
                            const QString& extra = {});

    MainWindow*       m_window  = nullptr;
    ImageStateManager m_state;
    int               m_currentTask = 1;
};

#endif // APPCONTROLLER_H 
