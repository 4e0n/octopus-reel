#pragma once

// Helpers for little-endian reads (safe'n fast)
static inline quint32 rd_u32_le(const char* p) {
 const uchar* u=reinterpret_cast<const uchar*>(p);
 return (quint32)u[0]
        |((quint32)u[1]<<8)
        |((quint32)u[2]<<16)
        |((quint32)u[3]<<24);
}

static inline quint64 rd_u64_le(const char* p) {
 const uchar* u=reinterpret_cast<const uchar*>(p);
 return (quint64)u[0]
        |((quint64)u[1]<<8)
        |((quint64)u[2]<<16)
        |((quint64)u[3]<<24)
        |((quint64)u[4]<<32)
        |((quint64)u[5]<<40)
        |((quint64)u[6]<<48)
        |((quint64)u[7]<<56);
}

static inline float rd_f32_le(const char* p) {
 quint32 w=rd_u32_le(p);
 float f;
 static_assert(sizeof(float)==4,"float must be 4 bytes");
 memcpy(&f,&w,4);
 return f;
}
