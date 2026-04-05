#ifndef PARAMETERBOX_H
#define PARAMETERBOX_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>

class ParameterBox : public QWidget {
    Q_OBJECT
public:
    explicit ParameterBox(QWidget* parent = nullptr);

    // Called whenever the active module changes (1-based index)
    void updateForTask(int taskIndex);

    // Generic value accessors (used by AppController)
    int    intValue(const QString& id, int fallback = 0)       const;
    double dblValue(const QString& id, double fallback = 0.0)  const;
    bool   boolValue(const QString& id, bool fallback = false) const;
    int    comboIndex(const QString& id, int fallback = 0)     const;

signals:
    void quickActionRequested(int taskIndex);

private:
    QGridLayout* m_layout = nullptr;
    void clearLayout();

    // Helpers for building parameter widgets
    void addSpinBox(const QString& id, const QString& label,
                    int min, int max, int def, int row, int col);
    void addDoubleSpinBox(const QString& id, const QString& label,
                          double min, double max, double def,
                          double step, int row, int col);
    void addSlider(const QString& id, const QString& label,
                   int min, int max, int def,
                   int row, int col, bool isFloat = false);
    void addCombo(const QString& id, const QString& label,
                  const QStringList& items, int row, int col);
    void addCheckBox(const QString& id, const QString& label,
                     bool def, int row, int col);
    void addButton(const QString& text, int targetTask, int row, int col);
};

#endif // PARAMETERBOX_H