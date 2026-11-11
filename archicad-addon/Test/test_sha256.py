"""Tests the Script Manager's SHA-256 (Sources/Sha256.cpp) against Python's hashlib.

Needs no Archicad: Sha256.cpp uses only the C++ standard library. It is compiled together with a small program by
the first C++ compiler found (c++, g++, clang++ or cl), which prints the digest of each test input; the digests must
be the ones hashlib gives. The inputs cover the padding's edge cases (55, 56, 63, 64 and 65 bytes) and several
blocks. Exits with 1 when a digest differs or nothing compiles.

    python archicad-addon/Test/test_sha256.py
"""

import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

SOURCES_FOLDER = os.path.normpath (os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', 'Sources'))

SIZES = [0, 1, 3, 55, 56, 57, 63, 64, 65, 119, 120, 127, 128, 1000, 100000]

TEST_PROGRAM = r'''
#include "Sha256.hpp"
#include <cstdio>
#include <string>

int main ()
{
    const size_t sizes[] = { %SIZES% };
    for (size_t size : sizes) {
        std::string data;
        for (size_t i = 0; i < size; ++i) {
            data.push_back (static_cast<char> ((i * 31 + 7) & 0xFF));
        }
        std::printf ("%zu %s\n", size, Sha256::HexDigest (data.data (), data.size ()).c_str ());
    }
    return 0;
}
'''


def Expected ():
    lines = []
    for size in SIZES:
        data = bytes (((i * 31 + 7) & 0xFF) for i in range (size))
        lines.append ('{} {}'.format (size, hashlib.sha256 (data).hexdigest ()))
    return lines


def Main ():
    with tempfile.TemporaryDirectory () as folder:
        sourceFile = os.path.join (folder, 'test_sha256.cpp')
        with open (sourceFile, 'w', encoding='utf-8') as f:
            f.write (TEST_PROGRAM.replace ('%SIZES%', ', '.join (str (size) for size in SIZES)))
        exeFile = os.path.join (folder, 'test_sha256.exe' if os.name == 'nt' else 'test_sha256')
        sha256Source = os.path.join (SOURCES_FOLDER, 'Sha256.cpp')

        for compiler in ('c++', 'g++', 'clang++', 'cl'):
            path = shutil.which (compiler)
            if path is None:
                continue
            if compiler == 'cl':
                command = [path, '/nologo', '/EHsc', '/std:c++17', '/I', SOURCES_FOLDER, sourceFile, sha256Source, '/Fe' + exeFile, '/Fo' + folder + os.sep]
            else:
                command = [path, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', SOURCES_FOLDER, sourceFile, sha256Source, '-o', exeFile]
            build = subprocess.run (command, capture_output=True, text=True)
            if build.returncode != 0:
                print (build.stdout + build.stderr)
                print ('Compiling with {} failed.'.format (compiler))
                return 1
            run = subprocess.run ([exeFile], capture_output=True, text=True)
            actual = run.stdout.splitlines ()
            failures = 0
            for expectedLine, actualLine in zip (Expected (), actual + [''] * len (SIZES)):
                if expectedLine != actualLine:
                    print ('FAIL: expected "{}", got "{}"'.format (expectedLine, actualLine))
                    failures += 1
            print ('{} case(s) failed'.format (failures))
            return 0 if failures == 0 and run.returncode == 0 else 1

    print ('No C++ compiler found (c++, g++, clang++ or cl).')
    return 1


if __name__ == '__main__':
    sys.exit (Main ())
