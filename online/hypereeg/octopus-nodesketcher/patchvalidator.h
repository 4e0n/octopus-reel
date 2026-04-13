#pragma once

#include "patchgraph.h"
#include <QStringList>
#include <QSet>

class PatchValidator
{
public:
    static QStringList validate(const PatchGraph& g)
    {
        QStringList errors;

        QSet<QString> nodeIds;

        for (const auto& n : g.nodes) {
            if (nodeIds.contains(n.id))
                errors << "Duplicate node id: " + n.id;
            nodeIds.insert(n.id);
        }

        for (const auto& e : g.edges) {
            if (!nodeIds.contains(e.fromNodeId))
                errors << "Edge from unknown node: " + e.fromNodeId;
            if (!nodeIds.contains(e.toNodeId))
                errors << "Edge to unknown node: " + e.toNodeId;
        }

        return errors;
    }
};
