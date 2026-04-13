#pragma once

#include <QString>
#include <QVector>

struct PortDef {
 QString name;
 QString kind;   // eeg, audio, event, control
 bool isInput;
};

struct NodeTypeDef {
 QString type;
 QString displayName;
 QVector<PortDef> ports;
};
