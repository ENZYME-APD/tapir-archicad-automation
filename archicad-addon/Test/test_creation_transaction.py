"""Fault-injection tests for the actual C++ creation transaction helper.

Run with Python 3 and a C++17 compiler, without Archicad or its DevKit:
    python3 archicad-addon/Test/test_creation_transaction.py

Only the SDK boundary is substituted. The helper is compiled directly from the
production source so these tests also cover a failure after IDs were collected.
They do not simulate Archicad geometry or prove how its transaction engine works.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[1] / "Sources" / "ExtendedElementCommands.cpp"

STUBS = r'''
#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>

using GSErrCode = int;
constexpr int NoError = 0;
namespace GS {
    using String = std::string;
    struct ObjectState {
        int error = 0;
        int id = 0;
        std::vector<ObjectState> elements;
    };
    template<typename T> using Array = std::vector<T>;
}
struct API_NotifyElementType { int notifID = 0; };
constexpr int APINotifyElement_BeginEvents = 1;
constexpr int APINotifyElement_EndEvents = 2;
std::vector<int> notifications;
struct AddElementNotificationClientCommand {
    static void ElementEventHandlerProc (API_NotifyElementType* event) {
        notifications.push_back (event->notifID);
    }
};
int failureMode = 0;
template<typename Func>
GSErrCode ACAPI_CallUndoableCommand (const GS::String&, Func&& callback) {
    if (failureMode == 1) return -1234;
    const auto result = callback ();
    return failureMode == 2 ? -5678 : result;
}
GS::ObjectState CreateErrorResponse (GSErrCode code, const char*) {
    return {code, 0, {}};
}
GS::ObjectState CreateElementListResponse (const GS::Array<GS::ObjectState>& items) {
    return {0, 0, items};
}
'''

CHECKS = r'''
int main (int argc, char** argv) {
    assert (argc == 2);
    failureMode = std::atoi (argv[1]);
    bool called = false;
    const auto result = ExecuteCreateWithElements ("Create test elements", [&](auto& items) {
        called = true;
        if (failureMode != 3) {
            items.push_back ({0, 42, {}});
            items.push_back ({-999, 0, {}});
            items.push_back ({0, 43, {}});
        }
    });
    assert ((notifications == std::vector<int> {1, 2}));
    if (failureMode == 1 || failureMode == 2) {
        assert (called == (failureMode == 2));
        assert (result.error == (failureMode == 1 ? -1234 : -5678));
        assert (result.elements.empty ());
    } else {
        assert (called && result.error == NoError);
        if (failureMode == 3) {
            assert (result.elements.empty ());
        } else {
            assert (result.elements.size () == 3);
            assert (result.elements[0].id == 42);
            assert (result.elements[1].error == -999);
            assert (result.elements[2].id == 43);
        }
    }
}
'''


class CreationTransactionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((path for name in ("c++", "clang++", "g++")
                         if (path := shutil.which(name))), None)
        if compiler is None:
            raise unittest.SkipTest("A C++17 compiler is required")
        source = SOURCE.read_text()
        start = source.index("template<typename Func>\nGS::ObjectState ExecuteCreateWithElements")
        end = source.index("template<typename Func>\nGS::ObjectState ExecuteModifyWithResults", start)
        temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(temp.cleanup)
        cpp = Path(temp.name) / "transaction.cpp"
        cls.binary = Path(temp.name) / "transaction"
        cpp.write_text(STUBS + source[start:end] + CHECKS)
        subprocess.run([compiler, "-std=c++17", str(cpp), "-o", str(cls.binary)], check=True)

    def check_mode(self, mode):
        result = subprocess.run([str(self.binary), str(mode)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_rejected_transaction_returns_original_error(self):
        self.check_mode(1)

    def test_failed_commit_does_not_report_collected_ids(self):
        self.check_mode(2)

    def test_success_preserves_item_order_and_individual_errors(self):
        self.check_mode(0)

    def test_successful_empty_batch_is_still_successful(self):
        self.check_mode(3)


if __name__ == "__main__":
    unittest.main()
