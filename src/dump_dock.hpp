#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
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

private:
	void SetStatus(const QString &text, bool isError, bool inProgress);

	QPushButton *dumpButton{nullptr};
	QLabel *statusLabel{nullptr};
	QColor statusDefaultColor;

	std::shared_ptr<dump_coordinator> coordinator;
};
