"""
Manual verification for issue #681: SaveProject used to fail from any window
but the Floor Plan, because the parameterless ACAPI_ProjectOperation_Save
saves the content of the current window, and a 3D or section window has no
file of its own.

The script records the project file's modification time, switches to the 3D
window, runs SaveProject, and checks both the command's answer and that the
file on disk really changed. It then restores the Floor Plan window.

Not part of the auto-discovered Examples/ (see test_examples.py +
ExpectedOutputs/): that harness runs against Test/TestProject.pla, and an
archive opens read-only, so ACAPI_ProjectOperation_Save refuses it with
APIERR_READONLY from every window - the Floor Plan included - and nothing
about #681 can be observed there. (SaveProjectAsArchive does not help: it
writes a copy but leaves the original as the open project, so a save after
it still targets the original.)

Run manually, with Archicad and the Tapir Add-On running and a normal,
already-saved .pln open - a throwaway copy, since it gets written:
    python manual_tests/test_save_project_from_3d_window.py
(from the Examples/ folder)
"""
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import aclib  # noqa: E402

passes = 0
fails = 0


def check(label, condition, detail=''):
    global passes, fails
    if condition:
        passes += 1
        print('PASS: {}'.format(label))
    else:
        fails += 1
        print('FAIL: {} {}'.format(label, detail))


def run(command, parameters=None):
    return aclib.RunTapirCommand(command, parameters or {}, debug=False)


info = run('GetProjectInfo')
projectPath = info.get('projectPath')
check('a saved project is open', bool(projectPath) and not info.get('isUntitled'), str(info))
check('the open project is not an archive (.pla opens read-only)',
      not str(projectPath).lower().endswith('.pla'), str(projectPath))
if fails:
    print('{} passed, {} failed'.format(passes, fails))
    sys.exit(1)

# 1) The 3D window: the exact situation of #681.
check('the 3D window can be activated', run('ChangeWindow', {'windowType': '3DModel'}).get('success') is True)
check('the current window is the 3D window', run('GetCurrentWindowType').get('currentWindowType') == '3DModel')

before = os.path.getmtime(projectPath)
time.sleep(1.1)  # so a changed modification time is unambiguous
result = run('SaveProject')
after = os.path.getmtime(projectPath)

check('SaveProject succeeds from the 3D window', result.get('success') is True, str(result))
check('the project file on disk was written', after > before,
      'mtime {} -> {}'.format(before, after))
check('the 3D window is still the current window after the save',
      run('GetCurrentWindowType').get('currentWindowType') == '3DModel')

# 2) Back on the Floor Plan the save must still work, and the window must be restored.
run('ChangeWindow', {'windowType': 'FloorPlan'})
check('the Floor Plan window is restored', run('GetCurrentWindowType').get('currentWindowType') == 'FloorPlan')
check('SaveProject still succeeds from the Floor Plan', run('SaveProject').get('success') is True)

print('{} passed, {} failed'.format(passes, fails))
sys.exit(0 if fails == 0 else 1)
