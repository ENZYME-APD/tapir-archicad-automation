# Tapir Additional JSON Commands Archicad Add-On

This is the source code of an Archicad Add-On that registers several additional JSON commands in Archicad. The code is based on the amazing work of Tibor Lorantfy on [archicad-additional-json-commands](https://github.com/tlorantfy/archicad-additional-json-commands).

## Developer documentation

[You can find the developer documentation here.](https://github.com/ENZYME-APD/tapir-archicad-automation/wiki/Archicad-Add-On-Development)

## Examples, workflows and tests

- [Examples/](Examples/): one short script per command, in one folder per command group
  (`Examples/<GroupFolder>/<CommandName>.py`). The [command reference](https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon)
  shows each of them in the command's *Example* popdown.
- [Workflows/](Workflows/): scripts that combine several commands into a task or a tool.
- [Test/Scripts/](Test/Scripts/): regression and behaviour tests, run with `Test/test_examples.py`,
  which also smoke-tests every example.
