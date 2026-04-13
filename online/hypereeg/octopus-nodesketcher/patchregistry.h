#pragma once

#include "nodetypes.h"
#include <QMap>

class PatchRegistry
{
public:
    static PatchRegistry& instance()
    {
        static PatchRegistry inst;
        return inst;
    }

    const NodeTypeDef* get(const QString& type) const
    {
     auto it = m_types.constFind(type);
     if (it == m_types.constEnd())
        return nullptr;

     return &it.value();
    }

    QStringList allTypes() const
    {
        return m_types.keys();
    }

private:
    QMap<QString, NodeTypeDef> m_types;

    PatchRegistry()
    {
        m_types["ACQ"] = {
            "ACQ", "node-acq",
            {
                {"eeg_out", "eeg", false},
                {"audio_out", "audio", false},
                {"event_out", "event", false}
            }
        };

        m_types["COMP_PP"] = {
            "COMP_PP", "node-comp-pp",
            {
                {"eeg_in", "eeg", true},
                {"eeg_out", "eeg", false}
            }
        };

        m_types["COMP_CMLEVELS"] = {
            "COMP_CMLEVELS", "node-comp-cmlevels",
            {
                {"eeg_in", "eeg", true}
            }
        };

        m_types["STOR"] = {
            "STOR", "node-stor",
            {
                {"eeg_in", "eeg", true}
            }
        };

        m_types["GUI_TIME"] = {
            "GUI_TIME", "node-gui-time",
            {
                {"eeg_in", "eeg", true}
            }
        };

        m_types["WAVPLAY"] = {
            "WAVPLAY", "node-wavplay",
            {
                {"ctrl_in", "control", true}
            }
        };

        m_types["HUMEVT"] = {
            "HUMEVT", "node-humevt",
            {
                {"event_out", "event", false}
            }
        };
    }
};
