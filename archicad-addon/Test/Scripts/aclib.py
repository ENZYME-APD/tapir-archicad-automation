"""Makes `import aclib` work for the scripts in this folder and in manual/.

The helper lives in archicad-addon/Examples/aclib/. This loads that file by its path and puts
it in place of this module, so `aclib.RunTapirCommand`, `aclib.RunCommand`, `aclib.port` and
the rest are the very same objects the examples use. The scripts in manual/ put this folder
on sys.path themselves."""
import importlib.util
import os
import sys

_path = os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', '..', 'Examples', 'aclib', '__init__.py')
_spec = importlib.util.spec_from_file_location ('aclib', os.path.normpath (_path))
_module = importlib.util.module_from_spec (_spec)
sys.modules['aclib'] = _module
_spec.loader.exec_module (_module)
# These scripts print every command they run, as all of them did before the examples went quiet;
# the recorded outputs in Test/ExpectedOutputs depend on it.
_module.printCommands = True
