#include "NestDAQDecoder.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: test_capture CAPTURE\n"; return 2; }
    std::ifstream f(argv[1], std::ios::binary);
    if (!f) { std::cerr << "cannot open capture\n"; return 2; }
    NestDAQDecoder decoder;
    std::vector<std::vector<std::byte>> parts;
    std::uint64_t frames=0, hits=0, low=0, high=0, mapped=0;
    std::map<std::uint32_t, std::uint64_t> fem_counts;
    std::ostringstream ignored;
    auto* saved = std::cout.rdbuf(ignored.rdbuf());
    while (true) {
        std::uint32_t length=0, more=0;
        if (!f.read(reinterpret_cast<char*>(&length), 4)) { if (!f.eof() || f.gcount()!=0) throw std::runtime_error("truncated header"); break; }
        if (!f.read(reinterpret_cast<char*>(&more), 4) || more>1 || length>10000000) throw std::runtime_error("invalid header");
        std::vector<std::byte> part(length);
        if (!f.read(reinterpret_cast<char*>(part.data()), length)) throw std::runtime_error("truncated part");
        parts.push_back(std::move(part));
        if (more) continue;
        auto data=decoder.decode(parts); ++frames; hits+=data.tdc_hits.size();
        for (const auto& hit:data.tdc_hits) { ++fem_counts[hit.fem_id]; if (hit.fem_type==6) ++low; if (hit.fem_type==5) ++high; if (hit.fem_id==0xc0a80a28 && hit.channel<=47) ++mapped; }
        parts.clear(); ignored.str(""); ignored.clear();
    }
    std::cout.rdbuf(saved);
    for (auto [id, n]:fem_counts) std::cout << "fem=0x" << std::hex << id << std::dec << " hits=" << n << "\n";
    if (!parts.empty()) throw std::runtime_error("truncated multipart message");
    std::cout << "frames=" << frames << " hits=" << hits << " low=" << low << " high=" << high << " mapped=" << mapped << '\n';
    return frames==2815 && hits==68956 && mapped==68956 && high>0 ? 0 : 1;
}
