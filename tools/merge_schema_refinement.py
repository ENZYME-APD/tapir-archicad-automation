"""Handle both /check-schema and /merge-schema comments on GitHub PRs.

prepare authorizes the command and records the main and PR commits. After the
workflow runs check_schema_refinement.py, finish reports the result or merges
the checked files. Merging requires write access and unchanged commits.
"""

import argparse
import base64
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from check_schema_refinement import git


MARKER = "<!-- tapir-schema-refinement -->"
COMMANDS = {"/check-schema", "/merge-schema"}
WORKFLOWS = ("archicad_addon_build_check.yml", "grasshopper_plugin_build_check.yml", "installer_build_check.yml")
BUILD_NAMES = ("generate_docs", "build_win", "build_mac", "build_installer", "build")


def api(repo, path, method="GET", body=None):
    command = ["gh", "api", f"repos/{repo}/{path}", "--method", method]
    data = None
    if body is not None:
        command += ["--input", "-"]
        data = json.dumps(body)
    result = subprocess.run(command, input=data, capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise ValueError(result.stderr.strip())
    return json.loads(result.stdout) if result.stdout else None


def pages(repo, path, key=None):
    page = 1
    result = []
    separator = "&" if "?" in path else "?"
    while True:
        response = api(repo, f"{path}{separator}per_page=100&page={page}")
        items = response[key] if key else response
        result.extend(items)
        if len(items) < 100:
            return result
        page += 1


def check_request(repo, event):
    """Authorize the command for this PR and check its marker."""
    if int(os.environ.get("GITHUB_RUN_ATTEMPT", "1")) != 1:
        raise ValueError("Post a new command instead of replaying an old merge authorization.")
    comment = api(repo, f"issues/comments/{event['comment']['id']}")
    command = comment["body"].strip()
    if command not in COMMANDS or comment["body"] != event["comment"]["body"]:
        raise ValueError("The command is invalid or the triggering comment was edited. Post a new command.")
    author = comment["user"]["login"]
    number = event["issue"]["number"]
    pr = api(repo, f"pulls/{number}")
    if command == "/merge-schema" or author != pr["user"]["login"]:
        permission = api(repo, f"collaborators/{author}/permission")["permission"]
        if permission not in {"write", "maintain", "admin"}:
            raise ValueError(
                "Only someone with write access can merge. "
                "To request a check, you must be the PR author or have write access."
            )
    if pr["state"] != "open" or pr["draft"] or pr["base"]["ref"] != "main":
        raise ValueError("Expected an open, ready PR targeting main.")
    if MARKER not in (pr["body"] or ""):
        raise ValueError(f"The PR description must contain {MARKER}.")
    if any(label["name"] in {"do-not-merge", "needs-maintainer"} for label in pr["labels"]):
        raise ValueError("Remove the merge hold before requesting this operation.")
    return pr, command


def check_builds(repo, head):
    """Require the existing build workflows and checks to pass on this PR commit."""
    for workflow in WORKFLOWS:
        runs = api(repo, f"actions/workflows/{workflow}/runs?head_sha={head}&per_page=1")["workflow_runs"]
        if not runs or runs[0]["status"] != "completed" or runs[0]["conclusion"] != "success":
            raise ValueError(f"{workflow} has not passed on the requested PR head.")
    checks = pages(repo, f"commits/{head}/check-runs?filter=latest", "check_runs")
    for name in BUILD_NAMES:
        if not any(check["name"].split(" ")[0] == name for check in checks):
            raise ValueError(f"Missing build check {name} on the requested PR head.")
    if any(
        check["status"] != "completed" or check["conclusion"] not in {"success", "skipped", "neutral"}
        for check in checks
    ):
        raise ValueError("A check on the PR head is pending or unsuccessful.")
    statuses = api(repo, f"commits/{head}/status")["statuses"]
    if any(status["state"] != "success" for status in statuses):
        raise ValueError("A commit status on the PR head is pending or unsuccessful.")


def check_reviews(repo, number):
    """Block merging for requested changes or unresolved human review threads."""
    latest = {}
    for review in pages(repo, f"pulls/{number}/reviews"):
        if review["state"] in {"APPROVED", "CHANGES_REQUESTED", "DISMISSED"}:
            latest[review["user"]["login"]] = review["state"]
    if "CHANGES_REQUESTED" in latest.values():
        raise ValueError("A reviewer has requested changes.")
    owner, name = repo.split("/")
    cursor = None
    while True:
        query = """query($owner:String!,$name:String!,$number:Int!,$cursor:String) {
          repository(owner:$owner,name:$name) { pullRequest(number:$number) {
            reviewThreads(first:100,after:$cursor) {
              pageInfo { hasNextPage endCursor }
              nodes { isResolved comments(first:1) { nodes { author { __typename } } } }
            }
          } }
        }"""
        payload = {"query": query, "variables": {"owner": owner, "name": name, "number": number, "cursor": cursor}}
        result = subprocess.run(
            ["gh", "api", "graphql", "--input", "-"],
            input=json.dumps(payload),
            text=True,
            capture_output=True,
            timeout=60,
        )
        data = json.loads(result.stdout) if result.returncode == 0 else {}
        if not data.get("data") or data.get("errors"):
            raise ValueError("Could not verify unresolved review threads.")
        threads = data["data"]["repository"]["pullRequest"]["reviewThreads"]
        for thread in threads["nodes"]:
            first = thread["comments"]["nodes"]
            author = first[0]["author"] if first else None
            if not thread["isResolved"] and (author is None or author["__typename"] != "Bot"):
                raise ValueError("Resolve the open human review threads before merging.")
        if not threads["pageInfo"]["hasNextPage"]:
            break
        cursor = threads["pageInfo"]["endCursor"]


def fetch_candidate(repo, number, head):
    """Fetch main and the requested PR commit, rejecting changes seen during fetch."""
    git(Path.cwd(), "fetch", "--no-tags", "origin", "refs/heads/main")
    base = git(Path.cwd(), "rev-parse", "FETCH_HEAD").decode().strip()
    git(Path.cwd(), "fetch", "--no-tags", "origin", f"refs/pull/{number}/head")
    # Read the fetched public PR ref without checking out or executing its files.
    fetched = git(Path.cwd(), "rev-parse", "FETCH_HEAD").decode().strip()
    if fetched != head:
        raise ValueError("The PR head changed. Post a new command.")
    if api(repo, "git/ref/heads/main")["object"]["sha"] != base:
        raise ValueError("Main moved while fetching the candidate. Post a new command.")
    return base


def prepare(repo, event, out):
    """Authorize either command and save the exact commits for the comparison job."""
    pr, command = check_request(repo, event)
    head, number = pr["head"]["sha"], pr["number"]
    if command == "/merge-schema":
        check_builds(repo, head)
        check_reviews(repo, number)
    base = fetch_candidate(repo, number, head)
    # This path is for schema-only source proposals, never workflow/tool changes.
    changed = git(Path.cwd(), "diff", "--name-only", f"{base}...{head}").decode().splitlines()
    allowed = re.compile(r"archicad-addon/Sources/(?:[^/]+\.(?:cpp|hpp)|RFIX/Images/CommonSchemaDefinitions\.json)")
    if not changed or any(not allowed.fullmatch(path) for path in changed):
        raise ValueError("This command accepts only command source/header and CommonSchemaDefinitions.json changes.")
    request = {"repo": repo, "number": number, "command": command, "base": base, "head": head}
    out.write_text(json.dumps(request), encoding="utf-8")
    api(
        repo,
        f"issues/{number}/comments",
        "POST",
        {
            "body": f"Validating PR head `{head}` against main `{base}`. "
            f"[Action run]({os.environ['GITHUB_SERVER_URL']}/{repo}/actions/runs/{os.environ['GITHUB_RUN_ID']})."
        },
    )


def finish(repo, event, request_path, report_path):
    """Verify the saved comparison, then report it or publish that exact merge."""
    request = json.loads(request_path.read_text(encoding="utf-8"))
    report = json.loads(report_path.read_text(encoding="utf-8"))
    pr, command = check_request(repo, event)
    number = pr["number"]
    if request["repo"] != repo or request["number"] != number or command != request["command"]:
        raise ValueError("The request no longer matches this run.")
    base, head = request["base"], request["head"]
    if (report["base"], report["head"]) != (base, head) or report["structural_differences"]:
        raise ValueError("The schema comparison did not pass for the requested commits.")
    if pr["head"]["sha"] != head or fetch_candidate(repo, number, head) != base:
        raise ValueError("The base or PR head changed during validation. Post a new command.")
    summary = (
        f"Schema contracts match for {report['commands']} commands and {report['definitions']} "
        f"existing definitions. {len(report['annotation_differences'])} annotation differences. "
        f"Added definitions: {', '.join(report['added_definitions']) or 'none'}. "
        f"[Full report and logs]({os.environ['GITHUB_SERVER_URL']}/{repo}/actions/runs/"
        f"{os.environ['GITHUB_RUN_ID']}). "
    )
    if command == "/check-schema":
        api(
            repo,
            f"issues/{number}/comments",
            "POST",
            {"body": summary + "Validation only. A maintainer can post `/merge-schema` to validate again and merge."},
        )
        return
    check_builds(repo, head)
    check_reviews(repo, number)
    tree = git(Path.cwd(), "merge-tree", "--write-tree", base, head).decode().splitlines()[0]
    if tree != report["tree"]:
        raise ValueError("The merge tree differs from the validated tree.")
    git(Path.cwd(), "config", "user.name", "github-actions[bot]")
    git(Path.cwd(), "config", "user.email", "41898282+github-actions[bot]@users.noreply.github.com")
    commit = (
        git(
            Path.cwd(),
            "commit-tree",
            tree,
            "-p",
            base,
            "-p",
            head,
            "-m",
            f"Merge pull request #{number}: validated schema refinement",
        )
        .decode()
        .strip()
    )
    # The new commit is a fast-forward descendant of base. The explicit lease also
    # rejects a concurrent main update, so no unvalidated recombination is published.
    env = os.environ.copy()
    credentials = base64.b64encode(f"x-access-token:{env['GH_TOKEN']}".encode()).decode()
    env.update(
        GIT_CONFIG_COUNT="1",
        GIT_CONFIG_KEY_0="http.https://github.com/.extraheader",
        GIT_CONFIG_VALUE_0=f"AUTHORIZATION: basic {credentials}",
    )
    git(
        Path.cwd(), "push", f"--force-with-lease=refs/heads/main:{base}", "origin", f"{commit}:refs/heads/main", env=env
    )
    print(f"Merged {commit}", flush=True)
    # Publication has succeeded. A later notification failure must not report
    # that the merge failed or encourage replaying the merge request.
    followups = [(f"issues/{number}/comments", {"body": summary + f"Merged as `{commit}`."})]
    # GITHUB_TOKEN pushes do not trigger main's normal push workflows.
    followups.extend((f"actions/workflows/{workflow}/dispatches", {"ref": "main"}) for workflow in WORKFLOWS)
    for path, body in followups:
        try:
            api(repo, path, "POST", body)
        except ValueError as error:
            print(f"::warning::Merged {commit}, but {path} failed: {error}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=["prepare", "finish", "failure"])
    parser.add_argument("--request", type=Path, default=Path("schema-request.json"))
    parser.add_argument("--report", type=Path, default=Path("schema-report.json"))
    args = parser.parse_args()
    repo = os.environ["GITHUB_REPOSITORY"]
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text(encoding="utf-8"))
    try:
        if args.operation == "prepare":
            prepare(repo, event, args.request)
        elif args.operation == "finish":
            finish(repo, event, args.request, args.report)
        else:
            api(
                repo,
                f"issues/{event['issue']['number']}/comments",
                "POST",
                {
                    "body": "Schema validation or merge stopped. "
                    f"[Action run]({os.environ['GITHUB_SERVER_URL']}/{repo}/actions/runs/{os.environ['GITHUB_RUN_ID']}) "
                    "contains the failure and any generated schema report. "
                    "If the base or PR head moved, post a new command to validate the new candidate."
                },
            )
    except (KeyError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
