import argparse
import os
import subprocess

# Starts a new Archicad with the dialog auto close already on, so the dialogs it shows
# while it opens the project (before any command can be sent) are closed too.
# The environment variable is passed to this one Archicad process only.
# Usage: python start_archicad_with_dialog_auto_close.py <Archicad executable> [<project file>]
# (on macOS the executable is inside the app: .../Archicad 29.app/Contents/MacOS/ARCHICAD)

parser = argparse.ArgumentParser ()
parser.add_argument ('archicadPath', type=str)
parser.add_argument ('projectFilePath', type=str, nargs='?')
args = parser.parse_args ()

env = dict (os.environ, TAPIR_DIALOG_AUTOCLOSE='true')

command = [args.archicadPath]
if args.projectFilePath:
    command.append (args.projectFilePath)

subprocess.Popen (command, env=env)
