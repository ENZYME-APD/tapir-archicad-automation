#!/usr/bin/env python3
"""Preview or run a finite, ordered queue of Tapir schema merge actions.

Requires authenticated GitHub CLI 2.87.0+ for workflow dispatch run URLs.
Preview is read-only. --merge explicitly dispatches the existing maintainer
workflow for the listed PRs, one at a time.
"""

from __future__ import annotations

import argparse
from collections import deque
from collections.abc import Iterable, Iterator, Sequence
from dataclasses import dataclass
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from typing import Any, Protocol, cast


DEFAULT_REPO = "ENZYME-APD/tapir-archicad-automation"
WORKFLOW = "schema_refinement.yml"
RUN_IDENTITY_ATTEMPTS = 6
RUN_IDENTITY_INTERVAL = 5
PREPARATION_MAIN_MOVED = "Main moved while fetching the candidate. Start a new workflow run."
FINALIZATION_STALE_MESSAGES = {
    "The base or PR head changed during validation. Start a new workflow run.",
    "Main changed before merging. Start a new workflow run.",
}

JsonObject = dict[str, Any]
ControllerError = tuple[str, str]


class ItemFailed(RuntimeError):
    """One PR cannot proceed; other queue entries may still run."""


class QueueBlocked(ItemFailed):
    """An action may still merge; do not dispatch another until reconciled."""


class GitHubClient(Protocol):
    def run(self, *args: str, live: bool = False, timeout: float = 60) -> str: ...

    def json(self, *args: str) -> JsonObject: ...


class GitHub:
    def run(self, *args: str, live: bool = False, timeout: float = 60) -> str:
        result = subprocess.run(
            ["gh", *args], text=True, capture_output=not live, timeout=timeout
        )
        if result.returncode:
            detail = (result.stderr or result.stdout or "").strip()
            raise ItemFailed(detail or f"gh {args[0]} failed (exit {result.returncode}).")
        return result.stdout or ""

    def json(self, *args: str) -> JsonObject:
        try:
            return cast(JsonObject, json.loads(self.run(*args)))
        except json.JSONDecodeError as error:
            raise ItemFailed("GitHub CLI returned invalid JSON.") from error


@dataclass(frozen=True)
class Proposal:
    number: int
    head: str
    merged: bool


@dataclass(frozen=True)
class WorkflowRun:
    id: int
    url: str


class HeadChanged(ItemFailed):
    def __init__(self, expected: Proposal, current: Proposal) -> None:
        self.current = current
        super().__init__(
            f"PR #{expected.number} changed from {expected.head[:12]} to {current.head[:12]}."
        )


class MainAdvanced(ItemFailed):
    def __init__(self, current: Proposal, reason: str) -> None:
        self.current = current
        super().__init__(f"PR #{current.number}: {reason}")


def fetch_proposal(gh: GitHubClient, repo: str, number: int) -> Proposal:
    data = gh.json("api", f"repos/{repo}/pulls/{number}")
    if data.get("number") != number:
        raise ItemFailed(f"Unexpected PR metadata for #{number}.")
    merged = bool(data.get("merged") or data.get("merged_at"))
    if not merged:
        if data.get("state") != "open" or data.get("base", {}).get("ref") != "main":
            raise ItemFailed(f"PR #{number} must be open and target main.")
        if not data.get("draft"):
            raise ItemFailed(f"PR #{number} must stay draft for the schema merge action.")
    head = data.get("head", {}).get("sha", "")
    if not re.fullmatch(r"[0-9a-f]{40}", head):
        raise ItemFailed(f"Invalid head SHA for PR #{number}.")
    return Proposal(number, head, merged)


def ensure_head_unchanged(expected: Proposal, current: Proposal) -> None:
    if current.head != expected.head:
        raise HeadChanged(expected, current)


def parse_dispatched_run(repo: str, number: int, output: str) -> WorkflowRun:
    pattern = rf"https://github\.com/{re.escape(repo)}/actions/runs/([1-9][0-9]*)"
    ids = set(re.findall(pattern, output, re.IGNORECASE))
    if len(ids) != 1:
        raise QueueBlocked(
            f"PR #{number}: workflow dispatch may have succeeded, but gh did not return "
            "one identifiable run URL. Inspect Actions before trying again; no retry was sent."
        )
    run_id = int(ids.pop())
    url = f"https://github.com/{repo}/actions/runs/{run_id}"
    return WorkflowRun(run_id, url)


def dispatched_run_mismatches(
    run: JsonObject, number: int, dispatched: WorkflowRun,
) -> list[str]:
    expected = {
        "id": dispatched.id,
        "event": "workflow_dispatch",
        "path": f".github/workflows/{WORKFLOW}",
        "display_title": f"Validate and merge PR #{number}",
    }
    return [
        f"{field}: expected {value!r}, received {run.get(field)!r}"
        for field, value in expected.items() if run.get(field) != value
    ]


def verify_dispatched_run(
    gh: GitHubClient, repo: str, number: int, dispatched: WorkflowRun,
) -> None:
    """Allow dispatch metadata to settle without ever dispatching another run."""
    for attempt in range(1, RUN_IDENTITY_ATTEMPTS + 1):
        try:
            run = gh.json("api", f"repos/{repo}/actions/runs/{dispatched.id}")
        except (ItemFailed, subprocess.TimeoutExpired) as error:
            detail = f"Cannot read dispatched run metadata: {error}"
        else:
            mismatches = dispatched_run_mismatches(run, number, dispatched)
            if not mismatches:
                return
            detail = "Unexpected dispatched run identity: " + "; ".join(mismatches)
        if attempt == RUN_IDENTITY_ATTEMPTS:
            raise QueueBlocked(
                f"{detail}. Could not verify the action after {attempt} metadata reads. "
                f"Inspect {dispatched.url}; queue stopped. No additional dispatch was sent."
            ) from None
        print(
            f"#{number}: {detail}. Retrying metadata for the same action in "
            f"{RUN_IDENTITY_INTERVAL}s ({attempt}/{RUN_IDENTITY_ATTEMPTS}): {dispatched.url}",
            flush=True,
        )
        time.sleep(RUN_IDENTITY_INTERVAL)


def wait_for_checks(
    gh: GitHubClient, repo: str, expected: Proposal, interval: int, timeout: float,
) -> Proposal:
    current = fetch_proposal(gh, repo, expected.number)
    if current.merged:
        return current
    ensure_head_unchanged(expected, current)
    print(f"#{expected.number}: waiting for existing PR checks.", flush=True)
    try:
        gh.run(
            "pr", "checks", str(expected.number), "--repo", repo, "--watch",
            "--interval", str(interval), "--fail-fast", live=True, timeout=timeout,
        )
    except ItemFailed:
        # Cancelled checks can belong to an old head. Confirm the new head
        # before treating that watcher failure as a reschedule.
        current = fetch_proposal(gh, repo, expected.number)
        if current.merged:
            return current
        ensure_head_unchanged(expected, current)
        raise
    current = fetch_proposal(gh, repo, expected.number)
    if not current.merged:
        ensure_head_unchanged(expected, current)
    return current


def wait_for_run_completion(
    gh: GitHubClient, repo: str, run: WorkflowRun, interval: int, deadline: float,
) -> None:
    previous_detail = None
    while True:
        try:
            status = gh.json("api", f"repos/{repo}/actions/runs/{run.id}")
        except (ItemFailed, subprocess.TimeoutExpired) as error:
            detail = f"Cannot read action status: {error}"
        else:
            if status.get("status") == "completed":
                return
            detail = f"Action is {status.get('status', 'unknown')}"
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise QueueBlocked(f"Cannot establish that {run.url} finished before the timeout. {detail}")
        if detail != previous_detail:
            print(f"{detail}; waiting on the same action: {run.url}", flush=True)
            previous_detail = detail
        time.sleep(min(interval, remaining))


def parse_controller_errors(logs: str) -> Iterator[ControllerError]:
    for line in logs.splitlines():
        parts = line.split("\t", 2)
        if len(parts) != 3:
            continue
        job, _, timestamped = parts
        # Step names can be UNKNOWN STEP in gh logs; only controller jobs
        # supply freshness errors. Validation output cannot authorize retries.
        if job in {"prepare", "finish"}:
            message = re.sub(r"^\d{4}-\d{2}-\d{2}T\S+Z\s+", "", timestamped).strip()
            yield job, message


def preparation_stale_base_reason(errors: Sequence[ControllerError]) -> str | None:
    """Recognize the early freshness guard, before a request artifact exists."""
    if ("prepare", PREPARATION_MAIN_MOVED) in errors:
        return "main advanced while preparing the candidate; retrying against fresh main."
    return None


def download_merge_request(gh: GitHubClient, repo: str, run_id: int) -> object:
    with tempfile.TemporaryDirectory(prefix="tapir-queue-request-") as directory:
        gh.run("run", "download", str(run_id), "--repo", repo,
               "--name", "schema-request", "--dir", directory)
        return json.loads((Path(directory) / "schema-request.json").read_text(encoding="utf-8"))


def matching_request_base(request: object, repo: str, expected: Proposal) -> str | None:
    if not isinstance(request, dict):
        return None
    base = request.get("base", "")
    if (
        request.get("repo") != repo or request.get("number") != expected.number
        or request.get("head") != expected.head or request.get("merge") is not True
        or not isinstance(base, str) or not re.fullmatch(r"[0-9a-f]{40}", base)
    ):
        return None
    return base


def fetch_main_sha(gh: GitHubClient, repo: str) -> str | None:
    sha = gh.json("api", f"repos/{repo}/git/ref/heads/main").get("object", {}).get("sha", "")
    if isinstance(sha, str) and re.fullmatch(r"[0-9a-f]{40}", sha):
        return sha
    return None


def finalization_stale_base_reason(
    gh: GitHubClient, repo: str, expected: Proposal, run_id: int,
    errors: Sequence[ControllerError],
) -> str | None:
    """Confirm a finish guard reflects main advancing past the pinned base."""
    if not any(job == "finish" and message in FINALIZATION_STALE_MESSAGES
               for job, message in errors):
        return None
    # The finish message combines head/base changes. Verify the pinned
    # request and current main to establish that this is a base change.
    request = download_merge_request(gh, repo, run_id)
    base = matching_request_base(request, repo, expected)
    if base is None:
        return None
    current_main = fetch_main_sha(gh, repo)
    if current_main is not None and current_main != base:
        return f"main advanced from {base[:12]} to {current_main[:12]}; retrying fresh validation."
    return None


def stale_base_reason(
    gh: GitHubClient, repo: str, expected: Proposal, run_id: int,
) -> str | None:
    """Retry only a controller freshness rejection, never a generic failure."""
    try:
        logs = gh.run("run", "view", str(run_id), "--repo", repo, "--log-failed")
        errors = list(parse_controller_errors(logs))
        preparation_reason = preparation_stale_base_reason(errors)
        if preparation_reason is not None:
            return preparation_reason
        return finalization_stale_base_reason(gh, repo, expected, run_id, errors)
    except (ItemFailed, subprocess.TimeoutExpired, OSError, ValueError, AttributeError):
        # Missing artifacts/logs or unreadable metadata do not authorize a retry.
        return None


def dispatch_merge_workflow(gh: GitHubClient, repo: str, number: int) -> WorkflowRun:
    # The existing workflow captures fresh main and enforces its own gates.
    try:
        output = gh.run(
            "workflow", "run", WORKFLOW, "--repo", repo, "--ref", "main",
            "--raw-field", f"pr={number}", "--raw-field", "merge=true",
        )
    except (ItemFailed, subprocess.TimeoutExpired) as error:
        raise QueueBlocked(
            f"Dispatch for #{number} was not confirmed: {error}. "
            "It may have started; inspect Actions before trying again. No retry was sent."
        ) from None
    run = parse_dispatched_run(repo, number, output)
    verify_dispatched_run(gh, repo, number, run)
    return run


def watch_merge_workflow(
    gh: GitHubClient, repo: str, run: WorkflowRun, interval: int, timeout: float,
) -> ItemFailed | subprocess.TimeoutExpired | None:
    deadline = time.monotonic() + timeout
    try:
        gh.run(
            "run", "watch", str(run.id), "--repo", repo, "--interval", str(interval),
            "--exit-status", "--compact", live=True, timeout=timeout,
        )
    except KeyboardInterrupt:
        raise QueueBlocked(f"Interrupted. Action may still be running: {run.url}") from None
    except (ItemFailed, subprocess.TimeoutExpired) as error:
        # A failed watch is not proof the server action has finished. Reconcile
        # its actual status before allowing another merge to start.
        wait_for_run_completion(gh, repo, run, interval, deadline)
        return error
    return None


def check_merge_result(
    gh: GitHubClient, repo: str, expected: Proposal, run: WorkflowRun,
    watch_error: ItemFailed | subprocess.TimeoutExpired | None,
) -> None:
    try:
        result = gh.json("api", f"repos/{repo}/pulls/{expected.number}")
        if result.get("head", {}).get("sha") != expected.head:
            raise ItemFailed("The PR head changed during the action; inspect its result.")
    except (ItemFailed, subprocess.TimeoutExpired) as error:
        raise ItemFailed(f"{error} Action: {run.url}") from None
    if not (result.get("merged") or result.get("merged_at")):
        if watch_error is not None:
            reason = stale_base_reason(gh, repo, expected, run.id)
            if reason:
                raise MainAdvanced(expected, reason)
        raise ItemFailed(f"The action finished but the PR is not merged. {watch_error or ''} Action: {run.url}".strip())


def merge_proposal(
    gh: GitHubClient, repo: str, expected: Proposal, interval: int, timeout: float,
) -> None:
    run = dispatch_merge_workflow(gh, repo, expected.number)
    print(f"#{expected.number}: watching {run.url}", flush=True)
    watch_error = watch_merge_workflow(gh, repo, run, interval, timeout)
    check_merge_result(gh, repo, expected, run, watch_error)
    print(f"#{expected.number}: merge confirmed.", flush=True)


def record_item_failure(failures: dict[int, str], number: int, reason: str) -> None:
    failures[number] = reason
    print(f"#{number}: failed; removed from queue. {reason}", flush=True)


def build_queue_plan(
    gh: GitHubClient, repo: str, numbers: Iterable[int], failures: dict[int, str],
) -> list[Proposal]:
    plan: list[Proposal] = []
    for number in numbers:
        try:
            item = fetch_proposal(gh, repo, number)
        except (ItemFailed, subprocess.TimeoutExpired) as error:
            record_item_failure(failures, number, str(error))
            continue
        plan.append(item)
        status = "already merged; skip" if item.merged else f"draft at {item.head[:12]}"
        print(f"#{item.number}: {status}", flush=True)
    return plan


def process_proposal(
    gh: GitHubClient, repo: str, expected: Proposal, interval: int, timeout: float,
) -> None:
    current = wait_for_checks(gh, repo, expected, interval, timeout)
    if current.merged:
        print(f"#{expected.number}: already merged; skipping.", flush=True)
        return
    merge_proposal(gh, repo, expected, interval, timeout)


def reschedule_proposal(
    pending: deque[Proposal], reschedules: dict[int, int], failures: dict[int, str],
    change: HeadChanged | MainAdvanced, max_reschedules: int,
) -> None:
    number = change.current.number
    count = reschedules.get(number, 0) + 1
    if count > max_reschedules:
        record_item_failure(failures, number, f"Exceeded {max_reschedules} candidate-change reschedules.")
        return
    reschedules[number] = count
    pending.append(change.current)
    print(
        f"{change} Moving to the back of the queue "
        f"({count}/{max_reschedules}); checks will run again before dispatch.",
        flush=True,
    )


def report_queue_summary(failures: dict[int, str]) -> None:
    print(f"Queue complete: {len(failures)} failed item(s).", flush=True)
    for number, reason in failures.items():
        print(f"  #{number}: {reason}", flush=True)


def run_queue(
    gh: GitHubClient, repo: str, numbers: Iterable[int], *, merge: bool = False,
    interval: int = 20, timeout_minutes: int = 90, max_reschedules: int = 3,
) -> dict[int, str]:
    failures: dict[int, str] = {}
    plan = build_queue_plan(gh, repo, numbers, failures)
    if not merge:
        print("Preview only. Add --merge to run this ordered queue.", flush=True)
        return failures

    timeout = timeout_minutes * 60
    pending = deque(plan)
    reschedules: dict[int, int] = {}
    while pending:
        expected = pending.popleft()
        if expected.merged:
            continue
        try:
            process_proposal(gh, repo, expected, interval, timeout)
        except (HeadChanged, MainAdvanced) as change:
            reschedule_proposal(pending, reschedules, failures, change, max_reschedules)
        except QueueBlocked:
            raise
        except (ItemFailed, subprocess.TimeoutExpired) as error:
            record_item_failure(failures, expected.number, str(error))
    report_queue_summary(failures)
    return failures


def positive_integer(value: str) -> int:
    if not value.isdecimal() or int(value) < 1:
        raise argparse.ArgumentTypeError("Expected a positive integer.")
    return int(value)


def nonnegative_integer(value: str) -> int:
    if not value.isdecimal():
        raise argparse.ArgumentTypeError("Expected a non-negative integer.")
    return int(value)


def create_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prs", nargs="+", type=positive_integer, help="Explicit PR numbers in merge order")
    parser.add_argument("--repo", default=DEFAULT_REPO, help="GitHub owner/repository")
    parser.add_argument("--merge", action="store_true", help="Dispatch merges; default is read-only preview")
    parser.add_argument("--interval", type=positive_integer, default=20, help="CI/run polling seconds (default: 20)")
    parser.add_argument("--timeout-minutes", type=positive_integer, default=90, help="Timeout for each CI/run wait")
    parser.add_argument("--max-reschedules", type=nonnegative_integer, default=3,
                        help="Head/stale-base reschedules before removing that PR (default: 3; 0 disables)")
    return parser


def parse_arguments(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = create_argument_parser()
    args = parser.parse_args(argv)
    if len(set(args.prs)) != len(args.prs):
        parser.error("List each PR only once.")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]*/[A-Za-z0-9][A-Za-z0-9_.-]*", args.repo):
        parser.error("--repo must be owner/repository on github.com.")
    return args


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_arguments(argv)
    try:
        failures = run_queue(GitHub(), args.repo, args.prs, merge=args.merge, interval=args.interval,
                             timeout_minutes=args.timeout_minutes, max_reschedules=args.max_reschedules)
    except (ItemFailed, FileNotFoundError, subprocess.TimeoutExpired) as error:
        print(f"Queue stopped: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("Queue interrupted. Inspect any already dispatched action before restarting.", file=sys.stderr)
        return 130
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
