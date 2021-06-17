#pragma once
// #include <gmp.h>
// #define __gmp_const const

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

static HardwareAddress to_hardware_address(const ap_uint<8*6>& value)
{
	HardwareAddress result;
	result[5] = value( 7,  0);
	result[4] = value(15,  8);
	result[3] = value(23, 16);
	result[2] = value(31, 24);
	result[1] = value(39, 32);
	result[0] = value(47, 40);
	return result;
}
static IPAddress to_ip_address(const ap_uint<8*4>& value)
{
	IPAddress result;
	result[3] = value( 7,  0);
	result[2] = value(15,  8);
	result[1] = value(23, 16);
	result[0] = value(31, 24);
	return result;
}


struct EthernetServiceConfig
{
	ap_uint<8*6> hardware_address;
	ap_uint<8*4> ip_address;

	HardwareAddress get_hardware_address() const { return to_hardware_address(this->hardware_address); }
	IPAddress get_ip_address() const { return to_ip_address(this->ip_address); }
};

void ethernet_service(const EthernetServiceConfig& config, hls::stream<mac_data_axis>& in, hls::stream<mac_data_axis>& out);
