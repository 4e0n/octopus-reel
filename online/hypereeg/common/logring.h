#pragma once

static inline void log_ring_1hz(
    const char* tag,
    quint64 head, quint64 tail,
    quint64& lastHead, quint64& lastTail,
    qint64& lastMs)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastMs == 0) lastMs = now;

    if (now - lastMs >= 1000) {
        const quint64 fill = head - tail;
        const quint64 dH = head - lastHead;   // pushes in last second
        const quint64 dT = tail - lastTail;   // pops in last second

        qInfo().noquote()
            << QString("[%1] fill=%2  +%3/s  -%4/s  head=%5 tail=%6")
               .arg(tag)
               .arg(fill)
               .arg(dH)
               .arg(dT)
               .arg((qulonglong)head)
               .arg((qulonglong)tail);

        lastHead = head;
        lastTail = tail;
        lastMs = now;
    }
}
