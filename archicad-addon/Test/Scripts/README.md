# Test scripts

Scripts that check the behaviour of the Tapir commands against a running Archicad.

- `<name>.py` with a recorded output in [`../ExpectedOutputs/<name>.py.output`](../ExpectedOutputs/)
  is a regression test: [`../test_examples.py`](../test_examples.py) opens `TestProject.pla`
  before each script, runs it and compares its (masked) output with the recorded one. Keep the
  name of such a script, or its recorded output no longer matches.
- The other scripts here are checks to run by hand: exhaustive, round-trip and edge-case tests
  that print their own PASS / FAIL or stop on a failed `assert`. Run them on a test project.
- [`manual/`](manual/) holds tests that need a person in front of Archicad (locked layers, the
  3D window, ...); `test_examples.py` does not run them.

Running:

```
cd archicad-addon/Test
python test_examples.py                          # every script here and every example
python test_examples.py get_libraries.py         # only the named ones
python record_expected_output.py <name>.py       # record one output against the open project
python Scripts/test_drawing_clip.py              # one check by hand
```

`test_examples.py` also runs every example of [`../../Examples/`](../../Examples/) as a smoke test,
with its recorded output in `../ExpectedOutputs/Examples/<GroupFolder>/`, except the examples that
cannot run unattended (listed in `EXAMPLES_NOT_SMOKE_TESTED`). A script without a recorded output
passes and gets one.

`aclib.py` here only loads [`../../Examples/aclib/`](../../Examples/aclib/), the helper the scripts
import (the scripts in `manual/` put this folder on their path).
