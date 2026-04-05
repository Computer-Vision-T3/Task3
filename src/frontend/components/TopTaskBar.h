#ifndef TOPTASKBAR_H
#define TOPTASKBAR_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "ParameterBox.h"

class TopTaskBar : public QWidget {
    Q_OBJECT
public:
    explicit TopTaskBar(QWidget* parent = nullptr);

    int           getSelectedOperation() const;  // 1-based
    ParameterBox* getParameterBox()      { return m_paramBox; }

    void setProcessing(bool on);
    void setStatus(const QString& msg, bool success = true);

signals:
    void taskChanged(int taskIndex);   // 1-based
    void applyRequested();
    void clearRequested();
    void saveRequested();

private slots:
    void onSelectionChanged(int index);

private:
    QComboBox*    m_opSelector = nullptr;
    ParameterBox* m_paramBox   = nullptr;
    QPushButton*  m_applyBtn   = nullptr;
    QPushButton*  m_clearBtn   = nullptr;
    QPushButton*  m_saveBtn    = nullptr;
    QLabel*       m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
};

#endif // TOPTASKBAR_H