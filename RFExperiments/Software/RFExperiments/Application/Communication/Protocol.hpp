#pragma once

#include <cstdint>

namespace Protocol {

using Datapoint = struct _datapoint {
	float real_S11, imag_S11;
	float real_S21, imag_S21;
	float real_S12, imag_S12;
	float real_S22, imag_S22;
	uint64_t frequency;
	uint16_t pointNum;
} __attribute__((packed));

using SweepSettings = struct _sweepSettings {
	uint64_t f_start;
	uint64_t f_stop;
	uint16_t points;
        uint32_t if_bandwidth;
	int16_t mdbm_excitation;
} __attribute__((packed));

using Status = struct _status {
        int16_t port1min, port1max;
        int16_t port2min, port2max;
        int16_t refmin, refmax;
        float port1real, port1imag;
        float port2real, port2imag;
        float refreal, refimag;
        uint8_t temp_source;
        uint8_t temp_LO;
        uint8_t source_locked :1;
        uint8_t LO_locked :1;
} __attribute__((packed));

using ManualControl = struct _manualControl {
    // Highband Source
    uint8_t SourceHighCE :1;
    uint8_t SourceHighRFEN :1;
    uint8_t SourceHighPower :2;
    uint8_t SourceHighLowpass :2;
    uint64_t SourceHighFrequency;
    // Lowband Source
    uint8_t SourceLowEN :1;
    uint8_t SourceLowPower :2;
    uint32_t SourceLowFrequency;
    // Source signal path
    uint8_t attenuator :7;
    uint8_t SourceHighband :1;
    uint8_t AmplifierEN :1;
    uint8_t PortSwitch :1;
    // LO1
    uint8_t LO1CE :1;
    uint8_t LO1RFEN :1;
    uint64_t LO1Frequency;
    // LO2
    uint8_t LO2EN :1;
    uint32_t LO2Frequency;
    // Acquisition
    uint8_t Port1EN :1;
    uint8_t Port2EN :1;
    uint8_t RefEN :1;
    uint32_t Samples;
} __attribute__((packed));

enum class PacketType : uint8_t {
	None,
	Datapoint,
	SweepSettings,
        Status,
        ManualControl,
};

using PacketInfo = struct _packetinfo {
	PacketType type;
	union {
		Datapoint datapoint;
		SweepSettings settings;
                ManualControl manual;
                Status status;
	};
};

uint16_t DecodeBuffer(uint8_t *buf, uint16_t len, PacketInfo *info);
uint16_t EncodePacket(PacketInfo packet, uint8_t *dest, uint16_t destsize);

}
