"""Tests the rules of a palette script's min-tapir-version and archicad-versions keys (Sources/ScriptRequirements.hpp).

Needs no Archicad: the header uses only the C++ standard library. It is compiled together with a table of cases by
the first C++ compiler found (c++, g++, clang++ or cl), then run. Exits with 1 when a case fails or nothing compiles.

    python archicad-addon/Test/test_script_requirements.py
"""

import os
import shutil
import subprocess
import sys
import tempfile

SOURCES_FOLDER = os.path.normpath (os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', 'Sources'))

# (min-tapir-version, installed version, expected result)
MIN_TAPIR_VERSION_CASES = [
    ('', '1.8.1', 'NotGiven'),
    ('1.8.1', '1.8.1', 'Satisfied'),
    ('1.8.0', '1.8.1', 'Satisfied'),
    ('1.8.2', '1.8.1', 'TooOld'),
    ('1.9.2', '1.10.0', 'Satisfied'),    # numeric, not text: 1.10.0 is newer than 1.9.2
    ('1.10.0', '1.9.2', 'TooOld'),
    ('2.0.0', '1.99.99', 'TooOld'),
    ('1.99.99', '2.0.0', 'Satisfied'),
    ('99.0.0', '1.8.1', 'TooOld'),
    ('0.0.0', '1.8.1', 'Satisfied'),
    ('01.8.1', '1.8.1', 'Satisfied'),
    ('1.8', '1.8.1', 'Malformed'),
    ('1.8.1.0', '1.8.1', 'Malformed'),
    ('v1.8.1', '1.8.1', 'Malformed'),
    ('1.8.1-beta', '1.8.1', 'Malformed'),
    (' 1.8.1', '1.8.1', 'Malformed'),
    ('1..1', '1.8.1', 'Malformed'),
    ('1.8.', '1.8.1', 'Malformed'),
    ('latest', '1.8.1', 'Malformed'),
    ('1.8.1234567890', '1.8.1', 'Malformed'),
    ('1.8.1', 'dev', 'Satisfied'),       # an installed version that is not N.N.N never blocks
]

# (archicad-versions, running main version, expected IsArchicadVersionListed)
ARCHICAD_VERSIONS_CASES = [
    ([], 29, True),                      # absent: no check
    ([28, 29, 30], 29, True),
    ([28, 29, 30], 27, False),
    ([28], 29, False),
    ([29], 29, True),
    ([28, 29], 0, True),                 # unknown running version: no warning
]

TEST_PROGRAM = r'''
#include "ScriptRequirements.hpp"
#include <cstdio>
#include <cstring>

static const char* Name (ScriptRequirements::MinTapirVersionResult result)
{
    switch (result) {
        case ScriptRequirements::MinTapirVersionResult::NotGiven:  return "NotGiven";
        case ScriptRequirements::MinTapirVersionResult::Malformed: return "Malformed";
        case ScriptRequirements::MinTapirVersionResult::Satisfied: return "Satisfied";
        case ScriptRequirements::MinTapirVersionResult::TooOld:    return "TooOld";
    }
    return "?";
}

int main ()
{
    int failures = 0;
    struct MinCase { const char* minVersion; const char* installed; const char* expected; };
    const MinCase minCases[] = {
%MIN_CASES%
    };
    for (const MinCase& c : minCases) {
        const char* actual = Name (ScriptRequirements::CheckMinTapirVersion (c.minVersion, c.installed));
        if (std::strcmp (actual, c.expected) != 0) {
            std::printf ("FAIL: min-tapir-version \"%s\" with Tapir \"%s\": %s, expected %s\n", c.minVersion, c.installed, actual, c.expected);
            ++failures;
        }
    }

    struct AcCase { std::vector<int> versions; int running; bool expected; };
    const AcCase acCases[] = {
%AC_CASES%
    };
    for (const AcCase& c : acCases) {
        const bool actual = ScriptRequirements::IsArchicadVersionListed (c.versions, c.running);
        if (actual != c.expected) {
            std::printf ("FAIL: archicad-versions (%d items) with Archicad %d: %d, expected %d\n", (int) c.versions.size (), c.running, (int) actual, (int) c.expected);
            ++failures;
        }
    }

    std::printf ("%d case(s) failed\n", failures);
    return failures == 0 ? 0 : 1;
}
'''


def CppString (text):
    return '"' + text.replace ('\\', '\\\\').replace ('"', '\\"') + '"'


def MakeProgram ():
    minCases = ',\n'.join ('        {{ {}, {}, {} }}'.format (CppString (m), CppString (i), CppString (e)) for m, i, e in MIN_TAPIR_VERSION_CASES)
    acCases = ',\n'.join ('        {{ {{ {} }}, {}, {} }}'.format (', '.join (str (v) for v in versions), running, 'true' if expected else 'false')
                          for versions, running, expected in ARCHICAD_VERSIONS_CASES)
    return TEST_PROGRAM.replace ('%MIN_CASES%', minCases).replace ('%AC_CASES%', acCases)


def Main ():
    with tempfile.TemporaryDirectory () as folder:
        sourceFile = os.path.join (folder, 'test_script_requirements.cpp')
        with open (sourceFile, 'w', encoding='utf-8') as f:
            f.write (MakeProgram ())
        exeFile = os.path.join (folder, 'test_script_requirements.exe' if os.name == 'nt' else 'test_script_requirements')

        for compiler in ('c++', 'g++', 'clang++', 'cl'):
            path = shutil.which (compiler)
            if path is None:
                continue
            if compiler == 'cl':
                command = [path, '/nologo', '/EHsc', '/std:c++17', '/I', SOURCES_FOLDER, sourceFile, '/Fe' + exeFile, '/Fo' + folder + os.sep]
            else:
                command = [path, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', SOURCES_FOLDER, sourceFile, '-o', exeFile]
            build = subprocess.run (command, capture_output=True, text=True)
            if build.returncode != 0:
                print (build.stdout + build.stderr)
                print ('Compiling with {} failed.'.format (compiler))
                return 1
            run = subprocess.run ([exeFile], capture_output=True, text=True)
            print (run.stdout, end='')
            return run.returncode

    print ('No C++ compiler found (c++, g++, clang++ or cl).')
    return 1


if __name__ == '__main__':
    sys.exit (Main ())
