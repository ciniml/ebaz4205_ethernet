#include "arp.hpp"
#include <cassert>

template<std::size_t N, typename Element>
static int compare_array(const std::array<Element, N>& lhs, const std::array<Element, N>& rhs)
{
	for(std::size_t i = 0; i < N; i++) {
#pragma HLS UNROLL
		auto l = lhs[i];
		auto r = rhs[i];
		if( l < r ) {
			return -1;
		}
		else if( r > l ) {
			return 1;
		}
	}
	return 0;
}

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
static inline std::uint16_t swap_bytes16(std::uint16_t value) {
	return (value >> 8) | (value << 8);
}
template<typename Array>
static inline void write16be(Array& a, std::size_t offset, std::uint16_t value) {
	a[offset + 0] = value >> 8;
	a[offset + 1] = value & 0xff;
}

template<typename Array>
static inline std::uint16_t read32be(const Array& a, std::size_t offset) {
	return (a[offset + 0] << 24)  | (a[offset + 1] << 16)  | (a[offset + 2] << 8) | (a[offset + 3]);
}
template<typename Array>
static inline void write32be(Array& a, std::size_t offset, std::uint16_t value) {
	a[offset + 0] = value >> 24;
	a[offset + 1] = (value >> 16) & 0xff;
	a[offset + 2] = (value >> 8) & 0xff;
	a[offset + 3] = value & 0xff;
}
template<typename Array, std::size_t N>
static inline std::array<std::uint8_t, N>& read_array(const Array& a, std::size_t offset, std::array<std::uint8_t, N>& buffer) {
	for(std::size_t i = 0; i < N; i++) {
#pragma HLS UNROLL
		buffer[i] = a[offset + i];
	}
	return buffer;
}
template<typename Array, std::size_t N>
static inline void write_array(Array& a, std::size_t offset, const std::array<std::uint8_t, N>& data) {
	for(std::size_t i = 0; i < N; i++) {
#pragma HLS UNROLL
		a[offset + i] = data[i];
	}
}

template<typename Array>
static inline HardwareAddress read_hwaddr(const Array& a, std::size_t offset) {
	HardwareAddress address;
	return read_array(a, offset, address);
}
template<typename Array>
static inline void write_hwaddr(Array& a, std::size_t offset, const HardwareAddress& address) {
	write_array(a, offset, address);
}
template<typename Array>
static inline IPAddress read_ipaddr(const Array& a, std::size_t offset) {
	IPAddress address;
	return read_array(a, offset, address);
}
template<typename Array>
static inline void write_ipaddr(Array& a, std::size_t offset, const IPAddress& address) {
	write_array(a, offset, address);
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

template<std::size_t N>
static std::uint16_t calculate_internet_checksum(const std::array<std::uint8_t, N>& a, const std::uint16_t initial = 0, const std::size_t length = N)
{
	std::uint32_t checksum = initial;
	for(std::size_t i = 0; i < length/2; i++) {
#pragma HLS PIPELINE II=2
		checksum += read16be(a, i*2);
	}
	if( (length & 1) != 0 ) {
		checksum += static_cast<std::uint16_t>(a[length/2]) << 8;
	}
	checksum = (checksum & 0xffff) + (checksum >> 16);
	checksum = (checksum & 0xffff) + (checksum >> 16);
	return checksum & 0xffff;
}
static std::uint16_t calculate_internet_checksum(const std::uint16_t* a, const std::uint16_t initial, const std::size_t length)
{
	std::uint32_t checksum = initial;
	for(std::size_t i = 0; i < length/2; i++) {
#pragma HLS PIPELINE II=1
		checksum += swap_bytes16(a[i]);
	}
	if( (length & 1) != 0 ) {
		checksum += a[length/2] << 8;
	}
	checksum = (checksum & 0xffff) + (checksum >> 16);
	checksum = (checksum & 0xffff) + (checksum >> 16);
	return checksum & 0xffff;
}

struct IPv4
{
	static constexpr const std::size_t SIZE = 20;
	std::array<std::uint8_t, SIZE> raw;
	
	std::uint8_t version() const            { return this->raw[0]; }
	std::uint8_t type() const               { return this->raw[1]; }
	std::uint16_t length() const            { return read16be(this->raw, 2); }
	std::uint16_t identification() const    { return read16be(this->raw, 4); }
	std::uint16_t flags_and_offset() const  { return read16be(this->raw, 6); }
	std::uint8_t time_to_live() const       { return this->raw[8]; }
	std::uint8_t protocol() const           { return this->raw[9]; }
	std::uint16_t header_checksum() const   { return read16be(this->raw, 10); }
	IPAddress source() const                { return read_ipaddr(this->raw, 12); }
	IPAddress destination() const           { return read_ipaddr(this->raw, 16); }
	
	void version(std::uint8_t value)            { this->raw[0] = value; }
	void type(std::uint8_t value)               { this->raw[1] = value; }
	void length(std::uint16_t value)            { write16be(this->raw, 2, value); }
	void identification(std::uint16_t value)    { write16be(this->raw, 4, value); }
	void flags_and_offset(std::uint16_t value)  { write16be(this->raw, 6, value); }
	void time_to_live(std::uint8_t value)       { this->raw[8] = value; }
	void protocol(std::uint8_t value)           { this->raw[9] = value; }
	void header_checksum(std::uint16_t value)   { write16be(this->raw, 10, value); }
	void source(const IPAddress& value)         { write_ipaddr(this->raw, 12, value); }
	void destination(const IPAddress& value)    { write_ipaddr(this->raw, 16, value); }

	void fill_checksum() 
	{
		this->header_checksum(0);
		auto checksum = calculate_internet_checksum(this->raw);
		this->header_checksum(~checksum);
	}
};

struct ICMP
{
	static constexpr const std::size_t SIZE = 12;
	std::array<std::uint8_t, SIZE> raw;
	std::uint8_t type() const { return this->raw[0]; }
	std::uint8_t code() const { return this->raw[1]; }
	std::uint16_t checksum() const { return read16be(this->raw, 2); }
	std::uint16_t identifier() const { return read16be(this->raw, 4); }
	std::uint16_t sequence_number() const { return read16be(this->raw, 6); }
	std::uint32_t payload() const { return read32be(this->raw, 8); }

	void type(std::uint8_t value) { this->raw[0] = value; }
	void code(std::uint8_t value) { this->raw[1] = value; }
	void checksum(std::uint16_t value) { write16be(this->raw, 2, value); }
	void identifier(std::uint16_t value) { write16be(this->raw, 4, value); }
	void sequence_number(std::uint16_t value) { write16be(this->raw, 6, value); }
	void payload(std::uint32_t value) { write32be(this->raw, 8, value); }

	void fill_checksum(const std::uint16_t* payload, std::size_t payload_length)
	{
		this->checksum(0);
		auto checksum = calculate_internet_checksum(this->raw);
		checksum = calculate_internet_checksum(payload, checksum, payload_length);
		this->checksum(~checksum);
	}
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
		buffer[i] = data.data;
		last = data.last;
		if( i < N - 1 && data.last ) return ReadExactResult::NotEnough;
	}
	return last ? ReadExactResult::Exact : ReadExactResult::Remaining;
}

template<std::size_t N>
static inline void write_all(hls::stream<mac_data_axis>& out, const std::array<std::uint8_t, N>& data, bool is_last, std::size_t length = N)
{
	for(std::size_t i = 0; i < length; i++) {
#pragma HLS PIPELINE II=1
		mac_data_axis mac_data;
		mac_data.data = data[i];
		mac_data.keep = 1;
		mac_data.last = is_last && i == length - 1;
		out.write(mac_data);
	}
}

static inline void write_all(hls::stream<mac_data_axis>& out, const std::uint8_t* data, bool is_last, std::size_t length)
{
	for(std::size_t i = 0; i < length; i++) {
#pragma HLS PIPELINE II=1
		mac_data_axis mac_data;
		mac_data.data = data[i];
		mac_data.keep = 1;
		mac_data.last = is_last && i == length - 1;
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

static inline std::size_t read_payload(hls::stream<mac_data_axis>& in, std::uint8_t* buffer, std::size_t buffer_length)
{
	for(std::size_t i = 0;; i++) {
#pragma HLS PIPELINE ii = 1
		auto d = in.read();
		if( i < buffer_length ) {
			buffer[i] = d.data;
		}
		if( d.last ) return i < buffer_length ? i + 1 : buffer_length;
	}
	return 0;
}

static inline void ensure_frame_minimum_length(hls::stream<mac_data_axis>& out, std::size_t length)
{
	if( length < 64 ) {
		std::size_t padding_count = 64 - length;
		for(std::size_t i = 0; i < padding_count; i++) {
#pragma HLS PIPELINE II=1
			out.write(MACData(0, i == padding_count - 1)); // Fill zero to ensure frame length is at least 64 octets.
		}
	}
}

static void arp(const EthernetServiceConfig& config, const EthernetHeader& header, hls::stream<mac_data_axis>& in, hls::stream<mac_data_axis>& out)
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
	arp.sha(config.get_hardware_address());
	arp.spa(config.get_ip_address());

	// Send Ethernet header
	std::array<std::uint8_t, 2> protocol;
	write_all(out, header.source, false);
	write_all(out, config.get_hardware_address(), false);
	write16be(protocol, 0, 0x0806);
	write_all(out, protocol, false);
	
	// Send ARP payload
	write_all(out, arp.raw, false);

	std::size_t total_frame_length = (14 + ARP::SIZE + 4);	// Ethernet header + payload + FCS
	ensure_frame_minimum_length(out, total_frame_length);
}


template<std::size_t MAX_PAYLOAD_LENGTH=1500>
static void icmp_reply(const EthernetServiceConfig& config, const EthernetHeader& header, hls::stream<mac_data_axis>& in, hls::stream<mac_data_axis>& out)
{
	IPv4 ip;
	{
		auto result =  read_exact(in, ip.raw);
		if( result != ReadExactResult::Remaining ) {
			return;
		}
	}

	if( compare_array(ip.destination(), config.get_ip_address()) != 0 || ip.protocol() != 0x01 ) {
		consume_remaining(in);
		return;
	}

	ICMP icmp;
	std::array<std::uint16_t, MAX_PAYLOAD_LENGTH/2> payload;
//#pragma HLS BIND_STORAGE variable=payload type=ram_2p
	std::size_t payload_length = 0;
	{
		auto result =  read_exact(in, icmp.raw);
		if( result == ReadExactResult::Remaining ) {
			payload_length = read_payload(in, reinterpret_cast<std::uint8_t*>(payload.data()), MAX_PAYLOAD_LENGTH);
		}
		if( result == ReadExactResult::NotEnough ) {
			return;
		}
	}
	assert(payload_length <= MAX_PAYLOAD_LENGTH);
	
	// accept only ICMP echo request.
	if( icmp.type() != 8 ) {
		return;
	}
	
	// Construct IP header.
	ip.destination(ip.source());
	ip.source(config.get_ip_address());
	ip.fill_checksum();

	// Construct reply packet.
	icmp.type(0);	// Just changing type field to 0 (ICMP echo reply) is enough.
	icmp.fill_checksum(payload.data(), payload_length);

	// Send Ethernet header
	std::array<std::uint8_t, 2> protocol;
	write_all(out, header.source, false);
	write_all(out, config.get_hardware_address(), false);
	write16be(protocol, 0, 0x0800);
	write_all(out, protocol, false);
	
	// Send IP header
	write_all(out, ip.raw, false);

	// Send ICMP packet
	write_all(out, icmp.raw, false);

	// Send payload
	std::size_t total_frame_length = (14 + (ip.length() - IPv4::SIZE) + 4);	// Ethernet header + payload + FCS
	write_all(out, reinterpret_cast<std::uint8_t*>(payload.data()), total_frame_length >= 64, payload_length);
	ensure_frame_minimum_length(out, total_frame_length);
}

void ethernet_service(const EthernetServiceConfig& config, hls::stream<mac_data_axis>& in, hls::stream<mac_data_axis>& out)
{
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS INTERFACE ap_stable register port=config
#pragma HLS interface axis port=in
#pragma HLS interface axis port=out

	auto header = read_header(in);
	if( !header ) return;

	// Check destination
	if( compare_array(header.get().destination, config.get_hardware_address()) != 0 && compare_array(header.get().destination, HardwareAddress({0xff, 0xff, 0xff, 0xff, 0xff, 0xff})) != 0 ) {
		consume_remaining(in);
		return;
	}

	switch( header.get().protocol ) {
	case 0x0800:	// IP
		icmp_reply(config, header.get(), in, out);
		break;
	case 0x0806:	// ARP
		arp(config, header.get(), in, out);
		break;
	default:
		consume_remaining(in);
		break;
	}
}
