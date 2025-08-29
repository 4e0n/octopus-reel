/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/
#ifndef _TCPCMARRAY_H
#define _TCPCMARRAY_H

#include <vector>
#include <QByteArray>
#include <QDataStream>

struct TcpCMArray {
    std::vector<std::vector<uint8_t>> cmLevel; // 0..255 -> red..yellow..green
    std::vector<std::vector<uint8_t>> reason;  // optional reason bits per ch (same shape)

    static constexpr quint32 MAGIC   = 0x434D4152; // 'CMAR'
    static constexpr quint16 VERSION = 1;          // 0 = old format

    void init(size_t ampCount,size_t chnCount,bool withReasons=false) {
        cmLevel.assign(ampCount,std::vector<uint8_t>(chnCount,0));
        if (withReasons) reason.assign(ampCount,std::vector<uint8_t>(chnCount,0));
        else reason.clear();
    }

    TcpCMArray(size_t ampCount=0,size_t chnCount=0,bool withReasons=false) {
        init(ampCount,chnCount,withReasons);
    }

    QByteArray serialize() const {
        QByteArray ba;
        QDataStream out(&ba, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);

        const quint32 ampCount  = static_cast<quint32>(cmLevel.size());
        const quint32 chnCount  = ampCount ? static_cast<quint32>(cmLevel[0].size()) : 0;
        const quint8  hasReason = (!reason.empty()) ? 1 : 0;

        // Header
        out << MAGIC << VERSION << ampCount << chnCount << hasReason;

        // Payload: cmLevel as one contiguous block
        for (const auto& v : cmLevel) {
            if (!v.empty())
                out.writeRawData(reinterpret_cast<const char*>(v.data()),
                                 static_cast<int>(v.size()));
        }

        // Optional: reason bitmask
        if (hasReason) {
            for (const auto& v : reason) {
                if (!v.empty())
                    out.writeRawData(reinterpret_cast<const char*>(v.data()),
                                     static_cast<int>(v.size()));
            }
        }

        return ba;
    }

    bool deserialize(const QByteArray &ba,size_t chnCountIfV0=0) {
        QDataStream in(ba);
        in.setByteOrder(QDataStream::LittleEndian);

        quint32 magic=0; in >> magic;
        if (magic!=MAGIC) return false;

        quint16 version=0;
        const int pos=in.device()->pos();
        in >> version;
        if (version!=VERSION) {
            // old v0 → rewind
            in.device()->seek(pos);
            return deserializeV0(in,chnCountIfV0);
        }

        // v1 parse
        quint32 ampCount=0,chnCount=0; quint8 hasReason=0;
        in >> ampCount >> chnCount >> hasReason;

        cmLevel.assign(ampCount,std::vector<uint8_t>(chnCount));
        for (auto& amp:cmLevel) {
            if (!amp.empty())
                in.readRawData(reinterpret_cast<char*>(amp.data()),
                               static_cast<int>(amp.size()));
        }

        if (hasReason) {
            reason.assign(ampCount,std::vector<uint8_t>(chnCount));
            for (auto& amp:reason) {
                if (!amp.empty())
                    in.readRawData(reinterpret_cast<char*>(amp.data()),
                                   static_cast<int>(amp.size()));
            }
        } else {
            reason.clear();
        }
        return true;
    }

private:
    bool deserializeV0(QDataStream& in,size_t chnCount) {
        quint32 ampCount=0; in >> ampCount;
        if (chnCount==0) return false;
        cmLevel.assign(ampCount,std::vector<uint8_t>(chnCount));
        for (auto& amp:cmLevel) {
            if (!amp.empty())
                in.readRawData(reinterpret_cast<char*>(amp.data()),
                               static_cast<int>(amp.size()));
        }
        reason.clear();
        return true;
    }
};

#endif

/*
struct TcpCMArray {
 std::vector<std::vector<uint8_t>> cmLevel; // 0..255 -> red..yellow..green
 std::vector<std::vector<uint8_t>> reason;  // optional reason bits per ch (same shape)

 static constexpr quint32 MAGIC=0x434D4152; // 'CMAR'
 static constexpr quint16 VERSION=1;        // 0 = your old format

 void init(size_t ampCount,size_t chnCount,bool withReasons=false) {
  cmLevel.assign(ampCount,std::vector<uint8_t>(chnCount,0));
  if (withReasons) reason.assign(ampCount,std::vector<uint8_t>(chnCount,0));
  else reason.clear();
 }

 TcpCMArray(size_t ampCount=0,size_t chnCount=0,bool withReasons=false) {
  init(ampCount,chnCount,withReasons);
 }

 QByteArray serialize() const {
  QByteArray ba;
  QDataStream out(&ba, QIODevice::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);

  const quint32 ampCount = static_cast<quint32>(cmLevel.size());
  const quint32 chnCount = ampCount ? static_cast<quint32>(cmLevel[0].size()) : 0;
  const quint8  hasReasons = (!reason.empty()) ? 1 : 0;

  // Header: MAGIC, VERSION, ampCount, chnCount, hasReasons
  out << MAGIC << VERSION << ampCount << chnCount << hasReasons;

  // Payload: cmLevel (amp-major, then channels)
  for (const auto& v:cmLevel) for (uint8_t c:v) out << c;

  // Optional: reason bitmask (same layout)
  if (hasReasons) for (const auto& v:reason) for (uint8_t r:v) out << r;

  return ba;
 }

 // Backward-compatible: if stream is v0 (MAGIC, ampCount, then cm bytes), pass chnCount explicitly.
 bool deserialize(const QByteArray &ba,size_t chnCountIfV0=0) {
  QDataStream in(ba);
  in.setByteOrder(QDataStream::LittleEndian);

  quint32 magic=0; in >> magic;
  if (magic!=MAGIC) return false;

  // Peek next 2 bytes to see if this is a VERSION field (v1)
  quint16 version=0;
  {
   const int pos=in.device()->pos();
   in >> version;
   if (version!=VERSION) {
    // Not v1 → assume v0: rewind and parse v0
    in.device()->seek(pos);
    return deserializeV0(in,chnCountIfV0);
   }
  }

  // v1 parse
  quint32 ampCount=0,chnCount=0;quint8 hasReasons=0;
  in >> ampCount >> chnCount >> hasReasons;

  cmLevel.assign(ampCount,std::vector<uint8_t>(chnCount));
  for (auto& amp:cmLevel) for (auto& c:amp) in >> c;

  if (hasReasons) {
   reason.assign(ampCount,std::vector<uint8_t>(chnCount));
   for (auto& amp:reason) for (auto& r:amp) in >> r;
  } else {
   reason.clear();
  }
  return true;
 }

 private:
  bool deserializeV0(QDataStream& in,size_t chnCount) {
   // v0: MAGIC, ampCount, then ampCount*chnCount bytes of cmLevel
   quint32 ampCount=0; in >> ampCount;
   if (chnCount==0) return false; // caller must supply chnCount for v0
   cmLevel.assign(ampCount,std::vector<uint8_t>(chnCount));
   for (auto& amp:cmLevel) for (auto& c:amp) in >> c;
   reason.clear();
   return true;
  }
};
*/
