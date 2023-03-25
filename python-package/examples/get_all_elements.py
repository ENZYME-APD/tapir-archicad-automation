import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src'))
from tapir_py import core

ac = core.Command.create()
elements = ac.GetAllElements()
for element in elements:
    print(element.guid)
