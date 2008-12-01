#ifndef _ERRORS_HEADER_
#define _ERRORS_HEADER_

// <borat> Great Success! </borat>
#define ErrorSuccess			0x0

// Generic Errors
#define ErrorGeneric			0x1
#define ErrorOutOfMemory		0x2
#define ErrorNotFound			0x3
#define ErrorInUse				0x4
#define ErrorUnknown			0x5
#define ErrorEmpty				0x6
#define ErrorNullArg			0x7
#define ErrorInvalidArg			0x8
#define ErrorUnimplemented		0x9
#define ErrorInternal			0xA

// Object Manager Errors
#define ErrorInvalidHandle		0x101
#define ErrorInvalidInterface	0x102
#define ErrorInvalidType		0x103
#define ErrorInvalidObject		0x104
#define ErrorNameInUse			0x105
#define ErrorReparse			0x106

// Thread Manager Errors
#define ErrorNoWait				0x201

// IO Manager Errors
#define ErrorUnmounted			0x301
#define ErrorUnopened			0x302
#define ErrorUnmountable		0x303
#define ErrorReadOnly			0x304
#define ErrorWriteOnly			0x305
#define ErrorTimeout			0x306
#define ErrorMountOnly			0x307
#define ErrorUnreadable			0x308
#define ErrorUnwriteable		0x309
#define ErrorWriting			0x30A
#define ErrorReading			0x30B
#define ErrorUnknownDevice		0x30C
#define ErrorDeviceOff			0x30D
#define ErrorUnknownFS			0x30E
#define ErrorInvalidPartition	0x30F
#define ErrorUnseekable			0x310
#define ErrorInvalidSeek		0x311
#define ErrorSignature			0x312
#define ErrorChecksum			0x313
#define ErrorInvalidCluster		0x314
#define ErrorInvalidFileName	0x315

#endif
