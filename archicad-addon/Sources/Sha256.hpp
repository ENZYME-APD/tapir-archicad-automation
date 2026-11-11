#pragma once

// A small self-contained SHA-256 (FIPS 180-4), for the Script Manager's check of a downloaded script against the
// catalogue (#2408). The DevKit has no hashing, and Tapir adds no libraries, so it is written here with the
// standard library only.

#include <cstddef>
#include <string>

namespace Sha256 {

// The SHA-256 of the bytes, as 64 lowercase hexadecimal digits (as Python's hashlib.sha256 (...).hexdigest ()).
std::string HexDigest (const char* data, size_t size);

}
