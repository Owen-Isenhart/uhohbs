#include "dump_dock.hpp"

#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>


#include <thread>

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.h>
#include <util/config-file.h>

namespace {
QString prettify_missing_key(const QString &key)
{
	QString token = key;
	const int idx = token.lastIndexOf('.');
	if (idx >= 0 && idx + 1 < token.size()) {
		token = token.mid(idx + 1);
	}

	token.replace('_', ' ');
	QStringList words = token.split(' ', Qt::SkipEmptyParts);
	for (QString &word : words) {
		if (!word.isEmpty()) {
			word = word.left(1).toUpper() + word.mid(1).toLower();
		}
	}

	if (!words.isEmpty()) {
		return words.join(' ');
	}

	return key;
}

QString tr_key(const char *key)
{
	const QString keyText = QString::fromUtf8(key);
	const QString localized = QString::fromUtf8(obs_module_text(key));
	if (localized == keyText) {
		return prettify_missing_key(keyText);
	}

	return localized;
}
} // namespace

dump_dock::dump_dock(QWidget *parent) : QDockWidget(parent)
{
	coordinator = std::make_shared<dump_coordinator>();

	setWindowTitle(tr_key("dock.window_title"));
	setObjectName("uhohbs_dock");

	auto *container = new QWidget(this);
	container->setObjectName("uhohbs_root");
	auto *rootLayout = new QVBoxLayout(container);
	rootLayout->setContentsMargins(4, 4, 4, 4);
	rootLayout->setSpacing(4);
	rootLayout->setAlignment(Qt::AlignTop);

	dumpButton = new QPushButton(tr_key("dock.dump_button"), container);
	dumpButton->setObjectName("uhohbs_dump_button");
	dumpButton->setContentsMargins(0, 0, 0, 0);
	dumpButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	dumpButton->setToolTip(tr_key("dock.dump_button.help"));
	rootLayout->addWidget(dumpButton);

	skipDelayCheckbox = new QCheckBox(tr_key("dock.skip_delay"), container);
	skipDelayCheckbox->setObjectName("uhohbs_skip_delay_checkbox");
	skipDelayCheckbox->setToolTip(tr_key("dock.skip_delay.help"));
	if (config_t *profile = obs_frontend_get_profile_config()) {
		skipDelayCheckbox->setChecked(config_get_bool(profile, "Uhohbs", "SkipDelay"));
	}
	connect(skipDelayCheckbox, &QCheckBox::toggled, [](bool checked) {
		if (config_t *profile = obs_frontend_get_profile_config()) {
			config_set_bool(profile, "Uhohbs", "SkipDelay", checked);
			config_save(profile);
		}
	});
	rootLayout->addWidget(skipDelayCheckbox);

	statusLabel = new QLabel(tr_key("status.ready"), container);
	statusLabel->setWordWrap(true);
	statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	rootLayout->addWidget(statusLabel);
	rootLayout->addStretch(1);


	container->setStyleSheet("QWidget#uhohbs_root { background: palette(base); }"
				 "QPushButton#uhohbs_dump_button { letter-spacing: 0.2px; }");

	setWidget(container);

	connect(dumpButton, &QPushButton::clicked, this, &dump_dock::HandleDump);
	connect(this, &dump_dock::dump_requested, this, &dump_dock::TriggerDump);

	QPointer<dump_dock> self(this);
	coordinator->set_status_callback([self](const dump_result &result) {
		auto *app = QCoreApplication::instance();
		if (!app) {
			return;
		}

		QMetaObject::invokeMethod(app, [self, result]() {
			if (!self) {
				return;
			}

			if (result.type == dump_result_type::InProgress) {
				self->SetStatus(tr_key("status.in_progress"), false, true);
				return;
			}

			if (result.type == dump_result_type::Success) {
				self->SetStatus(tr_key("status.success"), false, false);
				return;
			}

			self->SetStatus(tr_key(result.message.c_str()), true, false);
		}, Qt::QueuedConnection);
	});
}

dump_dock::~dump_dock()
{
	if (!shutdownRequested.exchange(true)) {
		coordinator->request_cancel();
		if (dumpThread.joinable()) {
			dumpThread.join();
		}
	}
}

void dump_dock::trigger_dump_from_hotkey()
{
	emit dump_requested();
}

void dump_dock::TriggerDump()
{
	HandleDump();
}

void dump_dock::shutdown()
{
	if (shutdownRequested.exchange(true)) {
		return;
	}

	coordinator->request_cancel();
	if (dumpThread.joinable()) {
		dumpThread.join();
	}
}

void dump_dock::HandleDump()
{
	if (shutdownRequested.load()) {
		return;
	}

	if (dumpThreadActive->exchange(true)) {
		return;
	}

	const bool skipDelay = skipDelayCheckbox->isChecked();
	if (dumpThread.joinable()) {
		dumpThread.join();
	}

	const auto coordinatorRef = coordinator;
	const auto activeFlag = dumpThreadActive;
	dumpThread = std::thread([coordinatorRef, skipDelay, activeFlag]() {
		(void)coordinatorRef->request_dump(skipDelay);
		activeFlag->store(false);
	});
}

void dump_dock::SetStatus(const QString &text, bool isError, bool inProgress)
{
	statusLabel->setText(text);
	statusLabel->setStyleSheet(isError ? "color: #d64b4b;" : "");
	dumpButton->setEnabled(!inProgress);
	skipDelayCheckbox->setEnabled(!inProgress);
}
