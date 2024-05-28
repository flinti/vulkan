#include "Utility.h"

#include <spdlog/fmt/fmt.h>
#include <fstream>

std::vector<std::byte> Utility::readFile(std::filesystem::path path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	auto len = file.tellg();
	if (len <= -1) {
		throw std::runtime_error(fmt::format("tellg on file {} failed", path.string()));
	}
	if (len == 0) {
		return std::vector<std::byte>{};
	}
	std::vector<std::byte> buf(len);
	file.seekg(0).read(reinterpret_cast<char *>(buf.data()), len);

	if(file.fail()) {
		throw std::runtime_error(fmt::format("reading file {} failed", path.string()));
	}

	file.close();

	return buf;
}

