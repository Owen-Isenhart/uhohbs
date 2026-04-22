#pragma once

#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>
#include <QColor>
#include <memory>

#include "dump.hpp"

class dump_dock : public QDockWidget {
Q_OBJECT

public:
    explicit dump_dock(QWidget *parent = nullptr);
    ~dump_dock() override = default;

    void trigger_dump_from_hotkey();

signals:
    void dump_requested();

public slots:
    void TriggerDump();

private slots:
    void HandleDump();
    void OnModeChanged(int index);
    void OnFillTypeChanged(int index);
    void OnPipelineTargetChanged(int index);
    void OnFillColorChanged(const QString &text);

private:
    dump_config BuildConfigFromUi() const;
    void ApplyConfigToUi(const dump_config &config);
    void UpdateUiState();
    void UpdateModeHint();
    void UpdateColorPreview(const QString &text);
    QString NormalizeHexColor(QString value) const;
    void SetStatus(const QString &text, bool isError, bool inProgress);
    void LoadPersistedSettings();
    void SavePersistedSettings() const;

    QSpinBox *delayInput{nullptr};
    QComboBox *modeInput{nullptr};
    QComboBox *fillTypeInput{nullptr};
    QLineEdit *fillTargetInput{nullptr};
    QLineEdit *fillColorInput{nullptr};
    QLabel *fillColorPreview{nullptr};
    QComboBox *pipelineTargetInput{nullptr};
    QLabel *modeHintLabel{nullptr};
    QPushButton *dumpButton{nullptr};
    QLabel *statusLabel{nullptr};
    QColor statusDefaultColor;

    std::shared_ptr<dump_coordinator> coordinator;
};