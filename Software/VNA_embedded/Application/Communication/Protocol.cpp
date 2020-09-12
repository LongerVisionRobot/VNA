#include "Protocol.hpp"

#include <cstring>

/*
 * General packet format:
 * 1. 1 byte header
 * 2. 2 byte overall packet length (with header and checksum)
 * 3. packet type
 * 4. packet payload
 * 5. 4 byte CRC32 (with header)
 */

static constexpr uint8_t header = 0x5A;
static constexpr uint8_t header_size = 4;

#define CRC32_POLYGON 0xEDB88320
uint32_t Protocol::CRC32(uint32_t crc, const void *data, uint32_t len) {
	uint8_t *u8buf = (uint8_t*) data;
	int k;

	crc = ~crc;
	while (len--) {
		crc ^= *u8buf++;
		for (k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ CRC32_POLYGON : crc >> 1;
	}
	return ~crc;
}

class Encoder {
public:
    Encoder(uint8_t *buf, uint16_t size) :
        buf(buf),
        bufSize(size),
        usedSize(0),
        bitpos(0) {
        memset(buf, 0, size);
    };
    template<typename T> bool add(T data) {
        if(bitpos != 0) {
            // add padding to next byte boundary
            bitpos = 0;
            usedSize++;
        }
        if(bufSize - usedSize < (long) sizeof(T)) {
            // not enough space left
            return false;
        }
        memcpy(&buf[usedSize], &data, sizeof(T));
        usedSize += sizeof(T);
        return true;
    }
    bool addBits(uint8_t value, uint8_t bits) {
        if(bits >= 8 || usedSize >= bufSize) {
            return false;
        }
        buf[usedSize] |= (value << bitpos) & 0xFF;
        bitpos += bits;
        if(bitpos > 8) {
            // the value did not fit completely into the current byte
            if(usedSize >= bufSize - 1) {
                // already at maximum limit, not enough space for remaining bits
                return false;
            }
            // move access to next byte
            bitpos -= 8;
            usedSize++;
            // add remaining bytes
            buf[usedSize] = value >> (bits - bitpos);
        } else if(bitpos == 8) {
            bitpos = 0;
            usedSize++;
        }
        return true;
    }
    uint16_t getSize() const {
        if(bitpos == 0) {
            return usedSize;
        } else {
            return usedSize + 1;
        }
    };

private:
    uint8_t *buf;
    uint16_t bufSize;
    uint16_t usedSize;
    uint8_t bitpos;
};

class Decoder {
public:
    Decoder(uint8_t *buf) :
        buf(buf),
        usedSize(0),
        bitpos(0) {};
    template<typename T> void get(T &t) {
        if(bitpos != 0) {
            // add padding to next byte boundary
            bitpos = 0;
            usedSize++;
        }
        // still enough bytes available
        memcpy(&t, &buf[usedSize], sizeof(T));
        usedSize += sizeof(T);
    }
    uint8_t getBits(uint8_t bits) {
        if(bits >= 8) {
            return 0;
        }
        uint8_t mask = 0x00;
        for(uint8_t i=0;i<bits;i++) {
            mask <<= 1;
            mask |= 0x01;
        }
        uint8_t value = (buf[usedSize] >> bitpos) & mask;
        bitpos += bits;
        if(bitpos > 8) {
            // the current byte did not contain the complete value
            // move access to next byte
            bitpos -= 8;
            usedSize++;
            // get remaining bits
            value |= (buf[usedSize] << (bits - bitpos)) & mask;
        } else if(bitpos == 8) {
            bitpos = 0;
            usedSize++;
        }
        return value;
    }
private:
    uint8_t *buf;
    uint16_t usedSize;
    uint8_t bitpos;
};

static Protocol::Datapoint DecodeDatapoint(uint8_t *buf) {
    Protocol::Datapoint d;
    Decoder e(buf);
    e.get<float>(d.real_S11);
    e.get<float>(d.imag_S11);
    e.get<float>(d.real_S21);
    e.get<float>(d.imag_S21);
    e.get<float>(d.real_S12);
    e.get<float>(d.imag_S12);
    e.get<float>(d.real_S22);
    e.get<float>(d.imag_S22);
    e.get<uint64_t>(d.frequency);
    e.get<uint16_t>(d.pointNum);
    return d;
}
static int16_t EncodeDatapoint(Protocol::Datapoint d, uint8_t *buf,
		uint16_t bufSize) {
    Encoder e(buf, bufSize);
    e.add<float>(d.real_S11);
    e.add<float>(d.imag_S11);
    e.add<float>(d.real_S21);
    e.add<float>(d.imag_S21);
    e.add<float>(d.real_S12);
    e.add<float>(d.imag_S12);
    e.add<float>(d.real_S22);
    e.add<float>(d.imag_S22);
    e.add<uint64_t>(d.frequency);
    e.add<uint16_t>(d.pointNum);
    return e.getSize();
}

static Protocol::SweepSettings DecodeSweepSettings(uint8_t *buf) {
    Protocol::SweepSettings d;
    Decoder e(buf);
    e.get<uint64_t>(d.f_start);
    e.get<uint64_t>(d.f_stop);
    e.get<uint16_t>(d.points);
    e.get<uint32_t>(d.if_bandwidth);
    e.get<int16_t>(d.cdbm_excitation);
    return d;
}
static int16_t EncodeSweepSettings(Protocol::SweepSettings d, uint8_t *buf,
		uint16_t bufSize) {
    Encoder e(buf, bufSize);
    e.add<uint64_t>(d.f_start);
    e.add<uint64_t>(d.f_stop);
    e.add<uint16_t>(d.points);
    e.add<uint32_t>(d.if_bandwidth);
    e.add<int16_t>(d.cdbm_excitation);
    return e.getSize();
}

static Protocol::ReferenceSettings DecodeReferenceSettings(uint8_t *buf) {
    Protocol::ReferenceSettings d;
    Decoder e(buf);
    e.get<uint32_t>(d.ExtRefOuputFreq);
    d.AutomaticSwitch = e.getBits(1);
    d.UseExternalRef = e.getBits(1);
    return d;
}
static int16_t EncodeReferenceSettings(Protocol::ReferenceSettings d, uint8_t *buf,
		uint16_t bufSize) {
    Encoder e(buf, bufSize);
    e.add<uint32_t>(d.ExtRefOuputFreq);
    e.addBits(d.AutomaticSwitch, 1);
    e.addBits(d.UseExternalRef, 1);
    return e.getSize();
}

static Protocol::DeviceInfo DecodeDeviceInfo(uint8_t *buf) {
    Protocol::DeviceInfo d;
    Decoder e(buf);
    e.get<uint16_t>(d.FW_major);
    e.get<uint16_t>(d.FW_minor);
    e.get<char>(d.HW_Revision);
    d.extRefAvailable = e.getBits(1);
    d.extRefInUse = e.getBits(1);
    d.FPGA_configured = e.getBits(1);
    d.source_locked = e.getBits(1);
    d.LO1_locked = e.getBits(1);
    d.ADC_overload = e.getBits(1);
    e.get<uint8_t>(d.temperatures.source);
    e.get<uint8_t>(d.temperatures.LO1);
    e.get<uint8_t>(d.temperatures.MCU);
    return d;
}
static int16_t EncodeDeviceInfo(Protocol::DeviceInfo d, uint8_t *buf,
                                                   uint16_t bufSize) {
    Encoder e(buf, bufSize);
    e.add<uint16_t>(d.FW_major);
    e.add<uint16_t>(d.FW_minor);
    e.add<char>(d.HW_Revision);
    e.addBits(d.extRefAvailable, 1);
    e.addBits(d.extRefInUse, 1);
    e.addBits(d.FPGA_configured, 1);
    e.addBits(d.source_locked, 1);
    e.addBits(d.LO1_locked, 1);
    e.addBits(d.ADC_overload, 1);
    e.add<uint8_t>(d.temperatures.source);
    e.add<uint8_t>(d.temperatures.LO1);
    e.add<uint8_t>(d.temperatures.MCU);
    return e.getSize();
}

static Protocol::ManualStatus DecodeStatus(uint8_t *buf) {
    Protocol::ManualStatus d;
    Decoder e(buf);
    e.get<int16_t>(d.port1min);
    e.get<int16_t>(d.port1max);
    e.get<int16_t>(d.port2min);
    e.get<int16_t>(d.port2max);
    e.get<int16_t>(d.refmin);
    e.get<int16_t>(d.refmax);
    e.get<float>(d.port1real);
    e.get<float>(d.port1imag);
    e.get<float>(d.port2real);
    e.get<float>(d.port2imag);
    e.get<float>(d.refreal);
    e.get<float>(d.refimag);
    e.get<uint8_t>(d.temp_source);
    e.get<uint8_t>(d.temp_LO);
    d.source_locked = e.getBits( 1);
    d.LO_locked = e.getBits(1);
    return d;
}
static int16_t EncodeStatus(Protocol::ManualStatus d, uint8_t *buf,
                                     uint16_t bufSize) {
    Encoder e(buf, bufSize);
    e.add<int16_t>(d.port1min);
    e.add<int16_t>(d.port1max);
    e.add<int16_t>(d.port2min);
    e.add<int16_t>(d.port2max);
    e.add<int16_t>(d.refmin);
    e.add<int16_t>(d.refmax);
    e.add<float>(d.port1real);
    e.add<float>(d.port1imag);
    e.add<float>(d.port2real);
    e.add<float>(d.port2imag);
    e.add<float>(d.refreal);
    e.add<float>(d.refimag);
    e.add<uint8_t>(d.temp_source);
    e.add<uint8_t>(d.temp_LO);
    e.addBits(d.source_locked, 1);
    e.addBits(d.LO_locked, 1);
    return e.getSize();
}

static Protocol::ManualControl DecodeManualControl(uint8_t *buf) {
    Protocol::ManualControl d;
    Decoder e(buf);
    d.SourceHighCE = e.getBits(1);
    d.SourceHighRFEN = e.getBits(1);
    d.SourceHighPower = e.getBits(2);
    d.SourceHighLowpass = e.getBits(2);
    e.get<uint64_t>(d.SourceHighFrequency);
    d.SourceLowEN = e.getBits(1);
    d.SourceLowPower = e.getBits( 2);
    e.get<uint32_t>(d.SourceLowFrequency);
    d.attenuator = e.getBits(7);
    d.SourceHighband = e.getBits(1);
    d.AmplifierEN = e.getBits(1);
    d.PortSwitch = e.getBits(1);
    d.LO1CE = e.getBits(1);
    d.LO1RFEN = e.getBits(1);
    e.get<uint64_t>(d.LO1Frequency);
    d.LO2EN = e.getBits(1);
    e.get<uint32_t>(d.LO2Frequency);
    d.Port1EN = e.getBits(1);
    d.Port2EN = e.getBits(1);
    d.RefEN = e.getBits(1);
    e.get<uint32_t>(d.Samples);
    return d;
}
static int16_t EncodeManualControl(Protocol::ManualControl d, uint8_t *buf,
                                                   uint16_t bufSize) {
    Encoder e(buf, bufSize);
    e.addBits(d.SourceHighCE, 1);
    e.addBits(d.SourceHighRFEN, 1);
    e.addBits(d.SourceHighPower, 2);
    e.addBits(d.SourceHighLowpass, 2);
    e.add<uint64_t>(d.SourceHighFrequency);
    e.addBits(d.SourceLowEN, 1);
    e.addBits(d.SourceLowPower, 2);
    e.add<uint32_t>(d.SourceLowFrequency);
    e.addBits(d.attenuator, 7);
    e.addBits(d.SourceHighband, 1);
    e.addBits(d.AmplifierEN, 1);
    e.addBits(d.PortSwitch, 1);
    e.addBits(d.LO1CE, 1);
    e.addBits(d.LO1RFEN, 1);
    e.add<uint64_t>(d.LO1Frequency);
    e.addBits(d.LO2EN, 1);
    e.add<uint32_t>(d.LO2Frequency);
    e.addBits(d.Port1EN, 1);
    e.addBits(d.Port2EN, 1);
    e.addBits(d.RefEN, 1);
    e.add<uint32_t>(d.Samples);
    return e.getSize();
}

static Protocol::FirmwarePacket DecodeFirmwarePacket(uint8_t *buf) {
    Protocol::FirmwarePacket d;
    // simple packet format, memcpy is faster than using the decoder
    memcpy(&d.address, buf, 4);
    buf += 4;
    memcpy(d.data, buf, Protocol::FirmwareChunkSize);
    return d;
}
static int16_t EncodeFirmwarePacket(const Protocol::FirmwarePacket &d, uint8_t *buf, uint16_t bufSize) {
    if(bufSize < 4 + Protocol::FirmwareChunkSize) {
        // unable to encode, not enough space
        return -1;
    }
    // simple packet format, memcpy is faster than using the encoder
    memcpy(buf, &d.address, 4);
    buf += 4;
    memcpy(buf, d.data, Protocol::FirmwareChunkSize);
    return 4 + Protocol::FirmwareChunkSize;
}

uint16_t Protocol::DecodeBuffer(uint8_t *buf, uint16_t len, PacketInfo *info) {
    if (!info || !len) {
        info->type = PacketType::None;
		return 0;
	}
	uint8_t *data = buf;
	/* Remove any out-of-order bytes in front of the frame */
	while (*data != header) {
		data++;
		if(--len == 0) {
			/* Reached end of data */
			/* No frame contained in data */
			info->type = PacketType::None;
			return data - buf;
		}
	}
	/* At this point, data points to the beginning of the frame */
	if(len < header_size) {
		/* the frame header has not been completely received */
		info->type = PacketType::None;
		return data - buf;
	}

	/* Evaluate frame size */
	uint16_t length = *(uint16_t*) &data[1];
	if(len < length) {
		/* The frame payload has not been completely received */
		info->type = PacketType::None;
		return data - buf;
	}

	/* The complete frame has been received, check checksum */
    uint32_t crc = *(uint32_t*) &data[length - 4];
	uint32_t compare = CRC32(0, data, length - 4);
	if(crc != compare) {
		// CRC mismatch, remove header
		data += 1;
		info->type = PacketType::None;
		return data - buf;
	}

	// Valid packet, extract packet info
	info->type = (PacketType) data[3];
	switch (info->type) {
	case PacketType::Datapoint:
		info->datapoint = DecodeDatapoint(&data[4]);
		break;
	case PacketType::SweepSettings:
		info->settings = DecodeSweepSettings(&data[4]);
		break;
	case PacketType::Reference:
		info->reference = DecodeReferenceSettings(&data[4]);
		break;
    case PacketType::DeviceInfo:
        info->info = DecodeDeviceInfo(&data[4]);
        break;
    case PacketType::Status:
        info->status = DecodeStatus(&data[4]);
        break;
    case PacketType::ManualControl:
        info->manual = DecodeManualControl(&data[4]);
        break;
    case PacketType::FirmwarePacket:
        info->firmware = DecodeFirmwarePacket(&data[4]);
        break;
    case PacketType::Ack:
    case PacketType::PerformFirmwareUpdate:
    case PacketType::ClearFlash:
    case PacketType::Nack:
        // no payload, nothing to do
        break;
    case PacketType::None:
        break;
	}

	return data - buf + length;
}

uint16_t Protocol::EncodePacket(PacketInfo packet, uint8_t *dest, uint16_t destsize) {
    int16_t payload_size = 0;
	switch (packet.type) {
	case PacketType::Datapoint:
        payload_size = EncodeDatapoint(packet.datapoint, &dest[4], destsize - 8);
		break;
	case PacketType::SweepSettings:
        payload_size = EncodeSweepSettings(packet.settings, &dest[4], destsize - 8);
		break;
	case PacketType::Reference:
		payload_size = EncodeReferenceSettings(packet.reference, &dest[4], destsize - 8);
		break;
    case PacketType::DeviceInfo:
        payload_size = EncodeDeviceInfo(packet.info, &dest[4], destsize - 8);
        break;
    case PacketType::Status:
        payload_size = EncodeStatus(packet.status, &dest[4], destsize - 8);
        break;
    case PacketType::ManualControl:
        payload_size = EncodeManualControl(packet.manual, &dest[4], destsize - 8);
        break;
    case PacketType::FirmwarePacket:
        payload_size = EncodeFirmwarePacket(packet.firmware, &dest[4], destsize - 8);
        break;
    case PacketType::Ack:
    case PacketType::PerformFirmwareUpdate:
    case PacketType::ClearFlash:
    case PacketType::Nack:
        // no payload, nothing to do
        break;
    case PacketType::None:
        break;
    }
    if (payload_size < 0 || payload_size + 8 > destsize) {
		// encoding failed, buffer too small
		return 0;
	}
	// Write header
	dest[0] = header;
	uint16_t overall_size = payload_size + 8;
	memcpy(&dest[1], &overall_size, 2);
	dest[3] = (int) packet.type;
	// Calculate checksum
	uint32_t crc = CRC32(0, dest, overall_size - 4);
	memcpy(&dest[overall_size - 4], &crc, 4);
	return overall_size;
}

