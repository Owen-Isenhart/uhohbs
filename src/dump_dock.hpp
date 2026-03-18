#pragma once

#include <QDockWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>

class dump_dock : public QDockWidget {
    Q_OBJECT

public:
    dump_dock(QWidget *parent = nullptr) : QDockWidget(parent) {
        setWindowTitle("UhohBS Control");
        setObjectName("uhohbs_dock"); // Critical for OBS to remember the dock's position

        auto *layout = new QVBoxLayout();
        
        // Configuration Section
        layout->addWidget(new QLabel("Dump Delay (seconds):"));
        auto *delayInput = new QSpinBox();
        delayInput->setRange(1, 60);
        layout->addWidget(delayInput);

        // The Big Red Button
        auto *button = new QPushButton("DUMP STREAM");
        button->setMinimumHeight(50);
        layout->addWidget(button);

        auto *container = new QWidget();
        container->setLayout(layout);
        setWidget(container);

        // This is where you will connect your button to the logic later
        connect(button, &QPushButton::clicked, this, &dump_dock::HandleDump);
    }

private slots:
    void HandleDump() {
        // We will fill this with the actual output-stopping logic next
    }
};