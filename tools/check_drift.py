"""Runs the generated-file checks that no workflow runs, and reports them together.

Two generators compare what they would write with what is committed, and exit 1 when the two differ:

  * tools/generate_gh_docs.py --check   docs/grasshopper-components/components.js
  * python-client/generate.py --check   python-client/tapir_archicad/commands.py

Nothing calls either of them. The generate_docs job of archicad_addon_build_check.yml runs
tools/generate_addon_docs.py on every pull request, but it only writes the files: it never compares
them with what the branch has, so a stale generated file passes the check. That is how the
Grasshopper reference page came to list four components whose .cs files no longer exist and to miss
one that does.

The examples and marketplace checkers are not run here: that same generate_docs job already runs
check_examples.py, check_marketplace.py and generate_catalogue.py --self-test.

Reported as a warning, never failed on: tools/check_tool_settings.py. CLAUDE.md says a field enters
a tool settings table only when its dialog label is found verbatim in Graphisoft's documentation, so
the tables are deliberately behind the schema, and the checker's "every schema key must be in the
table" rule is stricter than that policy. Whoever settles which side is right moves this step to
error in one line.

Usage:
  python tools/check_drift.py            check every step
  python tools/check_drift.py --write    regenerate the files, so the next run is clean
  python tools/check_drift.py --strict   fail on the warning too

Exit code: 1 when a file that must be current is not, 0 otherwise. On GitHub Actions the findings
are also written as error and warning annotations, so they show on the pull request.
"""

import argparse
import os
import subprocess
import sys

REPO_ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))

# Findings are printed for the log and, on GitHub Actions, as annotations.
ANNOTATE = os.environ.get ('GITHUB_ACTIONS') == 'true'


class Step:
    """One generated-file check: how to check it, how to fix it, and what it writes."""

    def __init__ (self, name, command, fix = None, files = '', severity = 'error', why = ''):
        self.name = name
        self.command = command
        self.fix = fix
        self.files = files
        self.severity = severity
        self.why = why


def Run (command):
    """Runs a check in the repository root; returns (exit code, output)."""
    result = subprocess.run (
        [sys.executable] + command,
        cwd = REPO_ROOT,
        capture_output = True,
        text = True,
        encoding = 'utf-8',
        errors = 'replace')
    return result.returncode, ((result.stdout or '') + (result.stderr or ''))


def Annotate (level, title, message):
    if not ANNOTATE:
        return
    # One line only: an annotation with a newline breaks the workflow command.
    first = message.strip ().split ('\n')[0]
    print ('::%s title=%s::%s' % (level, title, first.replace ('%', '%25').replace ('\r', '%0D').replace ('\n', '%0A')))


def Main ():
    parser = argparse.ArgumentParser (description = 'Report generated files that are out of date.')
    parser.add_argument ('--write', action = 'store_true', help = 'regenerate the files instead of only reporting them')
    parser.add_argument ('--strict', action = 'store_true', help = 'fail on the warning too')
    args = parser.parse_args ()

    steps = [
        Step (
            'Grasshopper component reference',
            ['tools/generate_gh_docs.py', '--check'],
            fix = 'python3 tools/generate_gh_docs.py',
            files = 'docs/grasshopper-components/components.js',
            why = 'written by tools/generate_gh_docs.py from the component classes'),
        Step (
            'Python client',
            ['python-client/generate.py', '--check'],
            fix = 'python3 python-client/generate.py',
            files = 'python-client/tapir_archicad/commands.py',
            why = 'written by python-client/generate.py from the command reference'),
        Step (
            'Tool settings tables',
            ['tools/check_tool_settings.py'],
            severity = 'warning',
            why = 'the Add-On has settings the tables do not; see the note at the top'),
    ]

    failures = 0
    warnings = 0
    stale = 0    # files that must be current and are not
    broken = 0   # checks that did not run at all
    for step in steps:
        command = list (step.command)
        if args.write and step.fix:
            command = [part for part in command if part != '--check']

        code, output = Run (command)

        if code == 0:
            print ('ok      %s' % step.name)
            continue

        lines = [line for line in output.strip ().split ('\n') if line.strip ()]
        if code > 1:
            # Not a finding: the check itself broke, which --write cannot fix.
            print ('FAILED  %s: %s exited %d' % (step.name, ' '.join (command), code))
            for line in lines[-6:]:
                print ('        ' + line)
            Annotate ('error', step.name, 'The check failed to run: ' + (lines[-1] if lines else ''))
            failures += 1
            broken += 1
            continue

        if args.write and step.fix:
            print ('wrote   %s (%s)' % (step.name, step.files))
            continue

        print ('DRIFT   %s%s' % (step.name, ' (%s)' % step.why if step.why else ''))
        if step.files:
            print ('        %s' % step.files)
        for line in lines[:8]:
            print ('        ' + line)
        if len (lines) > 8:
            print ('        ... and %d more' % (len (lines) - 8))

        if step.severity == 'error':
            failures += 1
            stale += 1
            Annotate ('error', step.name, 'Out of date: %s. Fix with: %s' % (step.files, step.fix))
        else:
            warnings += 1
            Annotate ('warning', step.name, '%d finding(s); first: %s' % (len (lines), lines[0] if lines else ''))

    print ()
    if broken:
        print ('%d check(s) did not run, see above; they have to work before --write can help.' % broken)
    if stale:
        print ('%d file(s) that must be current are not. Run: python tools/check_drift.py --write' % stale)
    if failures:
        pass
    elif warnings:
        print ('Nothing that must be current is out of date; %d warning(s) above.' % warnings)
    else:
        print ('Everything that must be current is up to date.')

    return 1 if failures or (args.strict and warnings) else 0


if __name__ == '__main__':
    sys.exit (Main ())
