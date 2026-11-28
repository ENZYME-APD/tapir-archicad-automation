import contextlib
import copy
import io
import json
import subprocess
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import queue_schema_refinements as queue


def pr(number, *, merged=False, head=None, draft=True, state="open"):
    return {"number": number, "head": {"sha": head or str(number % 10) * 40},
            "merged": merged, "draft": draft, "state": state, "base": {"ref": "main"}}


class FakeGitHub:
    def __init__(self, proposals):
        self.proposals = copy.deepcopy(proposals)
        self.events = []
        self.fail_run = None
        self.fail_checks = None
        self.skip_merge = False
        self.change_after_first = False
        self.missing_url = False
        self.run_metadata_responses = {}
        self.timeout_run = False
        self.change_during_checks = False
        self.fail_dispatch = False
        self.change_after_merge = False
        self.fail_checks_once = False
        self.churn_checks = False
        self.check_revision = 0
        self.merge_during_checks = False
        self.run_status = {}
        self.timeout_finished = False
        self.status_unavailable = False
        self.churn_number = None
        self.active_watch_failure = False
        self.complete_after_status_reads = None
        self.status_reads = 0
        self.main_sha = "a" * 40
        self.run_numbers = {}
        self.run_requests = {}
        self.run_logs = {}
        self.attempts = {}
        self.stale_remaining = {}
        self.stale_during_preparation = False
        self.advance_main_on_merge = False
        self.request_override = {}
        self.missing_request = False

    def json(self, *args):
        path = args[1]
        self.events.append(("read", path))
        if "/pulls/" in path:
            return copy.deepcopy(self.proposals[int(path.rsplit("/", 1)[1])])
        if path.endswith("/git/ref/heads/main"):
            return {"object": {"sha": self.main_sha}}
        run_id = int(path.rsplit("/", 1)[1])
        responses = self.run_metadata_responses.get(run_id)
        if responses:
            response = responses.pop(0)
            if isinstance(response, Exception):
                raise response
            return response
        number = self.run_numbers.get(run_id, run_id - 1000)
        if self.status_unavailable and number in self.run_status:
            raise queue.ItemFailed("Status unavailable")
        if number in self.run_status and self.complete_after_status_reads:
            self.status_reads += 1
            if self.status_reads >= self.complete_after_status_reads:
                self.run_status[number] = "completed"
        return {"id": run_id, "event": "workflow_dispatch",
                "path": ".github/workflows/schema_refinement.yml",
                "display_title": f"Validate and merge PR #{number}",
                "status": self.run_status.get(number, "queued")}

    def run(self, *args, **kwargs):
        if args[:2] == ("pr", "checks"):
            number = int(args[2]);self.events.append(("checks", number))
            if self.change_during_checks:self.proposals[number]["head"]["sha"] = "f" * 40
            if self.churn_checks and self.churn_number in (None, number):
                self.check_revision += 1
                self.proposals[number]["head"]["sha"] = f"{self.check_revision:040x}"
            if self.merge_during_checks:self.proposals[number]["merged"] = True
            if self.fail_checks == number:
                if self.fail_checks_once:self.fail_checks = None
                raise queue.ItemFailed("CI failed")
        elif args[:2] == ("workflow", "run"):
            number = int(next(value[3:] for value in args if value.startswith("pr=")))
            self.events.append(("dispatch", number))
            self.events.append(("dispatch_head", number, self.proposals[number]["head"]["sha"]))
            if self.fail_dispatch:raise queue.ItemFailed("Connection lost")
            if self.missing_url:return ""
            attempt = self.attempts.get(number, 0) + 1;self.attempts[number] = attempt
            run_id = 1000 + number + (attempt - 1) * 10000
            self.run_numbers[run_id] = number;self.run_status.pop(number, None)
            self.run_requests[run_id] = {
                "repo": queue.DEFAULT_REPO, "number": number, "merge": True,
                "head": self.proposals[number]["head"]["sha"], "base": self.main_sha,
            }
            return f"https://github.com/{queue.DEFAULT_REPO}/actions/runs/{run_id}\n"
        elif args[:2] == ("run", "watch"):
            run_id = int(args[2]);number = self.run_numbers[run_id];self.events.append(("watch", number))
            self.run_status[number] = "in_progress"
            if self.stale_remaining.get(number, 0):
                self.stale_remaining[number] -= 1
                self.main_sha = f"{int(self.main_sha, 16)+1:040x}"
                self.run_status[number] = "completed"
                self.run_logs[run_id] = (
                    "finish\tUNKNOWN STEP\t2026-11-28T09:00:00Z "
                    "The base or PR head changed during validation. Start a new workflow run.\n"
                )
                if self.stale_during_preparation:
                    self.run_logs[run_id] = (
                        "prepare\tUNKNOWN STEP\t2026-11-28T09:00:00Z "
                        "Main moved while fetching the candidate. Start a new workflow run.\n"
                    )
                raise queue.ItemFailed("Main advanced")
            if self.fail_run == number:
                self.run_status[number] = "in_progress" if self.active_watch_failure else "completed"
                raise queue.ItemFailed("Action failed")
            if self.timeout_run:
                if self.timeout_finished:self.run_status[number] = "completed"
                raise subprocess.TimeoutExpired("gh run watch", 1)
            if not self.skip_merge:self.proposals[number]["merged"] = True
            if self.advance_main_on_merge:self.main_sha = f"{int(self.main_sha, 16)+1:040x}"
            if self.change_after_merge:self.proposals[number]["head"]["sha"] = "f" * 40
            if self.change_after_first:
                self.proposals[2]["head"]["sha"] = "f" * 40
            self.run_status[number] = "completed"
        elif args[:2] == ("run", "view"):
            return self.run_logs.get(int(args[2]), "")
        elif args[:2] == ("run", "download"):
            if self.missing_request:raise queue.ItemFailed("Artifact missing")
            request = dict(self.run_requests[int(args[2])]);request.update(self.request_override)
            directory = Path(args[args.index("--dir")+1])
            (directory/"schema-request.json").write_text(json.dumps(request), encoding="utf-8")
        else:raise AssertionError(args)
        return ""


class QueueTests(unittest.TestCase):
    def run_queue(self, gh, numbers=(1, 2), *, merge=True, max_reschedules=3):
        with contextlib.redirect_stdout(io.StringIO()):
            return queue.run_queue(gh, queue.DEFAULT_REPO, numbers, merge=merge,
                                   max_reschedules=max_reschedules)

    def test_preview_never_dispatches_or_waits(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)})
        self.run_queue(gh, merge=False)
        self.assertTrue(all(event[0] == "read" for event in gh.events))

    def test_next_dispatch_follows_previous_watch_and_confirmed_merge(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)})
        self.run_queue(gh)
        first_watch = gh.events.index(("watch", 1))
        second_dispatch = gh.events.index(("dispatch", 2))
        self.assertLess(first_watch, second_dispatch)
        self.assertIn(("read", f"repos/{queue.DEFAULT_REPO}/pulls/1"), gh.events[first_watch+1:second_dispatch])
        self.assertTrue(all(p["merged"] for p in gh.proposals.values()))

    def test_main_advancing_between_queued_merges_needs_no_retry(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.advance_main_on_merge = True
        self.assertFalse(self.run_queue(gh))
        self.assertEqual([e[1] for e in gh.events if e[0] == "dispatch"], [1, 2])
        self.assertNotEqual(gh.run_requests[1001]["base"], gh.run_requests[1002]["base"])

    def test_main_advancing_during_validation_requeues_for_fresh_comparison(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.stale_remaining = {1: 1}
        self.assertFalse(self.run_queue(gh))
        self.assertEqual([e[1] for e in gh.events if e[0] == "dispatch"], [1, 2, 1])
        self.assertNotEqual(gh.run_requests[1001]["base"], gh.run_requests[11001]["base"])
        self.assertEqual(gh.run_requests[1001]["head"], gh.run_requests[11001]["head"])
        self.assertTrue(all(p["merged"] for p in gh.proposals.values()))

    def test_continually_advancing_main_is_bounded_and_other_pr_can_merge(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.stale_remaining = {1: 10}
        failures = self.run_queue(gh, max_reschedules=2)
        self.assertEqual(set(failures), {1})
        self.assertEqual(gh.attempts[1], 3)
        self.assertTrue(gh.proposals[2]["merged"])

    def test_main_advancing_during_preparation_requeues_without_request_artifact(self) -> None:
        gh = FakeGitHub({1: pr(1), 2: pr(2)})
        gh.stale_remaining = {1: 1}
        gh.stale_during_preparation = True
        gh.missing_request = True
        self.assertFalse(self.run_queue(gh))
        self.assertEqual([e[1] for e in gh.events if e[0] == "dispatch"], [1, 2, 1])
        self.assertTrue(all(p["merged"] for p in gh.proposals.values()))

    def test_wrong_request_head_does_not_authorize_stale_base_retry(self):
        gh = FakeGitHub({1: pr(1)});gh.stale_remaining = {1: 1}
        gh.request_override = {"head": "f"*40}
        self.assertIn(1, self.run_queue(gh, (1,)))
        self.assertEqual(gh.attempts[1], 1)

    def test_controller_message_with_unchanged_main_does_not_authorize_retry(self):
        gh = FakeGitHub({1: pr(1)});gh.stale_remaining = {1: 1}
        gh.request_override = {"base": f"{int(gh.main_sha, 16)+1:040x}"}
        self.assertIn(1, self.run_queue(gh, (1,)))
        self.assertEqual(gh.attempts[1], 1)

    def test_missing_request_artifact_does_not_authorize_retry(self):
        gh = FakeGitHub({1: pr(1)});gh.stale_remaining = {1: 1};gh.missing_request = True
        self.assertIn(1, self.run_queue(gh, (1,)))
        self.assertEqual(gh.attempts[1], 1)

    def test_stale_text_in_validation_output_is_not_a_controller_retry(self):
        gh = FakeGitHub({1: pr(1)});gh.fail_run = 1
        gh.run_logs[1001] = (
            "validate\tCompare schemas\t2026-11-28T09:00:00Z "
            "Main changed before merging. Start a new workflow run.\n"
        )
        self.assertIn(1, self.run_queue(gh, (1,)))
        self.assertEqual(gh.attempts[1], 1)

    def test_completed_workflow_failure_is_removed_and_next_pr_merges(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.fail_run = 1
        failures = self.run_queue(gh)
        self.assertEqual(set(failures), {1})
        self.assertIn("Action failed", failures[1])
        self.assertIn(("dispatch", 2), gh.events)
        self.assertTrue(gh.proposals[2]["merged"])

    def test_ci_failure_is_removed_without_dispatching_failed_pr(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.fail_checks = 1
        failures = self.run_queue(gh)
        self.assertEqual(set(failures), {1})
        self.assertNotIn(("dispatch", 1), gh.events)
        self.assertIn(("dispatch", 2), gh.events)

    def test_successful_action_without_merge_is_reported_and_queue_continues(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.skip_merge = True
        failures = self.run_queue(gh)
        self.assertEqual(set(failures), {1, 2})
        self.assertIn("not merged", failures[1])
        self.assertIn(("dispatch", 2), gh.events)

    def test_changed_later_head_moves_behind_other_prs_and_uses_new_head(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2), 3: pr(3)});gh.change_after_first = True
        self.run_queue(gh, (1, 2, 3))
        self.assertEqual([e[1] for e in gh.events if e[0] == "dispatch"], [1, 3, 2])
        self.assertIn(("dispatch_head", 2, "f" * 40), gh.events)
        self.assertTrue(all(p["merged"] for p in gh.proposals.values()))

    def test_already_merged_pr_is_skipped_for_resume(self):
        gh = FakeGitHub({1: pr(1, merged=True, draft=False, state="closed"), 2: pr(2)})
        self.run_queue(gh)
        self.assertNotIn(("dispatch", 1), gh.events)
        self.assertIn(("dispatch", 2), gh.events)

    def test_head_change_during_ci_wait_rechecks_new_head_before_dispatch(self):
        gh = FakeGitHub({1: pr(1)});gh.change_during_checks = True
        self.run_queue(gh, (1,))
        self.assertEqual(gh.events.count(("checks", 1)), 2)
        self.assertEqual(gh.events.count(("dispatch", 1)), 1)
        self.assertIn(("dispatch_head", 1, "f" * 40), gh.events)

    def test_cancelled_old_ci_with_confirmed_new_head_is_rescheduled(self):
        gh = FakeGitHub({1: pr(1)});gh.change_during_checks = True
        gh.fail_checks = 1;gh.fail_checks_once = True
        self.run_queue(gh, (1,))
        self.assertEqual(gh.events.count(("checks", 1)), 2)
        self.assertEqual(gh.events.count(("dispatch", 1)), 1)

    def test_continuous_head_changes_hit_finite_reschedule_limit(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.churn_checks = True;gh.churn_number = 1
        failures = self.run_queue(gh, max_reschedules=2)
        self.assertEqual(set(failures), {1})
        self.assertIn("Exceeded 2", failures[1])
        self.assertEqual(gh.events.count(("checks", 1)), 3)
        self.assertNotIn(("dispatch", 1), gh.events)
        self.assertTrue(gh.proposals[2]["merged"])

    def test_zero_limit_keeps_strict_head_policy(self):
        gh = FakeGitHub({1: pr(1)});gh.change_during_checks = True
        failures = self.run_queue(gh, (1,), max_reschedules=0)
        self.assertIn("Exceeded 0", failures[1])
        self.assertNotIn(("dispatch", 1), gh.events)

    def test_external_merge_during_wait_is_skipped_even_if_head_changed(self):
        gh = FakeGitHub({1: pr(1)});gh.change_during_checks = True
        gh.merge_during_checks = True
        self.run_queue(gh, (1,))
        self.assertNotIn(("dispatch", 1), gh.events)

    def test_dispatch_error_is_not_retried(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.fail_dispatch = True
        with self.assertRaisesRegex(queue.QueueBlocked, "may have started"):
            self.run_queue(gh)
        self.assertEqual([e for e in gh.events if e[0] == "dispatch"], [("dispatch", 1)])

    def test_head_changed_after_completed_action_is_per_item_failure(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.change_after_merge = True
        failures = self.run_queue(gh)
        self.assertIn("changed during the action", failures[1])
        self.assertIn(("dispatch", 2), gh.events)

    def test_closed_unmerged_pr_is_removed_while_valid_pr_can_merge(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2, state="closed")})
        failures = self.run_queue(gh)
        self.assertEqual(set(failures), {2})
        self.assertIn(("dispatch", 1), gh.events)
        self.assertNotIn(("dispatch", 2), gh.events)

    def test_missing_run_url_is_not_guessed_or_retried(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.missing_url = True
        with self.assertRaisesRegex(queue.QueueBlocked, "dispatch may have succeeded"):
            self.run_queue(gh)
        self.assertEqual([e for e in gh.events if e[0] == "dispatch"], [("dispatch", 1)])

    def test_initial_run_metadata_settles_without_another_dispatch(self) -> None:
        initial_responses = [
            {},
            {"id": 1001, "event": "workflow_dispatch",
             "path": ".github/workflows/schema_refinement.yml",
             "display_title": "Validate and Merge Schema Refinement"},
        ]
        for initial in initial_responses:
            with self.subTest(initial=initial):
                gh = FakeGitHub({1: pr(1), 2: pr(2)})
                gh.run_metadata_responses[1001] = [initial]
                with patch.object(queue.time, "sleep") as sleep:
                    self.assertFalse(self.run_queue(gh))
                sleep.assert_called_once_with(queue.RUN_IDENTITY_INTERVAL)
                self.assertEqual([e[1] for e in gh.events if e[0] == "dispatch"], [1, 2])
                metadata_reads = [i for i, e in enumerate(gh.events)
                                  if e == ("read", f"repos/{queue.DEFAULT_REPO}/actions/runs/1001")]
                self.assertEqual(len(metadata_reads), 2)
                self.assertLess(metadata_reads[-1], gh.events.index(("watch", 1)))
                self.assertLess(gh.events.index(("watch", 1)), gh.events.index(("dispatch", 2)))

    def test_transient_run_metadata_errors_retry_the_same_run(self) -> None:
        gh = FakeGitHub({1: pr(1)})
        gh.run_metadata_responses[1001] = [
            queue.ItemFailed("HTTP 404"), subprocess.TimeoutExpired("gh api", 60),
        ]
        with patch.object(queue.time, "sleep") as sleep:
            self.assertFalse(self.run_queue(gh, (1,)))
        self.assertEqual(sleep.call_count, 2)
        self.assertEqual(gh.events.count(("read", f"repos/{queue.DEFAULT_REPO}/actions/runs/1001")), 3)
        self.assertEqual(gh.events.count(("dispatch", 1)), 1)
        self.assertEqual(gh.events.count(("watch", 1)), 1)

    def test_wrong_run_identity_blocks_with_mismatch_details_after_bounded_reads(self) -> None:
        expected = {"id": 1001, "event": "workflow_dispatch",
                    "path": ".github/workflows/schema_refinement.yml",
                    "display_title": "Validate and merge PR #1"}
        wrong_values = {"id": 9999, "event": "push", "path": ".github/workflows/other.yml",
                        "display_title": "Validate and merge PR #2"}
        for field, wrong in wrong_values.items():
            with self.subTest(field=field):
                gh = FakeGitHub({1: pr(1), 2: pr(2)})
                metadata = {**expected, field: wrong}
                gh.run_metadata_responses[1001] = [metadata] * queue.RUN_IDENTITY_ATTEMPTS
                with patch.object(queue.time, "sleep") as sleep, \
                        self.assertRaises(queue.QueueBlocked) as error:
                    self.run_queue(gh)
                self.assertIn(f"{field}: expected {expected[field]!r}, received {wrong!r}", str(error.exception))
                self.assertIn("actions/runs/1001", str(error.exception))
                self.assertEqual(sleep.call_count, queue.RUN_IDENTITY_ATTEMPTS - 1)
                self.assertEqual(gh.events.count(("read", f"repos/{queue.DEFAULT_REPO}/actions/runs/1001")),
                                 queue.RUN_IDENTITY_ATTEMPTS)
                self.assertEqual([e for e in gh.events if e[0] == "dispatch"], [("dispatch", 1)])
                self.assertFalse(any(e[0] == "watch" for e in gh.events))

    def test_unavailable_run_metadata_blocks_after_bounded_reads_without_redispatch(self) -> None:
        gh = FakeGitHub({1: pr(1), 2: pr(2)})
        gh.run_metadata_responses[1001] = [queue.ItemFailed("HTTP 404")] * queue.RUN_IDENTITY_ATTEMPTS
        with patch.object(queue.time, "sleep") as sleep, \
                self.assertRaisesRegex(queue.QueueBlocked, "HTTP 404"):
            self.run_queue(gh)
        self.assertEqual(sleep.call_count, queue.RUN_IDENTITY_ATTEMPTS - 1)
        self.assertEqual(gh.events.count(("read", f"repos/{queue.DEFAULT_REPO}/actions/runs/1001")),
                         queue.RUN_IDENTITY_ATTEMPTS)
        self.assertEqual([e for e in gh.events if e[0] == "dispatch"], [("dispatch", 1)])
        self.assertFalse(any(e[0] == "watch" for e in gh.events))

    def test_watch_timeout_blocks_if_server_action_is_still_active(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.timeout_run = True
        with patch.object(queue.time, "monotonic", side_effect=[0, 10000]), \
                self.assertRaisesRegex(queue.QueueBlocked, "actions/runs/1001"):
            self.run_queue(gh)
        self.assertNotIn(("dispatch", 2), gh.events)

    def test_watch_timeout_with_completed_server_action_allows_next_pr(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.timeout_run = True;gh.timeout_finished = True
        failures = self.run_queue(gh)
        self.assertEqual(set(failures), {1, 2})
        self.assertIn(("dispatch", 2), gh.events)

    def test_unavailable_action_status_blocks_later_dispatch(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.fail_run = 1;gh.status_unavailable = True
        with patch.object(queue.time, "monotonic", side_effect=[0, 10000]), \
                self.assertRaisesRegex(queue.QueueBlocked, "Cannot establish"):
            self.run_queue(gh)
        self.assertNotIn(("dispatch", 2), gh.events)

    def test_watch_error_waits_for_same_run_to_finish_before_continuing(self):
        gh = FakeGitHub({1: pr(1), 2: pr(2)});gh.fail_run = 1
        gh.active_watch_failure = True;gh.complete_after_status_reads = 2
        with patch.object(queue.time, "sleep") as sleep:
            failures = self.run_queue(gh)
        self.assertEqual(set(failures), {1})
        sleep.assert_called_once()
        self.assertEqual(gh.events.count(("dispatch", 1)), 1)
        self.assertIn(("dispatch", 2), gh.events)
        self.assertTrue(gh.proposals[2]["merged"])

    def test_status_read_failure_is_retried_without_redispatch(self):
        gh = FakeGitHub({1: pr(1)});gh.fail_run = 1
        original = gh.json
        attempts = []
        def read(*args):
            if "/actions/runs/" in args[1] and 1 in gh.run_status:
                attempts.append(1)
                if len(attempts) == 1:raise queue.ItemFailed("Temporary network error")
            return original(*args)
        gh.json = read
        with patch.object(queue.time, "sleep") as sleep:
            failures = self.run_queue(gh, (1,))
        self.assertEqual(set(failures), {1})
        self.assertEqual(len(attempts), 2)
        sleep.assert_called_once()
        self.assertEqual(gh.events.count(("dispatch", 1)), 1)

    def test_main_returns_failure_status_if_any_item_failed(self):
        with patch.object(queue, "run_queue", return_value={1: "CI failed"}):
            self.assertEqual(queue.main(["1", "--merge"]), 1)

    def test_main_returns_success_after_all_items_succeed(self):
        with patch.object(queue, "run_queue", return_value={}):
            self.assertEqual(queue.main(["1", "--merge"]), 0)

    def test_duplicate_pr_arguments_are_rejected_before_network(self):
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
            queue.main(["1", "1", "--merge"])
        self.assertEqual(error.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
