// converts between LITTLE and BIG ENDIAN

#ifndef __ENDIANESS_H__
#define __ENDIANESS_H__

static inline u16 get_16bitBE(u8* p) {
    return (p[0]<<8) | (p[1]);
}

static inline u32 get_32bitBE(u8* p) {
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | (p[3]);
}

static inline u16 get_16bitLE(u8* p) {
    return (p[0]) | (p[1]<<8);
}

static inline u32 get_32bitLE(u8* p) {
	return (p[0]) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

#endif
