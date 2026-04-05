#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QList>
#include <QTextBrowser>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStackedWidget>

#include "components/TopTaskBar.h"
#include "components/ImagePanel.h"

class AppController;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // ── Accessors used by AppController ──────────────────────────────
    TopTaskBar*   getTopTaskBar()    { return m_taskBar; }
    ImagePanel*   getPanelA()        { return m_panelA; }
    ImagePanel*   getPanelB()        { return m_panelB; }
    ImagePanel*   getPanelOut()      { return m_panelOut; }
    QTextBrowser* getInfoSidebar()   { return m_infoSidebar; }

    void updateLayoutForTask(int taskIndex);
    void setStatusMessage(const QString& msg, bool success = true);

private:
    // ── Build helpers ─────────────────────────────────────────────────
    void buildLeftRail();
    void buildHeaderBar();
    void buildCanvasArea();
    void setActiveTask(int taskIndex);

    // ── Widgets ───────────────────────────────────────────────────────
    QWidget*      m_leftRail        = nullptr;
    QWidget*      m_mainArea        = nullptr;
    QWidget*      m_headerBar       = nullptr;
    TopTaskBar*   m_taskBar         = nullptr;
    QWidget*      m_canvasArea      = nullptr;

    QSplitter*    m_contentSplitter = nullptr;
    QSplitter*    m_inputSplitter   = nullptr;   // left: input panels
    QWidget*      m_inputContainer  = nullptr;
    QWidget*      m_outputContainer = nullptr;

    // Three panels: A (always visible), B (only for matching tasks), Out
    ImagePanel*   m_panelA          = nullptr;
    ImagePanel*   m_panelB          = nullptr;
    ImagePanel*   m_panelOut        = nullptr;

    QTextBrowser* m_infoSidebar     = nullptr;

    QList<QPushButton*> m_taskButtons;

    AppController* m_controller     = nullptr;

    // Track which tasks need 2 inputs
    bool taskNeedsTwoInputs(int idx) const { return idx == 4 || idx == 5; }
};

#endif // MAINWINDOW_H