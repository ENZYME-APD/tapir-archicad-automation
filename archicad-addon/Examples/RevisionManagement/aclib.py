"""Makes `import aclib` work for the examples in this folder.

The helper is the archicad-addon/Examples/aclib/ package, one folder up. This loads it by its
path and puts it in place of this module, so `python Examples/RevisionManagement/<CommandName>.py` works
from any working directory and `aclib.RunTapirCommand`, `aclib.port`, ... are the package's own."""
import importlib.util
import os
import sys

_path = os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', 'aclib', '__init__.py')
_spec = importlib.util.spec_from_file_location ('aclib', os.path.normpath (_path))
_module = importlib.util.module_from_spec (_spec)
sys.modules['aclib'] = _module
_spec.loader.exec_module (_module)
