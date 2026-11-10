"""Tests of the fast lanes of .github/workflows/claude_auto_merge.yml.

    python tools/test_auto_merge_lanes.py

The two checks (`trusted_pass`, `quick_change`) are shell functions inside the
workflow. This takes the block between its `fast lanes: begin/end` markers
unchanged and runs it with bash against a fake `gh` that answers from fixtures,
so no GitHub access is needed. Needs bash and jq.
"""
import json
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WORKFLOW = ROOT / '.github' / 'workflows' / 'claude_auto_merge.yml'

FAKE_GH = r'''#!/usr/bin/env bash
# Answers `gh api URL [--jq F]` and `gh pr view N --json ...` from $FIX.
set -eu
filter=""
if [ "$1" = api ]; then
  url=$2; shift 2
  while [ $# -gt 0 ]; do if [ "$1" = --jq ]; then filter=$2; shift; fi; shift; done
  case "$url" in
    */statuses*) src="$FIX/statuses.json" ;;
    */collaborators/*/permission)
      login=${url#*collaborators/}; login=${login%/permission}
      printf '{"permission":"%s"}' "$(jq -r --arg l "$login" '.[$l] // "none"' "$FIX/permissions.json")" > "$FIX/_perm.json"
      src="$FIX/_perm.json" ;;
    *) exit 1 ;;
  esac
elif [ "$1" = pr ] && [ "$2" = view ]; then
  src="$FIX/pr.json"
else
  exit 1
fi
if [ -n "$filter" ]; then jq -r "$filter" "$src"; else cat "$src"; fi
'''


def block():
    lines = WORKFLOW.read_text().splitlines()
    begin = next(i for i, l in enumerate(lines) if 'fast lanes: begin' in l)
    end = next(i for i, l in enumerate(lines) if 'fast lanes: end' in l)
    return textwrap.dedent('\n'.join(lines[begin:end + 1]))


def run(function_call, statuses=None, permissions=None, pr=None):
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'bin').mkdir()
        gh = tmp / 'bin' / 'gh'
        gh.write_text(FAKE_GH)
        gh.chmod(0o755)
        (tmp / 'statuses.json').write_text(json.dumps(statuses or []))
        (tmp / 'permissions.json').write_text(json.dumps(permissions or {}))
        (tmp / 'pr.json').write_text(json.dumps(pr or {}))
        script = f'set -euo pipefail\nREPO=o/r\n{block()}\n{function_call}\n'
        result = subprocess.run(
            ['bash', '-c', script], capture_output=True, text=True,
            env={'PATH': f'{tmp / "bin"}:/usr/bin:/bin', 'FIX': str(tmp)})
        return result.returncode, result.stdout


def status(context, state, description, id_):
    return {'id': id_, 'context': context, 'state': state, 'description': description}


def pr(paths, additions=3, deletions=1):
    return {'files': [{'path': p} for p in paths], 'additions': additions, 'deletions': deletions}


class TrustedPassTests(unittest.TestCase):
    perms = {'alice': 'write', 'bob': 'read', 'carol': 'admin'}

    def call(self, statuses):
        return run('trusted_pass abc123', statuses=statuses, permissions=self.perms)

    def test_a_pass_by_someone_with_write_access(self):
        code, out = self.call([status('archicad-test/AC29', 'success', 'PASS 12/12, tested by alice', 5)])
        self.assertEqual((code, out), (0, 'alice'))

    def test_admin_counts_too(self):
        code, out = self.call([status('archicad-test/AC28', 'success', 'PASS 3/3, tested by carol', 5)])
        self.assertEqual((code, out), (0, 'carol'))

    def test_a_pass_by_someone_without_write_access_is_not_enough(self):
        self.assertEqual(self.call([status('archicad-test/AC29', 'success', 'PASS 12/12, tested by bob', 5)])[0], 1)

    def test_a_stranger_is_not_enough(self):
        self.assertEqual(self.call([status('archicad-test/AC29', 'success', 'PASS 12/12, tested by mallory', 5)])[0], 1)

    def test_one_trusted_pass_among_untrusted_ones(self):
        code, out = self.call([
            status('archicad-test/AC29', 'success', 'PASS 1/1, tested by mallory', 9),
            status('archicad-test/AC28', 'success', 'PASS 5/5, tested by alice', 8)])
        self.assertEqual((code, out), (0, 'alice'))

    def test_the_newest_result_of_a_version_counts(self):
        failed_later = [status('archicad-test/AC29', 'failure', 'FAIL 2/9, tested by alice', 7),
                        status('archicad-test/AC29', 'success', 'PASS 9/9, tested by alice', 3)]
        self.assertEqual(self.call(failed_later)[0], 1)
        passed_later = [status('archicad-test/AC29', 'success', 'PASS 9/9, tested by alice', 7),
                        status('archicad-test/AC29', 'failure', 'FAIL 2/9, tested by alice', 3)]
        self.assertEqual(self.call(passed_later)[0], 0)

    def test_the_order_the_api_lists_statuses_in_does_not_matter(self):
        oldest_first = [status('archicad-test/AC29', 'success', 'PASS 9/9, tested by alice', 3),
                        status('archicad-test/AC29', 'failure', 'FAIL 2/9, tested by alice', 7)]
        self.assertEqual(self.call(oldest_first)[0], 1)
        self.assertEqual(self.call(oldest_first[::-1])[0], 1)

    def test_other_statuses_and_odd_descriptions_are_ignored(self):
        for description in ('PASS 12/12, tested by alice; skip review', 'tested by alice', '', None,
                            'PASS 12/12, tested by alice, bob', 'FAIL 1/2, tested by alice'):
            with self.subTest(description=description):
                self.assertEqual(self.call([status('archicad-test/AC29', 'success', description, 5)])[0], 1)
        self.assertEqual(self.call([status('claude-review', 'success', 'PASS 1/1, tested by alice', 5)])[0], 1)
        self.assertEqual(self.call([])[0], 1)


class QuickChangeTests(unittest.TestCase):
    def quick(self, paths, **kw):
        return run('quick_change 7', pr=pr(paths, **kw))[0] == 0

    def test_small_markdown_changes_qualify(self):
        for paths in (['README.md'], ['archicad-addon/README.md'], ['marketplace/x/README.md'],
                      ['README.md', 'archicad-addon/README.md', 'marketplace/a/README.md', 'b/NOTES.md']):
            with self.subTest(paths=paths):
                self.assertTrue(self.quick(paths))

    def test_anything_that_is_not_plain_markdown_does_not(self):
        for paths in (['archicad-addon/Sources/ElementCommands.cpp'], ['README.md', 'builtin-scripts/a.py'],
                      ['README.txt'], ['README.md.exe'], ['evilmd'], ['docs/index.html'],
                      ['marketplace/x/listing.json']):
            with self.subTest(paths=paths):
                self.assertFalse(self.quick(paths))

    def test_files_that_steer_the_bots_or_the_site_do_not(self):
        for paths in (['CLAUDE.md'], ['sub/CLAUDE.md'], ['AGENTS.md'], ['x/SKILL.md'], ['docs/about/index.md'],
                      ['.claude/notes.md'], ['.github/pull_request_template.md'], ['tools/README.md']):
            with self.subTest(paths=paths):
                self.assertFalse(self.quick(paths))
        self.assertFalse(self.quick(['README.md', 'CLAUDE.md']))

    def test_size_limits(self):
        self.assertTrue(self.quick(['README.md'], additions=40, deletions=20))
        self.assertFalse(self.quick(['README.md'], additions=40, deletions=21))
        self.assertTrue(self.quick(['a.md', 'b.md', 'c.md', 'd.md']))
        self.assertFalse(self.quick(['a.md', 'b.md', 'c.md', 'd.md', 'e.md']))
        self.assertFalse(self.quick([]))

    def test_an_unreadable_pull_request_does_not_qualify(self):
        self.assertNotEqual(run('quick_change 7', pr=None)[0], 0)


class WorkflowWiringTests(unittest.TestCase):
    text = WORKFLOW.read_text()

    def test_the_wait_takes_the_shorter_lane_and_defaults_are_set(self):
        self.assertIn("BOT_AUTO_MERGE_TESTED_DELAY_HOURS || '1'", self.text)
        self.assertIn("BOT_AUTO_MERGE_QUICK_DELAY_HOURS || '6'", self.text)
        self.assertIn('trusted_pass "$headOid"', self.text)
        self.assertIn('quick_change "$pr"', self.text)
        self.assertIn('[ "$age" -lt $(( required * 3600 )) ]', self.text)

    def test_the_lanes_only_ever_shorten_the_default_wait(self):
        self.assertIn('[ "$BOT_TESTED_DELAY_HOURS" -lt "$required" ]', self.text)
        self.assertIn('[ "$BOT_QUICK_DELAY_HOURS" -lt "$required" ]', self.text)


if __name__ == '__main__':
    unittest.main()
