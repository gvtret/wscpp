#include <cstring>
#include <wscpp/extensions/permessage_deflate.hpp>
#include <zlib.h>

namespace wscpp {
namespace extensions {

namespace {

const char kOffer[] = "permessage-deflate; client_no_context_takeover; server_no_context_takeover";

bool ends_with_deflate_tail(const uint8_t *data, std::size_t size) {
    return size >= 4 && data[size - 4] == 0x00 && data[size - 3] == 0x00 &&
           data[size - 2] == 0xff && data[size - 1] == 0xff;
}

} // namespace

const char *permessage_deflate_offer() {
    return kOffer;
}

bool header_offers_permessage_deflate(const std::string &extensions_header) {
    return extensions_header.find("permessage-deflate") != std::string::npos;
}

bool compress_message(const uint8_t *data, std::size_t size, std::vector<uint8_t> &out) {
    if (size == 0) {
        out.clear();
        return true;
    }

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) !=
        Z_OK) {
        return false;
    }

    std::vector<uint8_t> buffer(size + 32);
    strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(data));
    strm.avail_in = static_cast<uInt>(size);
    strm.next_out = buffer.data();
    strm.avail_out = static_cast<uInt>(buffer.size());

    const int ret = deflate(&strm, Z_SYNC_FLUSH);
    const std::size_t produced = buffer.size() - strm.avail_out;
    deflateEnd(&strm);

    if (ret != Z_OK || produced < 4 || !ends_with_deflate_tail(buffer.data(), produced)) {
        return false;
    }

    out.assign(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(produced - 4));
    return true;
}

bool decompress_message(const uint8_t *data, std::size_t size, std::vector<uint8_t> &out) {
    if (size == 0) {
        out.clear();
        return true;
    }

    std::vector<uint8_t> input(size + 4);
    std::memcpy(input.data(), data, size);
    input[size] = 0x00;
    input[size + 1] = 0x00;
    input[size + 2] = 0xff;
    input[size + 3] = 0xff;

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, -15) != Z_OK) {
        return false;
    }

    std::vector<uint8_t> buffer(size * 8 + 64);
    strm.next_in = input.data();
    strm.avail_in = static_cast<uInt>(input.size());
    strm.next_out = buffer.data();
    strm.avail_out = static_cast<uInt>(buffer.size());

    const int ret = inflate(&strm, Z_SYNC_FLUSH);
    const std::size_t produced = buffer.size() - strm.avail_out;
    inflateEnd(&strm);

    if (ret != Z_OK && ret != Z_BUF_ERROR) {
        return false;
    }

    out.assign(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(produced));
    return true;
}

} // namespace extensions
} // namespace wscpp
