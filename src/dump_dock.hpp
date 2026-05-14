#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QWidget>

#include <atomic>
#include <memory>
#include <thread>

#include "dump.hpp"

class dump_dock : public QDockWidget {
	Q_OBJECT

public:
	explicit dump_dock(QWidget *parent = nullptr);
	~dump_dock() override;

	void trigger_dump_from_hotkey();
	void shutdown();

signals:
	void dump_requested();

public slots:
	void TriggerDump();

private slots:
	void HandleDump();

private:
	void SetStatus(const QString &text, bool isError, bool inProgress);

	QPushButton *dumpButton{nullptr};
	QCheckBox *skipDelayCheckbox{nullptr};
	QLabel *statusLabel{nullptr};


	std::shared_ptr<dump_coordinator> coordinator;
	std::thread dumpThread;
	std::shared_ptr<std::atomic<bool>> dumpThreadActive{std::make_shared<std::atomic<bool>>(false)};
	std::atomic<bool> shutdownRequested{false};
};
