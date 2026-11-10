"""Tests of check_workflow_security.py: python tools/test_check_workflow_security.py"""
import tempfile
import textwrap
import unittest
from pathlib import Path

import check_workflow_security as cws


def violations_of(yaml_text, name='w.yml'):
    with tempfile.TemporaryDirectory() as tmp:
        (Path(tmp) / name).write_text(textwrap.dedent(yaml_text))
        return cws.violations(Path(tmp))


def rules(found):
    return sorted({rule for rule, _, _ in found})


CLEAN = '''
    on: {pull_request_target: {}}
    permissions: {contents: read}
    jobs:
      j:
        runs-on: ubuntu-latest
        steps:
          - uses: actions/checkout@v4
            with: {ref: "${{ github.workflow_sha }}"}
          - run: echo "$TITLE"
            env: {TITLE: "${{ github.event.pull_request.title }}"}
          - uses: some/action@0123456789abcdef0123456789abcdef01234567
'''


class RuleTests(unittest.TestCase):
    def test_a_clean_workflow_has_no_violation(self):
        self.assertEqual(violations_of(CLEAN), [])

    def test_rule_2_untrusted_text_in_a_shell_script(self):
        for expression in ('github.event.issue.title', 'github.event.comment.body', 'github.head_ref',
                           'github.event.pull_request.head.ref', 'github.event.head_commit.message',
                           'github.event.workflow_run.head_branch', 'github.event.commits[0].message'):
            with self.subTest(expression=expression):
                found = violations_of(f'''
                    on: {{issues: {{}}}}
                    permissions: {{}}
                    jobs:
                      j:
                        steps:
                          - run: echo "${{{{ {expression} }}}}"
                ''')
                self.assertEqual(rules(found), [2])

    def test_rule_2_allows_safe_expressions_and_env(self):
        found = violations_of('''
            on: {issues: {}}
            permissions: {}
            jobs:
              j:
                steps:
                  - run: echo "${{ github.event.issue.number }} ${{ github.repository }} ${{ secrets.X }}"
                  - run: echo "$B"
                    env: {B: "${{ github.event.issue.body }}"}
        ''')
        self.assertEqual(found, [])

    def test_rule_2_also_covers_github_script(self):
        found = violations_of('''
            on: {issues: {}}
            permissions: {}
            jobs:
              j:
                steps:
                  - uses: actions/github-script@v7
                    with: {script: "core.info('${{ github.event.issue.title }}')"}
        ''')
        self.assertEqual(rules(found), [2])

    def test_rule_3_checkout_of_pull_request_code_in_an_open_workflow(self):
        for ref in ('${{ github.event.pull_request.head.sha }}', 'refs/pull/1/merge', '${{ github.head_ref }}'):
            with self.subTest(ref=ref):
                found = violations_of(f'''
                    on: {{pull_request_target: {{}}}}
                    permissions: {{}}
                    jobs:
                      j:
                        steps:
                          - uses: actions/checkout@v4
                            with: {{ref: "{ref}"}}
                ''')
                self.assertEqual(rules(found), [3])

    def test_rule_3_another_repository_and_other_triggers(self):
        found = violations_of('''
            on: {issue_comment: {}}
            permissions: {}
            jobs:
              j:
                steps:
                  - uses: actions/checkout@v4
                    with: {repository: someone/fork}
        ''')
        self.assertEqual(rules(found), [3])
        # a plain pull_request workflow (read-only token for forks) may check out its merge ref
        found = violations_of('''
            on: {pull_request: {}}
            permissions: {contents: read}
            jobs:
              j:
                steps:
                  - uses: actions/checkout@v4
                    with: {ref: "${{ github.event.pull_request.head.sha }}"}
        ''')
        self.assertEqual(found, [])

    def test_rule_4_every_job_declares_permissions(self):
        found = violations_of('''
            on: {push: {}}
            jobs:
              a:
                permissions: {contents: read}
                steps: [{run: echo}]
              b:
                steps: [{run: echo}]
        ''')
        self.assertEqual([key for _, key, _ in found], ['4|w.yml:b'])

    def test_rule_5_third_party_actions_are_pinned_to_a_sha(self):
        found = violations_of('''
            on: {push: {}}
            permissions: {}
            jobs:
              j:
                steps:
                  - uses: actions/checkout@v4
                  - uses: ./.github/actions/x
                  - uses: some/action@v1
                  - uses: other/action@0123456789abcdef0123456789abcdef01234567
        ''')
        self.assertEqual([key for _, key, _ in found], ['5|w.yml|some/action@v1'])


class ProtectedPathTests(unittest.TestCase):
    def merge_workflow(self, pattern):
        return f"""
            on: {{push: {{}}}}
            permissions: {{}}
            jobs:
              j:
                steps:
                  - run: |
                      PROTECTED='{pattern}'
        """

    def test_the_real_pattern_covers_everything_that_steers_the_bots(self):
        self.assertEqual([v for v in cws.check_protected_paths() ], [])

    def test_a_pattern_without_the_steering_files_is_a_violation(self):
        found = violations_of(self.merge_workflow(r'^\.github/|^tools/'), name='claude_auto_merge.yml')
        self.assertEqual(rules(found), [6])
        self.assertIn('6|CLAUDE.md', [key for _, key, _ in found])

    def test_a_missing_pattern_is_a_violation(self):
        found = violations_of('on: {push: {}}\npermissions: {}\njobs: {j: {steps: [{run: echo}]}}\n',
                              name='claude_auto_merge.yml')
        self.assertEqual(rules(found), [6])


class RealTreeTests(unittest.TestCase):
    def test_the_repository_passes_against_its_baseline(self):
        self.assertEqual(cws.main([]), 0)


if __name__ == '__main__':
    unittest.main()
