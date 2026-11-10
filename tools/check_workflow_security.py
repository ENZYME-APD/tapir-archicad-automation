"""Checks the workflows against the mechanical rules of AUTOMATION_SECURITY.md.

    python tools/check_workflow_security.py            # check; exit 1 on a new violation
    python tools/check_workflow_security.py --baseline # print the current violations as the baseline

Rules (numbers as in AUTOMATION_SECURITY.md):
  2  Text from outside (issue, comment, branch name, commit message ...) never goes into a
     shell script or script step through ${{ }}; it reaches it through env:.
  3  A workflow that a stranger can trigger checks out only the base or the workflow's own
     commit, never a pull request's code.
  4  Every job declares its permissions (job or workflow level).
  5  Third-party actions (not actions/* and not local) are pinned to a full commit SHA.
  6  claude_auto_merge.yml never merges, without a person, the files that steer the bots.

Rules 4 and 5 started with violations; tools/workflow_security_baseline.json lists them. A
violation not in the baseline fails, and so does a baseline entry that no longer applies (delete
it: the list only gets shorter).
"""
import argparse
import json
import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
WORKFLOWS = ROOT / '.github' / 'workflows'
BASELINE = Path(__file__).resolve().parent / 'workflow_security_baseline.json'

# Triggers a stranger can cause, with the repository's secrets and token in reach.
OPEN_TRIGGERS = {'pull_request_target', 'issue_comment', 'issues', 'pull_request_review_comment',
                 'workflow_run', 'discussion', 'discussion_comment'}

UNTRUSTED = re.compile(
    r'github\.head_ref'
    r'|github\.event\.(?:issue\.(?:title|body)|comment\.body|review\.body|discussion\.(?:title|body)'
    r'|pull_request\.(?:title|body|head\.(?:ref|label)|head\.repo\.\w+)'
    r'|workflow_run\.(?:head_branch|display_title|head_commit\.[\w.]+|head_repository\.\w+)'
    r'|head_commit\.[\w.]+|commits[\w.\[\]*]*|pages[\w.\[\]*]*)')

SAFE_CHECKOUT_REF = re.compile(
    r'^(?:main|\$\{\{ *(?:github\.workflow_sha|github\.sha|github\.event\.repository\.default_branch'
    r'|steps\.\w+\.outputs\.base_sha) *\}\})$')

SHA_PIN = re.compile(r'@[0-9a-f]{40}$')

# What steers the bots, so changing it without a person is changing the rules.
STEERING_FILES = ['CLAUDE.md', 'sub/CLAUDE.md', 'AGENTS.md', 'sub/SKILL.md', '.claude/settings.json',
                  '.claude/skills/x/SKILL.md', 'AUTOMATION_SECURITY.md']


def triggers(workflow):
    on = workflow.get(True, workflow.get('on'))  # YAML reads a bare `on` as True
    if isinstance(on, dict):
        return set(on)
    return {on} if isinstance(on, str) else set(on or [])


def load(path):
    return yaml.safe_load(path.read_text())


def check_workflow(path, workflow):
    """Yield (rule, key, message) for every violation in one workflow."""
    name = path.name
    open_trigger = bool(triggers(workflow) & OPEN_TRIGGERS)
    for job_name, job in (workflow.get('jobs') or {}).items():
        where = f'{name}:{job_name}'
        if 'permissions' not in job and 'permissions' not in workflow:
            yield 4, f'4|{where}', f'{where}: no permissions block (the token gets the repository default)'
        for index, step in enumerate(job.get('steps') or [], 1):
            step_where = f'{where} step {index}'
            for field in ('run',) + (('script',) if 'github-script' in str(step.get('uses', '')) else ()):
                text = step.get('run') if field == 'run' else (step.get('with') or {}).get('script')
                for expression in re.findall(r'\$\{\{(.*?)\}\}', text or '', re.S):
                    found = UNTRUSTED.search(expression)
                    if found:
                        yield 2, f'2|{step_where}|{found.group(0)}', (
                            f'{step_where}: {found.group(0)} inside {field}: (pass it through env:)')
            uses = str(step.get('uses', ''))
            if uses.startswith('actions/checkout') and open_trigger:
                with_ = step.get('with') or {}
                ref = str(with_.get('ref', 'main')).strip()
                if with_.get('repository') or not SAFE_CHECKOUT_REF.match(ref):
                    yield 3, f'3|{step_where}', (
                        f'{step_where}: checks out {with_.get("repository") or ref!r} in a workflow a stranger can trigger')
            if uses and not uses.startswith(('./', 'actions/', 'docker://')) and not SHA_PIN.search(uses):
                yield 5, f'5|{name}|{uses}', f'{name}: {uses} is not pinned to a commit SHA'


def check_protected_paths(workflows_dir=WORKFLOWS):
    """Rule 6: the PROTECTED pattern of claude_auto_merge.yml must cover the steering files."""
    text = (workflows_dir / 'claude_auto_merge.yml').read_text()
    match = re.search(r"^\s*PROTECTED='(.*)'\s*$", text, re.M)
    if not match:
        yield 6, '6|claude_auto_merge.yml', 'claude_auto_merge.yml: the PROTECTED pattern was not found'
        return
    pattern = re.compile(match.group(1))
    for path in STEERING_FILES:
        if not pattern.search(path):
            yield 6, f'6|{path}', f'claude_auto_merge.yml: PROTECTED does not cover {path}'


def violations(workflows_dir=WORKFLOWS):
    found = []
    for path in sorted(workflows_dir.glob('*.yml')):
        found.extend(check_workflow(path, load(path)))
    if (workflows_dir / 'claude_auto_merge.yml').exists():
        found.extend(check_protected_paths(workflows_dir))
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--baseline', action='store_true', help='print the current violations as a baseline')
    args = parser.parse_args(argv)
    found = violations()
    keys = sorted({key for _, key, _ in found})
    if args.baseline:
        print(json.dumps({'allowed': keys}, indent=2))
        return 0
    allowed = set(json.loads(BASELINE.read_text())['allowed']) if BASELINE.exists() else set()
    new = [(rule, message) for rule, key, message in found if key not in allowed]
    stale = sorted(allowed - set(keys))
    for rule, message in new:
        print(f'rule {rule}: {message}')
    for key in stale:
        print(f'baseline: {key} no longer applies - delete it from {BASELINE.name}')
    if new or stale:
        return 1
    print(f'ok: {len(found)} known exception(s) in the baseline, no new violation')
    return 0


if __name__ == '__main__':
    sys.exit(main())
