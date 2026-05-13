#include "dump_dock.hpp"

#include <QMetaObject>
#include <QPointer>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>

#include <thread>

#include <obs-module.h>
#include <obs.h>

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

	statusLabel = new QLabel(tr_key("status.ready"), container);
	statusLabel->setWordWrap(true);
	statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	rootLayout->addWidget(statusLabel);
	rootLayout->addStretch(1);
	statusDefaultColor = statusLabel->palette().color(statusLabel->foregroundRole());

	container->setStyleSheet("QWidget#uhohbs_root { background: palette(base); }"
				 "QPushButton#uhohbs_dump_button { letter-spacing: 0.2px; }");

	setWidget(container);

	connect(dumpButton, &QPushButton::clicked, this, &dump_dock::HandleDump);
	connect(this, &dump_dock::dump_requested, this, &dump_dock::TriggerDump);

	QPointer<dump_dock> self(this);
	coordinator->set_status_callback([self](const dump_result &result) {
		if (!self) {
			return;
		}

		QMetaObject::invokeMethod(self, [self, result]() {
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

			self->SetStatus(QString::fromStdString(result.message), true, false);
		});
	});

}

void dump_dock::trigger_dump_from_hotkey()
{
	emit dump_requested();
}

void dump_dock::TriggerDump()
{
	HandleDump();
}

void dump_dock::HandleDump()
{
	const auto coordinatorRef = coordinator;
	std::thread([coordinatorRef]() { coordinatorRef->request_dump(); }).detach();
}

void dump_dock::SetStatus(const QString &text, bool isError, bool inProgress)
{
	statusLabel->setText(text);
	statusLabel->setStyleSheet(isError ? "color: #d64b4b;" : QString("color: %1;").arg(statusDefaultColor.name()));
	dumpButton->setEnabled(!inProgress);
}
