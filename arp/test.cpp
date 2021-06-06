#include "arp.hpp"
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <cassert>

static std::vector<std::uint8_t> read_all(const char* path)
{
	std::fstream fs(path);
	if( !fs ) {
		std::cerr << "Failed to open file " << path << std::endl;
		return std::vector<std::uint8_t>();
	}
	fs.seekp(0, std::ios_base::end);
	auto length = fs.tellp();
	std::vector<std::uint8_t> buffer;
	buffer.resize(length);
	fs.seekp(0, std::ios_base::beg);
	fs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
	return std::move(buffer);
}

static void write_array(hls::stream<mac_data_axis>& stream, const std::vector<std::uint8_t>& data)
{
	std::size_t n = 0;
	for(const auto& d : data) {
		stream.write(MACData(d, n == data.size() - 1));
		n++;
	}
}

bool run_test(const char* test_data_name)
{
	hls::stream<mac_data_axis> in;
	hls::stream<mac_data_axis> out;

	EthernetServiceConfig config = {
		"0xaabbccddeeff",	// hwaddr
		"0xc0a80402",		// ipaddr
	};

	std::stringstream input_path;
	input_path << "../../../data/" << test_data_name << ".input.bin";
	std::stringstream expected_path;
	expected_path << "../../../data/" << test_data_name << ".expected.bin";

	auto input = read_all(input_path.str().c_str());
	auto output= read_all(expected_path.str().c_str());

	if( input.size() == 0 || output.size() == 0 ) {
		return false;
	}
	std::cout << "input : " << input.size() << std::endl;
	std::cout << "output: " << output.size() << std::endl;

	write_array(in, input);

	ethernet_service(config, in, out);

	bool result = true;

	std::size_t bytes_output;
	for(bytes_output = 0; bytes_output < output.size() && !out.empty(); bytes_output++ ) {
		auto value = out.read();
		if( value.data != output[bytes_output] ) {
			std::printf("mismatch at %04ld expected %02x actual %02lx\n", bytes_output, output[bytes_output], value.data.to_uint64());
			result = false;
		}
	}
	if( bytes_output != output.size() ) {
		std::printf("mismatch output length expected %ld actual %ld\n", output.size(), bytes_output);
		result = false;
	}

	return result;
}

int main(int argc, char* argv[])
{
	return run_test("arp")
		//&& run_test("icmp")
		&& run_test("icmp_dump")
		? 0 : 1;
}
