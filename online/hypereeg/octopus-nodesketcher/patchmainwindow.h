#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "patchgraph.h"
#include "patchregistry.h"
#include "patchvalidator.h"

class PatchMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    PatchMainWindow()
    {
        auto* central = new QWidget;
        auto* layout = new QHBoxLayout(central);

        m_palette = new QListWidget;
        m_palette->addItems(PatchRegistry::instance().allTypes());

        m_output = new QTextEdit;
        m_output->setReadOnly(true);

        auto* btnAdd = new QPushButton("Add Node");
        auto* btnValidate = new QPushButton("Validate");
        auto* btnRun = new QPushButton("Run");

        connect(btnAdd, &QPushButton::clicked, this, &PatchMainWindow::onAddNode);
        connect(btnValidate, &QPushButton::clicked, this, &PatchMainWindow::onValidate);
        connect(btnRun, &QPushButton::clicked, this, &PatchMainWindow::onRun);

        auto* rightLayout = new QVBoxLayout;
        rightLayout->addWidget(btnAdd);
        rightLayout->addWidget(btnValidate);
        rightLayout->addWidget(btnRun);
        rightLayout->addWidget(m_output);

        layout->addWidget(m_palette);
        layout->addLayout(rightLayout);

        setCentralWidget(central);
        resize(900, 500);
    }

private slots:
    void onAddNode()
    {
        auto item = m_palette->currentItem();
        if (!item) return;

        PatchNode n;
        n.id = QString("node_%1").arg(++m_nodeCounter);
        n.type = item->text();
        n.name = n.type;

        m_graph.nodes.append(n);

        m_output->append("Added: " + n.id + " (" + n.type + ")");
    }

    void onValidate()
    {
        auto errs = PatchValidator::validate(m_graph);

        if (errs.isEmpty()) {
            m_output->append("VALID: OK");
        } else {
            for (auto& e : errs)
                m_output->append("ERROR: " + e);
        }
    }

    void onRun()
    {
        m_output->append("=== RUN PREVIEW ===");

        for (const auto& n : m_graph.nodes)
            m_output->append("Node: " + n.id + " (" + n.type + ")");

        for (const auto& e : m_graph.edges)
            m_output->append("Edge: " + e.fromNodeId + " -> " + e.toNodeId);
    }

private:
    PatchGraph m_graph;

    QListWidget* m_palette = nullptr;
    QTextEdit* m_output = nullptr;

    int m_nodeCounter = 0;
};
