#include "dump_dock.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMetaObject>
#include <QPointer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
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
	if (key == "dock.delay.suffix") {
		return " s";
	}
	if (key == "dock.fill_target.placeholder") {
		return "Name of source or scene";
	}

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
	auto *rootLayout = new QVBoxLayout(container);
	rootLayout->setContentsMargins(10, 10, 10, 10);
	rootLayout->setSpacing(10);
	rootLayout->setAlignment(Qt::AlignTop);

	auto *operationGroup = new QGroupBox(tr_key("dock.group.operation"), container);
	auto *operationLayout = new QFormLayout(operationGroup);
	operationLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	delayInput = new QSpinBox(container);
	delayInput->setRange(1, 300);
	delayInput->setValue(5);
	delayInput->setSuffix(tr_key("dock.delay.suffix"));
	delayInput->setToolTip(tr_key("dock.delay.help"));
	operationLayout->addRow(tr_key("dock.delay"), delayInput);

	modeInput = new QComboBox(container);
	modeInput->addItem(tr_key("dock.mode.cut"), static_cast<int>(dump_mode::Mode::Cut));
	modeInput->addItem(tr_key("dock.mode.fill"), static_cast<int>(dump_mode::Mode::Fill));
	modeInput->setToolTip(tr_key("dock.mode.help"));
	operationLayout->addRow(tr_key("dock.mode"), modeInput);

	pipelineTargetInput = new QComboBox(container);
	pipelineTargetInput->addItem(tr_key("dock.pipeline.stream_delay"),
				     static_cast<int>(pipeline_target::StreamDelay));
	pipelineTargetInput->addItem(tr_key("dock.pipeline.replay_buffer"),
				     static_cast<int>(pipeline_target::ReplayBuffer));
	pipelineTargetInput->setToolTip(tr_key("dock.pipeline.help"));
	operationLayout->addRow(tr_key("dock.pipeline"), pipelineTargetInput);

	rootLayout->addWidget(operationGroup);

	auto *fillGroup = new QGroupBox(tr_key("dock.group.fill"), container);
	auto *fillLayout = new QFormLayout(fillGroup);
	fillLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	fillTypeInput = new QComboBox(container);
	fillTypeInput->addItem(tr_key("dock.fill_type.color"), static_cast<int>(dump_mode::FillType::Color));
	fillTypeInput->addItem(tr_key("dock.fill_type.source"), static_cast<int>(dump_mode::FillType::Source));
	fillTypeInput->addItem(tr_key("dock.fill_type.scene"), static_cast<int>(dump_mode::FillType::Scene));
	fillTypeInput->setToolTip(tr_key("dock.fill_type.help"));
	fillLayout->addRow(tr_key("dock.fill_type"), fillTypeInput);

	fillTargetInput = new QLineEdit(container);
	fillTargetInput->setPlaceholderText(tr_key("dock.fill_target.placeholder"));
	fillTargetInput->setToolTip(tr_key("dock.fill_target.help"));
	fillLayout->addRow(tr_key("dock.fill_target"), fillTargetInput);

	fillColorInput = new QLineEdit(container);
	fillColorInput->setText("#ff0000");
	fillColorInput->setPlaceholderText("#ff0000");
	fillColorInput->setToolTip(tr_key("dock.fill_color.help"));
	auto *hexValidator =
		new QRegularExpressionValidator(QRegularExpression("^#?[0-9A-Fa-f]{0,6}$"), fillColorInput);
	fillColorInput->setValidator(hexValidator);

	auto *colorRow = new QWidget(container);
	auto *colorRowLayout = new QHBoxLayout(colorRow);
	colorRowLayout->setContentsMargins(0, 0, 0, 0);
	colorRowLayout->setSpacing(8);
	colorRowLayout->addWidget(fillColorInput);

	fillColorPreview = new QLabel(colorRow);
	fillColorPreview->setFixedSize(20, 20);
	fillColorPreview->setToolTip(tr_key("dock.fill_color.preview"));
	colorRowLayout->addWidget(fillColorPreview);
	fillLayout->addRow(tr_key("dock.fill_color"), colorRow);

	rootLayout->addWidget(fillGroup);

	modeHintLabel = new QLabel(container);
	modeHintLabel->setWordWrap(true);
	modeHintLabel->setObjectName("uhohbs_mode_hint");
	modeHintLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	rootLayout->addWidget(modeHintLabel);

	dumpButton = new QPushButton(tr_key("dock.dump_button"), container);
	dumpButton->setObjectName("uhohbs_dump_button");
	dumpButton->setMinimumHeight(44);
	dumpButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	dumpButton->setToolTip(tr_key("dock.dump_button.help"));
	rootLayout->addWidget(dumpButton);

	statusLabel = new QLabel(tr_key("status.ready"), container);
	statusLabel->setWordWrap(true);
	statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	rootLayout->addWidget(statusLabel);
	rootLayout->addStretch(1);
	statusDefaultColor = statusLabel->palette().color(statusLabel->foregroundRole());

	container->setStyleSheet("QGroupBox { font-weight: 600; }"
				 "QLabel#uhohbs_mode_hint { color: palette(mid); }"
				 "QPushButton#uhohbs_dump_button { font-weight: 700; letter-spacing: 0.2px; }");

	setWidget(container);

	connect(dumpButton, &QPushButton::clicked, this, &dump_dock::HandleDump);
	connect(modeInput, &QComboBox::currentIndexChanged, this, &dump_dock::OnModeChanged);
	connect(fillTypeInput, &QComboBox::currentIndexChanged, this, &dump_dock::OnFillTypeChanged);
	connect(pipelineTargetInput, &QComboBox::currentIndexChanged, this, &dump_dock::OnPipelineTargetChanged);
	connect(fillColorInput, &QLineEdit::textChanged, this, &dump_dock::OnFillColorChanged);
	connect(this, &dump_dock::dump_requested, this, &dump_dock::TriggerDump);
	connect(delayInput, &QSpinBox::valueChanged, this, [this](int) { SavePersistedSettings(); });
	connect(modeInput, &QComboBox::currentIndexChanged, this, [this](int) { SavePersistedSettings(); });
	connect(fillTypeInput, &QComboBox::currentIndexChanged, this, [this](int) { SavePersistedSettings(); });
	connect(fillTargetInput, &QLineEdit::editingFinished, this, [this]() { SavePersistedSettings(); });
	connect(fillColorInput, &QLineEdit::editingFinished, this, [this]() { SavePersistedSettings(); });
	connect(pipelineTargetInput, &QComboBox::currentIndexChanged, this, [this](int) { SavePersistedSettings(); });

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
				const QString statusText = result.usedFallback ? tr_key("status.fill_fallback")
									       : tr_key("status.success");
				self->SetStatus(statusText, false, false);
				return;
			}

			self->SetStatus(QString::fromStdString(result.message), true, false);
		});
	});

	LoadPersistedSettings();
	UpdateColorPreview(fillColorInput->text());
	UpdateUiState();
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
	const auto config = BuildConfigFromUi();
	const auto coordinatorRef = coordinator;
	std::thread([coordinatorRef, config]() { coordinatorRef->request_dump(config); }).detach();
}

void dump_dock::OnModeChanged(int)
{
	UpdateUiState();
}

void dump_dock::OnFillTypeChanged(int)
{
	UpdateUiState();
}

void dump_dock::OnPipelineTargetChanged(int)
{
	UpdateUiState();
}

void dump_dock::OnFillColorChanged(const QString &text)
{
	UpdateColorPreview(text);
}

dump_config dump_dock::BuildConfigFromUi() const
{
	dump_config config;
	config.set_delay_seconds(static_cast<std::uint16_t>(delayInput->value()));
	config.set_mode(static_cast<dump_mode::Mode>(modeInput->currentData().toInt()));
	config.set_fill_type(static_cast<dump_mode::FillType>(fillTypeInput->currentData().toInt()));
	config.set_fill_target_name(fillTargetInput->text().toStdString());
	config.set_fill_color_hex(NormalizeHexColor(fillColorInput->text()).toStdString());
	config.set_pipeline_target(static_cast<pipeline_target>(pipelineTargetInput->currentData().toInt()));
	return config;
}

void dump_dock::ApplyConfigToUi(const dump_config &config)
{
	delayInput->setValue(static_cast<int>(config.get_delay_seconds()));

	const int modeIndex = modeInput->findData(static_cast<int>(config.get_mode()));
	if (modeIndex >= 0) {
		modeInput->setCurrentIndex(modeIndex);
	}

	const int fillTypeIndex = fillTypeInput->findData(static_cast<int>(config.get_fill_type()));
	if (fillTypeIndex >= 0) {
		fillTypeInput->setCurrentIndex(fillTypeIndex);
	}

	fillTargetInput->setText(QString::fromStdString(config.get_fill_target_name()));
	fillColorInput->setText(NormalizeHexColor(QString::fromStdString(config.get_fill_color_hex())));

	const int pipelineIndex = pipelineTargetInput->findData(static_cast<int>(config.get_pipeline_target()));
	if (pipelineIndex >= 0) {
		pipelineTargetInput->setCurrentIndex(pipelineIndex);
	}
}

void dump_dock::UpdateUiState()
{
	const bool isFillMode = static_cast<dump_mode::Mode>(modeInput->currentData().toInt()) == dump_mode::Mode::Fill;
	const auto selectedFillType = static_cast<dump_mode::FillType>(fillTypeInput->currentData().toInt());
	const bool isReplayMode = static_cast<pipeline_target>(pipelineTargetInput->currentData().toInt()) ==
				  pipeline_target::ReplayBuffer;

	const bool enableFillControls = isFillMode && !isReplayMode;
	fillTypeInput->setEnabled(enableFillControls);
	fillTargetInput->setEnabled(enableFillControls && selectedFillType != dump_mode::FillType::Color);
	fillColorInput->setEnabled(enableFillControls && selectedFillType == dump_mode::FillType::Color);
	fillColorPreview->setEnabled(enableFillControls && selectedFillType == dump_mode::FillType::Color);

	UpdateModeHint();
}

void dump_dock::UpdateModeHint()
{
	const auto mode = static_cast<dump_mode::Mode>(modeInput->currentData().toInt());
	const bool isReplayMode = static_cast<pipeline_target>(pipelineTargetInput->currentData().toInt()) ==
				  pipeline_target::ReplayBuffer;

	if (isReplayMode) {
		modeHintLabel->setText(tr_key("dock.hint.replay"));
		return;
	}

	if (mode == dump_mode::Mode::Cut) {
		modeHintLabel->setText(tr_key("dock.hint.cut"));
		return;
	}

	modeHintLabel->setText(tr_key("dock.hint.fill"));
}

void dump_dock::UpdateColorPreview(const QString &text)
{
	const QColor color = ParsePreviewColor(text);
	const QString previewColor = color.isValid() ? color.name(QColor::HexRgb) : QString("#444444");
	fillColorPreview->setStyleSheet(
		QString("border: 1px solid palette(mid); border-radius: 3px; background-color: %1;").arg(previewColor));
}

QColor dump_dock::ParsePreviewColor(QString value) const
{
	value = value.trimmed();
	if (value.isEmpty()) {
		return QColor("#ff0000");
	}

	if (!value.startsWith('#')) {
		value.prepend('#');
	}

	if (value.size() != 7) {
		return QColor();
	}

	QColor color(value);
	return color.isValid() ? color : QColor();
}

QString dump_dock::NormalizeHexColor(QString value) const
{
	value = value.trimmed();
	if (value.isEmpty()) {
		return "#ff0000";
	}

	if (!value.startsWith('#')) {
		value.prepend('#');
	}

	if (value.size() > 7) {
		value = value.left(7);
	}

	if (value.size() == 7) {
		QColor color(value);
		if (color.isValid()) {
			return color.name(QColor::HexRgb);
		}
	}

	return "#ff0000";
}

void dump_dock::SetStatus(const QString &text, bool isError, bool inProgress)
{
	statusLabel->setText(text);
	statusLabel->setStyleSheet(isError ? "color: #d64b4b;" : QString("color: %1;").arg(statusDefaultColor.name()));
	dumpButton->setEnabled(!inProgress);
}

void dump_dock::LoadPersistedSettings()
{
	char *configPath = obs_module_config_path("uhohbs-settings.json");
	if (!configPath) {
		return;
	}

	obs_data_t *data = obs_data_create_from_json_file_safe(configPath, "bak");
	if (!data) {
		data = obs_data_create();
	}

	dump_config config;
	config.load_settings(data);
	ApplyConfigToUi(config);

	obs_data_release(data);
	bfree(configPath);
}

void dump_dock::SavePersistedSettings() const
{
	char *configPath = obs_module_config_path("uhohbs-settings.json");
	if (!configPath) {
		return;
	}

	obs_data_t *data = obs_data_create();
	BuildConfigFromUi().save_settings(data);
	obs_data_save_json_safe(data, configPath, "tmp", "bak");
	obs_data_release(data);
	bfree(configPath);
}
