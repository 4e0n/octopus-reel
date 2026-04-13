#pragma once

#include <QString>
#include <QVector>
#include <QVariantMap>

struct PatchNode {
    QString id;
    QString type;
    QString name;
    QVariantMap params;
};

struct PatchEdge {
    QString fromNodeId;
    QString fromPort;
    QString toNodeId;
    QString toPort;
    QString kind;
};

struct PatchGraph {
    QVector<PatchNode> nodes;
    QVector<PatchEdge> edges;

    void clear()
    {
        nodes.clear();
        edges.clear();
    }
};
