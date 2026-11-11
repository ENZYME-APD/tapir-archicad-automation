#include "Sha256.hpp"

#include <cstdint>

namespace Sha256 {

static const std::uint32_t RoundConstants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static inline std::uint32_t RotateRight (std::uint32_t value, int bits)
{
    return (value >> bits) | (value << (32 - bits));
}

static void ProcessBlock (const unsigned char* block, std::uint32_t state[8])
{
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t> (block[i * 4]) << 24) |
               (static_cast<std::uint32_t> (block[i * 4 + 1]) << 16) |
               (static_cast<std::uint32_t> (block[i * 4 + 2]) << 8) |
               (static_cast<std::uint32_t> (block[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 = RotateRight (w[i - 15], 7) ^ RotateRight (w[i - 15], 18) ^ (w[i - 15] >> 3);
        const std::uint32_t s1 = RotateRight (w[i - 2], 17) ^ RotateRight (w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];
    std::uint32_t e = state[4];
    std::uint32_t f = state[5];
    std::uint32_t g = state[6];
    std::uint32_t h = state[7];
    for (int i = 0; i < 64; ++i) {
        const std::uint32_t s1 = RotateRight (e, 6) ^ RotateRight (e, 11) ^ RotateRight (e, 25);
        const std::uint32_t choice = (e & f) ^ (~e & g);
        const std::uint32_t temp1 = h + s1 + choice + RoundConstants[i] + w[i];
        const std::uint32_t s0 = RotateRight (a, 2) ^ RotateRight (a, 13) ^ RotateRight (a, 22);
        const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temp2 = s0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

std::string HexDigest (const char* data, size_t size)
{
    std::uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    const unsigned char* bytes = reinterpret_cast<const unsigned char*> (data);
    size_t offset = 0;
    while (size - offset >= 64) {
        ProcessBlock (bytes + offset, state);
        offset += 64;
    }

    // The padding: a 1 bit, zeros, and the length in bits as a 64-bit big-endian number, to a multiple of 64 bytes.
    unsigned char tail[128] = {};
    const size_t rest = size - offset;
    for (size_t i = 0; i < rest; ++i) {
        tail[i] = bytes[offset + i];
    }
    tail[rest] = 0x80;
    const size_t tailLength = (rest < 56) ? 64 : 128;
    const std::uint64_t bitLength = static_cast<std::uint64_t> (size) * 8;
    for (int i = 0; i < 8; ++i) {
        tail[tailLength - 1 - i] = static_cast<unsigned char> (bitLength >> (8 * i));
    }
    ProcessBlock (tail, state);
    if (tailLength == 128) {
        ProcessBlock (tail + 64, state);
    }

    static const char HexDigits[] = "0123456789abcdef";
    std::string result;
    result.reserve (64);
    for (int i = 0; i < 8; ++i) {
        for (int shift = 28; shift >= 0; shift -= 4) {
            result.push_back (HexDigits[(state[i] >> shift) & 0xF]);
        }
    }
    return result;
}

}
