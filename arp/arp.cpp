#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <array>
#include <cstdint>

typedef ap_axiu<8, 0, 0, 0> mac_data_axis;

template<typename T>
struct optional
{
	typedef optional<T> SelfType;
	bool has_value;
	T value;
	optional() : has_value(false) {}
	optional(const SelfType&) = default;
	optional(SelfType&&) = default;
	optional(const T& value) : has_value(true), value(value) {}
	optional(T&& value) : has_value(true), value(std::forward<T>(value)) {}

	T& get() { return this->value; }
	const T& get() const { return this->value; }

	operator bool() const { return this->has_value; }
	SelfType& operator=(const SelfType& rhs) {
		this->has_value = rhs.has_value;
		this->value = rhs.value;
		return *this;
	}
	SelfType& operator=(SelfType&& rhs) {
		this->has_value = rhs.has_value;
		this->value = std::forward<T>(rhs);
		return *this;
	}
};

struct MACData {
	ap_uint<8> data;
	ap_uint<1> last;
	MACData() {}
	MACData(std::uint8_t data) : data(data), last(0) {}
	MACData(std::uint8_t data, bool last) : data(data), last(last ? 1 : 0) {}
	MACData(const MACData&) = default;
	MACData(const mac_data_axis& axis) : data(axis.data), last(axis.last) {}

	operator mac_data_axis() const {
		mac_data_axis axis;
		axis.data = this->data;
		axis.keep = 1;
		axis.last = this->last;
		return axis;
	}
};


typedef std::array<std::uint8_t, 6> HardwareAddress;
typedef std::array<std::uint8_t, 4> IPAddress;

static constexpr const HardwareAddress HWADDR = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
};
static constexpr const IPAddress IPADDR = {
	192, 168, 4, 2,
};

struct EthernetHeader
{
	HardwareAddress destination;
	HardwareAddress source;
	std::uint16_t protocol;
};

template<typename Array>
static inline std::uint16_t read16be(const Array& a, std::size_t offset) {
	return (a[offset + 0] << 8) | (a[offset + 1]);
}
template<typename Array>
static inline void write16be(Array& a, std::size_t offset, std::uint16_t value) {
	a[offset + 0] = value >> 8;
	a[offset + 1] = value & 0xff;
}
template<typename Array>
static inline HardwareAddress read_hwaddr(const Array& a, std::size_t offset) {
	HardwareAddress address;
	for(std::size_t i = 0; i < 6; i++) {
#pragma HLS UNROLL
		address[i] = a[offset + i];
	}
	return address;
}
template<typename Array>
static inline void write_hwaddr(Array& a, std::size_t offset, const HardwareAddress& address) {
	for(std::size_t i = 0; i < 6; i++) {
#pragma HLS UNROLL
		a[offset + i] = address[i];
	}
}
template<typename Array>
static inline IPAddress read_ipaddr(const Array& a, std::size_t offset) {
	IPAddress address;
	for(std::size_t i = 0; i < 4; i++) {
#pragma HLS UNROLL
		address[i] = a[offset + i];
	}
	return address;
}
template<typename Array>
static inline void write_ipaddr(Array& a, std::size_t offset, const IPAddress& address) {
	for(std::size_t i = 0; i < 4; i++) {
#pragma HLS UNROLL
		a[offset + i] = address[i];
	}
}

struct ARP
{
	static constexpr const std::size_t SIZE = 28;
	std::array<std::uint8_t, SIZE> raw;
	std::uint16_t hardware_type() const { return read16be(this->raw, 0); }
	std::uint16_t protocol_type() const { return read16be(this->raw, 2); }
	std::uint8_t hlen() const { return this->raw[4]; }
	std::uint8_t plen() const { return this->raw[5]; }
	std::uint16_t operation() const { return read16be(this->raw, 6); }
	HardwareAddress sha() const { return read_hwaddr(this->raw, 8); }
	IPAddress spa() const { return read_ipaddr(this->raw, 14); }
	HardwareAddress tha() const { return read_hwaddr(this->raw, 18); }
	IPAddress tpa() const { return read_ipaddr(this->raw, 24); }

	void hardware_type(std::uint16_t value) { write16be(this->raw, 0, value); }
	void protocol_type(std::uint16_t value) { write16be(this->raw, 2, value); }
	void hlen(std::uint8_t value) { this->raw[4] = value; }
	void plen(std::uint8_t value) { this->raw[5] = value; }
	void operation(std::uint16_t value) { write16be(this->raw, 6, value); }

	void sha(const HardwareAddress& value) { write_hwaddr(this->raw, 8, value); }
	void spa(const IPAddress& value) { write_ipaddr(this->raw, 14, value); }
	void tha(const HardwareAddress& value) { write_hwaddr(this->raw, 18, value); }
	void tpa(const IPAddress& value) { write_ipaddr(this->raw, 24, value); }
};

enum class ReadExactResult
{
	NotEnough,
	Exact,
	Remaining,
};

template<std::size_t N>
static inline ReadExactResult read_exact(hls::stream<mac_data_axis>& in, std::array<std::uint8_t, N>& buffer)
{
	bool last = false;
	for(std::size_t i = 0; i < N; i++) {
#pragma HLS pipeline ii=1
		auto data = in.read();
		if( data.last ) return ReadExactResult::NotEnough;
		buffer[i] = data.data;
		last = data.last;
	}
	return last ? ReadExactResult::Exact : ReadExactResult::Remaining;
}

template<std::size_t N>
static inline void write_all(hls::stream<mac_data_axis>& out, const std::array<std::uint8_t, N>& data, bool is_last)
{
	for(std::size_t i = 0; i < N; i++) {
#pragma HLS PIPELINE ii=1
		mac_data_axis mac_data;
		mac_data.data = data[i];
		mac_data.keep = 1;
		mac_data.last = is_last && i == N - 1;
		out.write(mac_data);
	}
}


static optional<EthernetHeader> read_header(hls::stream<mac_data_axis>& in)
{
	EthernetHeader header;
	if( read_exact(in, header.destination) != ReadExactResult::Remaining ) return {};
	if( read_exact(in, header.source) != ReadExactResult::Remaining ) return {};
	
	std::array<std::uint8_t, 2> protocol;
	if( read_exact(in, protocol) != ReadExactResult::Remaining ) return {};
	header.protocol = (protocol[0] << 8) | protocol[1];
	
	return header;
}

static inline void consume_remaining(hls::stream<mac_data_axis>& in)
{
	for(;;) {
#pragma HLS PIPELINE ii = 1
		auto d = in.read();
		if( d.last ) break;
	}
}

static inline void ensure_frame_minimum_length(hls::stream<mac_data_axis>& out, std::size_t length)
{
	if( length < 64 ) {
		std::size_t padding_count = 64 - length;
		for(std::size_t i = 0; i < padding_count; i++) {
	#pragma HLS UNROLL
			out.write(MACData(0, i == padding_count - 1)); // Fill zero to ensure frame length is at least 64 octets.
		}
	}
}

static void arp(const EthernetHeader& header, hls::stream<mac_data_axis>& in, hls::stream<mac_data_axis>& out)
{
	ARP arp;
	{
		auto result =  read_exact(in, arp.raw);
		if( result != ReadExactResult::Exact ) {
			consume_remaining(in);
		}
		if( result == ReadExactResult::NotEnough ) {
			return;
		}
	}
	
	if( arp.operation() != 0x0001 ) {
		return;
	}

	// Update ARP frame to send reply
	arp.operation(0x0002);
	arp.tha(arp.sha());
	arp.tpa(arp.spa());
	arp.sha(HWADDR);
	arp.spa(IPADDR);

	// Send Ethernet header
	std::array<std::uint8_t, 2> protocol;
	write_all(out, header.source, false);
	write_all(out, HWADDR, false);
	write16be(protocol, 0, 0x0806);
	write_all(out, protocol, false);
	
	// Send ARP payload
	write_all(out, arp.raw, false);

	std::size_t total_frame_length = (14 + ARP::SIZE + 4);	// Ethernet header + payload + FCS
	ensure_frame_minimum_length(out, total_frame_length);
}


void ethernet_service(hls::stream<mac_data_axis>& in, hls::stream<mac_data_axis>& out)
{
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS interface axis port=in
#pragma HLS interface axis port=out

	auto header = read_header(in);
	if( !header ) return;

	switch( header.get().protocol ) {
	case 0x0806:	// ARP
		arp(header.get(), in, out);
		break;
	default:
		consume_remaining(in);
		break;
	}
}
