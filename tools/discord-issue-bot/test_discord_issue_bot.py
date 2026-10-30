"""Offline unit tests for the Discord issue bot.

Everything here runs against stubs - no Discord, GitHub or Claude access
is needed:

    python -m unittest tools.discord-issue-bot.test_discord_issue_bot

does not work because of the dash in the directory name; run instead:

    cd tools/discord-issue-bot && python -m unittest test_discord_issue_bot
"""

import datetime
import importlib.util
import os
import sys
import tempfile
import time
import unittest

import requests

_spec = importlib.util.spec_from_file_location(
    "discord_issue_bot",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "discord_issue_bot.py"))
bot = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(bot)


class FakeResponse:
    def __init__(self, status, payload=None, text="err"):
        self.status_code = status
        self.ok = 200 <= status < 300
        self.text = text
        self._payload = payload if payload is not None else {}
        self.headers = {}

    def json(self):
        return self._payload


def github_client(session):
    client = bot.GitHubClient.__new__(bot.GitHubClient)
    client.repository = "owner/repo"
    client.session = session
    return client


def discord_client(session):
    client = bot.DiscordClient.__new__(bot.DiscordClient)
    client.session = session
    return client


GENUINE_MARKER_BODY = "summary\n`{0} {1}`\n<!-- {0} {1} -->\n".format(
    bot.ISSUE_MARKER_PREFIX, "12345")
BORDERLINE_MARKER_BODY = "intro\n`{0}`\n<!-- {0} -->\n".format(
    bot.BORDERLINE_ISSUE_MARKER)


class NoSleepTestCase(unittest.TestCase):
    def setUp(self):
        self._sleep = time.sleep
        time.sleep = lambda seconds: None
        self._log = bot.log
        self.logged = []
        bot.log = self.logged.append

    def tearDown(self):
        time.sleep = self._sleep
        bot.log = self._log


class NormalizeTests(NoSleepTestCase):
    def normalize(self, **overrides):
        entry = {"id": "1", "actionable": True, "confidence": 0.9,
                 "title": "t", "summary": "s", "type": "bug"}
        entry.update(overrides)
        return bot.ClassifierBase._normalize(entry)

    def test_title_whitespace_collapsed_and_truncated(self):
        result = self.normalize(title="A\n\nB\tC   D")
        self.assertEqual(result["title"], "A B C D")
        result = self.normalize(title="x" * 300)
        self.assertEqual(len(result["title"]), 250)

    def test_summary_cannot_carry_the_marker_or_html_comments(self):
        result = self.normalize(
            summary="<!-- hidden -->{} 123".format(bot.ISSUE_MARKER_PREFIX))
        self.assertNotIn("<!--", result["summary"])
        self.assertNotIn(bot.ISSUE_MARKER_PREFIX, result["summary"])

    def test_non_finite_confidence_becomes_zero(self):
        self.assertEqual(self.normalize(confidence="nan")["confidence"], 0.0)

    def test_actionable_coercion(self):
        self.assertTrue(self.normalize(actionable="true")["actionable"])
        self.assertTrue(self.normalize(actionable=1)["actionable"])
        self.assertFalse(self.normalize(actionable="no")["actionable"])

    def test_type_restricted(self):
        self.assertIsNone(self.normalize(type="epic")["type"])


class SanitizerTests(NoSleepTestCase):
    def test_mentions_neutralized(self):
        self.assertNotIn("@here", bot.neutralize_mentions("@here hello"))

    def test_issue_body_code_spans_survive_crafted_names(self):
        body = bot.build_issue_body(
            {"id": "1", "channel_id": "c", "content": "hello world",
             "author": {"username": "evil`[x](http://e)`name"}},
            {"name": "gen`eral", "guild_id": "g"},
            {"summary": "s"})
        self.assertNotIn("evil`", body)
        self.assertNotIn("gen`eral", body)
        self.assertIn("`{} 1`".format(bot.ISSUE_MARKER_PREFIX), body)

    def test_borderline_line_quotes_title(self):
        line = bot.borderline_line(
            {"id": "1", "channel_id": "c"}, {"guild_id": "g"},
            {"title": "Fix `crash` [x](y)"}, 0.6)
        self.assertIn("`Fix 'crash' [x](y)`", line)


class CandidateTests(NoSleepTestCase):
    class Config:
        min_message_length = 25

    def test_attachment_exempts_short_caption(self):
        message = {"type": 0, "content": "crash on export",
                   "attachments": [{"id": "a"}], "author": {"username": "u"}}
        self.assertIsNone(bot.skip_reason(message, self.Config))

    def test_captionless_attachment_is_not_a_candidate(self):
        message = {"type": 0, "content": " ",
                   "attachments": [{"id": "a"}], "author": {"username": "u"}}
        self.assertIsNotNone(bot.skip_reason(message, self.Config))

    def test_short_plain_message_is_not_a_candidate(self):
        message = {"type": 0, "content": "crash on export",
                   "attachments": [], "author": {"username": "u"}}
        self.assertIsNotNone(bot.skip_reason(message, self.Config))

    def test_bots_are_never_candidates(self):
        message = {"type": 0, "content": "x" * 40,
                   "author": {"username": "u", "bot": True}}
        self.assertIsNotNone(bot.skip_reason(message, self.Config))


class DedupeSearchTests(NoSleepTestCase):
    def search_client(self, items):
        class Session:
            def get(self, url, params=None, timeout=None):
                return FakeResponse(200, {"items": items})
        return github_client(Session())

    def test_marker_from_bot_author_matches(self):
        client = self.search_client([
            {"number": 2, "body": GENUINE_MARKER_BODY,
             "user": {"login": bot.BOT_ISSUE_AUTHOR}}])
        self.assertEqual(client.find_issue_for_message("12345")["number"], 2)

    def test_marker_from_other_author_is_hostile(self):
        client = self.search_client([
            {"number": 1, "body": GENUINE_MARKER_BODY,
             "user": {"login": "attacker"}}])
        self.assertIsNone(client.find_issue_for_message("12345"))

    def test_quoted_marker_does_not_count(self):
        client = self.search_client([
            {"number": 1,
             "body": "> `{} 12345`".format(bot.ISSUE_MARKER_PREFIX),
             "user": {"login": bot.BOT_ISSUE_AUTHOR}}])
        self.assertIsNone(client.find_issue_for_message("12345"))

    def test_transport_failure_is_search_failed(self):
        class Session:
            def get(self, url, params=None, timeout=None):
                raise requests.ConnectionError("down")
        client = github_client(Session())
        self.assertIs(client.find_issue_for_message("12345"), bot.SEARCH_FAILED)


class GithubRetryTests(NoSleepTestCase):
    def test_transient_500_is_retried(self):
        class Session:
            calls = 0
            def get(self, url, params=None, timeout=None):
                Session.calls += 1
                return FakeResponse(500) if Session.calls < 3 else FakeResponse(200)
        client = github_client(Session())
        self.assertTrue(client._get("u", {}).ok)
        self.assertEqual(Session.calls, 3)

    def test_404_is_not_retried(self):
        class Session:
            calls = 0
            def get(self, url, params=None, timeout=None):
                Session.calls += 1
                return FakeResponse(404)
        client = github_client(Session())
        self.assertFalse(client._get("u", {}).ok)
        self.assertEqual(Session.calls, 1)

    def test_persistent_transport_failure_never_raises(self):
        class Session:
            def get(self, url, params=None, timeout=None):
                raise requests.ConnectionError("down")
        client = github_client(Session())
        response = client._get("u", {})
        self.assertFalse(response.ok)


class CreateIssueTests(NoSleepTestCase):
    def test_labels_dropped_from_the_back_on_422(self):
        class Session:
            def __init__(self):
                self.label_sets = []
                self.responses = [FakeResponse(422), FakeResponse(201, {"number": 1})]
            def post(self, url, json=None, timeout=None):
                self.label_sets.append(list(json["labels"]))
                return self.responses.pop(0)
        session = Session()
        client = github_client(session)
        issue = client.create_issue("t", "b", ["bug", "discord"])
        self.assertEqual(issue["number"], 1)
        self.assertEqual(session.label_sets, [["bug", "discord"], ["bug"]])


class CreatedIssuesOutputTests(NoSleepTestCase):
    def setUp(self):
        super().setUp()
        handle, self.path = tempfile.mkstemp()
        os.close(handle)
        self.previous = os.environ.get("GITHUB_OUTPUT")
        os.environ["GITHUB_OUTPUT"] = self.path

    def tearDown(self):
        if self.previous is None:
            os.environ.pop("GITHUB_OUTPUT", None)
        else:
            os.environ["GITHUB_OUTPUT"] = self.previous
        os.remove(self.path)
        super().tearDown()

    def read(self):
        with open(self.path, encoding="utf-8") as stream:
            return stream.read()

    def test_numbers_written_space_separated(self):
        bot.write_created_issues_output([722, 723])
        self.assertEqual(self.read(), "created_issues=722 723\n")

    def test_nothing_written_without_issues(self):
        bot.write_created_issues_output([])
        self.assertEqual(self.read(), "")


class BorderlineIssueTests(NoSleepTestCase):
    def test_marker_requires_bot_author(self):
        class Session:
            def get(self, url, params=None, timeout=None):
                return FakeResponse(200, [
                    {"number": 1, "body": BORDERLINE_MARKER_BODY,
                     "user": {"login": "attacker"}},
                    {"number": 2, "body": BORDERLINE_MARKER_BODY,
                     "user": {"login": bot.BOT_ISSUE_AUTHOR}}])
        client = github_client(Session())
        self.assertEqual(client.find_borderline_issue()["number"], 2)

    def test_listing_sorts_by_updated(self):
        captured = {}
        class Session:
            def get(self, url, params=None, timeout=None):
                captured.update(params)
                return FakeResponse(200, [])
        github_client(Session()).find_borderline_issue()
        self.assertEqual(captured["sort"], "updated")

    def test_exhausted_scan_is_unknown_not_absent(self):
        class Session:
            def get(self, url, params=None, timeout=None):
                return FakeResponse(200, [{"number": index, "body": ""}
                                          for index in range(100)])
        client = github_client(Session())
        self.assertIs(client.find_borderline_issue(), bot.SEARCH_FAILED)

    def test_report_failures_are_never_counted_fatal(self):
        class Session:
            def get(self, url, params=None, timeout=None):
                return FakeResponse(500)
        client = github_client(Session())
        state = {"borderline": ["- x"], "github_failures": 0}
        bot.report_borderline(client, state)
        self.assertEqual(state["github_failures"], 0)

    def test_rolling_issue_created_without_the_discord_label(self):
        class Session:
            def __init__(self):
                self.created_with = None
            def get(self, url, params=None, timeout=None):
                return FakeResponse(200, [])
            def post(self, url, json=None, timeout=None):
                if url.endswith("/issues"):
                    self.created_with = list(json["labels"])
                    return FakeResponse(201, {"number": 9})
                return FakeResponse(201, {})
        session = Session()
        state = {"borderline": ["- x"], "github_failures": 0}
        bot.report_borderline(github_client(session), state)
        self.assertEqual(session.created_with, [])


class DiscordRequestTests(NoSleepTestCase):
    def request_client(self, responses):
        responses = list(responses)
        class Session:
            calls = 0
            def request(self, method, url, timeout=None, **kwargs):
                Session.calls += 1
                return responses.pop(0)
        client = discord_client(Session())
        return client, Session

    def test_non_idempotent_5xx_is_not_retried(self):
        client, session = self.request_client([FakeResponse(502)])
        response = client._request("POST", "/x", idempotent=False)
        self.assertEqual(response.status_code, 502)
        self.assertEqual(session.calls, 1)

    def test_idempotent_5xx_is_retried(self):
        client, session = self.request_client(
            [FakeResponse(502), FakeResponse(200)])
        self.assertTrue(client._request("GET", "/x").ok)
        self.assertEqual(session.calls, 2)


class PaginationTests(NoSleepTestCase):
    NOW = datetime.datetime(2026, 8, 28, 12, 0, tzinfo=datetime.timezone.utc)

    def run_pages(self, pages):
        pages = list(pages)
        client = discord_client(None)
        client._request = lambda method, path, idempotent=True, **kwargs: \
            FakeResponse(200, pages.pop(0) if pages else [])
        return client.recent_messages("c1", self.NOW - datetime.timedelta(hours=24))

    def page(self, count, start):
        return [{"id": str(10**9 - start - index),
                 "timestamp": (self.NOW - datetime.timedelta(
                     seconds=start + index)).isoformat()}
                for index in range(count)]

    def test_cap_reached_warns(self):
        pages = [self.page(100, index * 100) for index in range(20)]
        messages = self.run_pages(pages)
        self.assertEqual(len(messages), 2000)
        self.assertTrue(any("scan limit" in line for line in self.logged))

    def test_short_final_page_means_history_ended(self):
        pages = [self.page(100, 0), self.page(30, 100)]
        messages = self.run_pages(pages)
        self.assertEqual(len(messages), 130)
        self.assertFalse(any("scan limit" in line for line in self.logged))


class ConfigTests(NoSleepTestCase):
    FULL_ENV = {"DISCORD_BOT_TOKEN": "t", "DISCORD_CHANNEL_IDS": "1,2",
                "GITHUB_TOKEN": "g", "GITHUB_REPOSITORY": "o/r",
                "CLAUDE_CODE_OAUTH_TOKEN": "o"}

    def setUp(self):
        super().setUp()
        self._env = dict(os.environ)
        for name in ("LOOKBACK_MINUTES", "MAX_ISSUES_PER_RUN",
                     "MAX_ISSUES_PER_DAY", "MIN_CONFIDENCE",
                     "MIN_MESSAGE_LENGTH", "DRY_RUN", "CLEANUP_SEEN_MARKS"):
            os.environ.pop(name, None)
        os.environ.update(self.FULL_ENV)

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._env)
        super().tearDown()

    def test_defaults_parse(self):
        config, missing = bot.Config.from_env()
        self.assertEqual(missing, [])
        self.assertEqual(config.channel_ids, ["1", "2"])
        self.assertEqual(config.lookback_minutes, 1440)

    def test_separator_only_channel_ids_fail(self):
        os.environ["DISCORD_CHANNEL_IDS"] = " , ,"
        config, missing = bot.Config.from_env()
        self.assertIsNone(config)
        self.assertTrue(any("contains no channel IDs" in item for item in missing))

    def test_non_numeric_setting_exits_cleanly(self):
        os.environ["LOOKBACK_MINUTES"] = "abc"
        with self.assertRaises(SystemExit) as raised:
            bot.Config.from_env()
        self.assertEqual(raised.exception.code, 1)


class GroupingTests(NoSleepTestCase):
    def message(self, message_id, author_id, content="text", **extra):
        message = {"id": message_id, "type": 0, "content": content,
                   "author": {"id": author_id, "username": author_id}}
        message.update(extra)
        return message

    def test_consecutive_messages_of_one_author_form_one_group(self):
        messages = [self.message("1", "a"), self.message("2", "a"),
                    self.message("3", "b"), self.message("4", "a")]
        groups = bot.group_messages(messages)
        self.assertEqual([[m["id"] for m in g] for g in groups],
                         [["1", "2"], ["3"], ["4"]])

    def test_bot_message_ends_a_group(self):
        messages = [self.message("1", "a"),
                    self.message("2", "bot", author={"id": "bot", "bot": True}),
                    self.message("3", "a")]
        groups = bot.group_messages(messages)
        self.assertEqual([[m["id"] for m in g] for g in groups], [["1"], ["3"]])

    def test_merge_joins_text_and_attachments_under_the_first_id(self):
        merged = bot.merge_group([
            self.message("1", "a", "Export crashes"),
            self.message("2", "a", " ", attachments=[{"id": "x"}]),
            self.message("3", "a", "when the plan has curtain walls")])
        self.assertEqual(merged["id"], "1")
        self.assertEqual(merged["content"],
                         "Export crashes\nwhen the plan has curtain walls")
        self.assertEqual(merged["attachments"], [{"id": "x"}])


class FakeDiscord:
    """Reactions live on the message dicts, as the API returns them."""

    def __init__(self, messages, add_ok=True):
        self.messages = messages
        self.add_ok = add_ok
        self.calls = []

    def get_channel(self, channel_id):
        return {"name": "general", "guild_id": "g"}

    def recent_messages(self, channel_id, since, max_pages=20):
        return self.messages

    has_mark = bot.DiscordClient.has_mark

    def add_mark(self, channel_id, message_id, emoji):
        self.calls.append(("add", message_id, emoji))
        if not self.add_ok:
            return False
        message = next(m for m in self.messages if m["id"] == message_id)
        message.setdefault("reactions", []).append({"emoji": {"name": emoji}, "me": True})
        return True

    def remove_mark(self, channel_id, message_id, emoji):
        self.calls.append(("remove", message_id, emoji))
        message = next(m for m in self.messages if m["id"] == message_id)
        message["reactions"] = [r for r in message.get("reactions", [])
                                if r["emoji"]["name"] != emoji]
        return True

    def reply(self, channel_id, message_id, content):
        self.calls.append(("reply", message_id))

    def seen(self):
        return [m["id"] for m in self.messages if self.has_mark(m, bot.SEEN_MARK)]


class FakeClassifier:
    def __init__(self, verdicts):
        self.verdicts = verdicts
        self.classified = []

    def classify_batch(self, chunk):
        result = {}
        for candidate in chunk:
            self.classified.append(candidate["id"])
            verdict = self.verdicts.get(candidate["id"], "chat")
            if verdict == "fail":
                continue
            result[candidate["id"]] = {
                "actionable": verdict == "bug", "type": "bug" if verdict == "bug" else None,
                "confidence": 0.9, "title": "t", "summary": "s"}
        return result


class FakeGitHub:
    def __init__(self, search=None):
        self.search = search
        self.created = []

    def find_issue_for_message(self, message_id):
        return self.search

    def create_issue(self, title, body, labels):
        self.created.append(body)
        number = len(self.created)
        return {"number": number, "html_url": "https://x/{}".format(number)}


class CheckpointTests(NoSleepTestCase):
    NOW = datetime.datetime.now(datetime.timezone.utc)

    class Config:
        min_message_length = 5
        min_confidence = 0.7
        max_issues_per_run = 5
        lookback_minutes = 1440
        dry_run = False

    def message(self, message_id, author="a", minutes_ago=60, seen=False, content=None):
        message = {"id": message_id, "type": 0, "channel_id": "c",
                   "content": content or "message number {}".format(message_id),
                   "author": {"id": author, "username": author},
                   "timestamp": (self.NOW - datetime.timedelta(minutes=minutes_ago)).isoformat()}
        if seen:
            message["reactions"] = [{"emoji": {"name": bot.SEEN_MARK}, "me": True}]
        return message

    def run_channel(self, messages, verdicts=None, github=None, config=None, add_ok=True):
        discord = FakeDiscord(messages, add_ok)
        classifier = FakeClassifier(verdicts or {})
        github = github or FakeGitHub()
        state = {"created": 0, "created_numbers": [], "unreadable_channels": 0,
                 "github_failures": 0, "human_messages": 0, "messages_with_signal": 0,
                 "borderline": []}
        self.state = state
        bot.process_channel("c", config or self.Config, discord, github, classifier, state)
        return discord, classifier, github

    def test_only_the_last_checked_message_carries_the_mark(self):
        messages = [self.message("1", "a"), self.message("2", "b"), self.message("3", "c")]
        discord, classifier, _ = self.run_channel(messages)
        self.assertEqual(classifier.classified, ["1", "2", "3"])
        self.assertEqual(discord.seen(), ["3"])

    def test_messages_up_to_the_checkpoint_are_skipped(self):
        messages = [self.message("1", "a", seen=True), self.message("2", "b"),
                    self.message("3", "c")]
        discord, classifier, _ = self.run_channel(messages)
        self.assertEqual(classifier.classified, ["2", "3"])
        self.assertEqual(discord.seen(), ["3"])

    def test_old_marks_in_the_window_are_removed(self):
        messages = [self.message("1", "a", seen=True), self.message("2", "b", seen=True),
                    self.message("3", "c")]
        discord, classifier, _ = self.run_channel(messages)
        self.assertEqual(classifier.classified, ["3"])
        self.assertEqual(discord.seen(), ["3"])

    def test_classifier_down_stops_the_scan_at_the_first_failure(self):
        messages = [self.message("1", "a"), self.message("2", "b"),
                    self.message("3", "c"), self.message("4", "d")]
        discord, classifier, _ = self.run_channel(messages, {"2": "fail", "3": "fail"})
        self.assertEqual(classifier.classified, ["1", "2", "3"])
        self.assertEqual(discord.seen(), ["1"])

    def test_one_unclassifiable_message_is_skipped_with_a_trace(self):
        messages = [self.message("1", "a"), self.message("2", "b"), self.message("3", "c")]
        discord, classifier, _ = self.run_channel(messages, {"2": "fail"})
        self.assertEqual(classifier.classified, ["1", "2", "3"])
        self.assertEqual(discord.seen(), ["3"])
        self.assertTrue(any("Could not be classified" in line for line in self.state["borderline"]))

    def test_a_filed_message_does_not_vouch_for_the_classifier(self):
        messages = [self.message("1", "a"), self.message("2", "b"), self.message("3", "c")]
        messages[1]["reactions"] = [{"emoji": {"name": bot.PROCESSED_MARK}, "me": True}]
        discord, classifier, _ = self.run_channel(messages, {"1": "fail", "3": "fail"})
        self.assertEqual(classifier.classified, ["1", "3"])
        self.assertEqual(discord.seen(), [])

    def test_failed_duplicate_search_stops_the_scan(self):
        messages = [self.message("1", "a"), self.message("2", "b")]
        discord, classifier, github = self.run_channel(
            messages, {"1": "bug"}, github=FakeGitHub(search=bot.SEARCH_FAILED))
        self.assertEqual(github.created, [])
        self.assertEqual(classifier.classified, ["1"])
        self.assertEqual(discord.seen(), [])

    def test_issue_cap_stops_the_scan(self):
        class CapConfig(self.Config):
            max_issues_per_run = 1
        messages = [self.message("1", "a"), self.message("2", "b"),
                    self.message("3", "c"), self.message("4", "d")]
        discord, classifier, github = self.run_channel(
            messages, {"1": "bug", "2": "bug"}, config=CapConfig)
        self.assertEqual(len(github.created), 1)
        self.assertEqual(classifier.classified, ["1", "2"])
        self.assertEqual(discord.seen(), ["1"])

    def test_failed_add_keeps_the_old_checkpoint(self):
        messages = [self.message("1", "a", seen=True), self.message("2", "b")]
        discord, _, _ = self.run_channel(messages, add_ok=False)
        self.assertEqual(discord.seen(), ["1"])

    def test_consecutive_messages_are_filed_as_one_issue(self):
        messages = [self.message("1", "a", content="Export to IFC crashes"),
                    self.message("2", "a", content="only with curtain walls"),
                    self.message("3", "b")]
        discord, classifier, github = self.run_channel(messages, {"1": "bug"})
        self.assertEqual(classifier.classified, ["1", "3"])
        self.assertEqual(len(github.created), 1)
        self.assertIn("> only with curtain walls", github.created[0])
        self.assertIn(("reply", "1"), discord.calls)
        self.assertIn(("add", "1", bot.PROCESSED_MARK), discord.calls)
        self.assertEqual(discord.seen(), ["3"])

    def test_group_marked_on_its_last_message(self):
        messages = [self.message("1", "a"), self.message("2", "a"), self.message("3", "b")]
        discord, _, _ = self.run_channel(messages, {"3": "fail"})
        self.assertEqual(discord.seen(), ["2"])

    def test_a_run_still_being_typed_is_left_for_the_next_scan(self):
        messages = [self.message("1", "a"), self.message("2", "b", minutes_ago=2)]
        discord, classifier, _ = self.run_channel(messages)
        self.assertEqual(classifier.classified, ["1"])
        self.assertEqual(discord.seen(), ["1"])

    def test_a_short_message_moves_the_checkpoint_unclassified(self):
        messages = [self.message("1", "a", seen=True), self.message("2", "b"),
                    self.message("3", "c", content="ok")]
        discord, classifier, _ = self.run_channel(messages)
        self.assertEqual(classifier.classified, ["2"])
        self.assertEqual(discord.seen(), ["3"])
        self.assertTrue(any("message 3: skipped, shorter than" in line
                            for line in self.logged))

    def test_a_short_message_does_not_pass_an_unclassified_one(self):
        messages = [self.message("1", "a"), self.message("2", "b", content="ok")]
        discord, classifier, _ = self.run_channel(messages, {"1": "fail"})
        self.assertEqual(classifier.classified, ["1"])
        self.assertEqual(discord.seen(), [])

    def test_every_skip_is_logged(self):
        bot_message = self.message("2", "x")
        bot_message["author"]["bot"] = True
        messages = [self.message("1", "a", seen=True), bot_message,
                    self.message("3", "b", minutes_ago=2)]
        discord, classifier, _ = self.run_channel(messages)
        self.assertEqual(classifier.classified, [])
        self.assertEqual(discord.seen(), ["1"])
        self.assertTrue(any("2 after the" in line for line in self.logged))
        self.assertTrue(any("message 2: skipped, posted by a bot" in line
                            for line in self.logged))
        self.assertTrue(any("message 3: left for the next run" in line
                            for line in self.logged))

    def test_filed_group_is_not_classified_again(self):
        messages = [self.message("1", "a"), self.message("2", "b")]
        messages[0]["reactions"] = [{"emoji": {"name": bot.PROCESSED_MARK}, "me": True}]
        discord, classifier, _ = self.run_channel(messages)
        self.assertEqual(classifier.classified, ["2"])
        self.assertEqual(discord.seen(), ["2"])

    def test_dry_run_moves_nothing(self):
        class DryConfig(self.Config):
            dry_run = True
        messages = [self.message("1", "a", seen=True), self.message("2", "b")]
        discord, classifier, _ = self.run_channel(messages, config=DryConfig)
        self.assertEqual(classifier.classified, ["2"])
        self.assertEqual(discord.seen(), ["1"])


class CleanupTests(NoSleepTestCase):
    def test_all_but_the_newest_mark_removed(self):
        def message(message_id, seen):
            message = {"id": message_id}
            if seen:
                message["reactions"] = [{"emoji": {"name": bot.SEEN_MARK}, "me": True}]
            return message
        messages = [message("1", True), message("2", False), message("3", True),
                    message("4", True), message("5", False)]
        discord = FakeDiscord(messages)

        class Config:
            dry_run = False
        self.assertTrue(bot.cleanup_seen_marks("c", Config, discord))
        self.assertEqual(discord.seen(), ["4"])


class IntentDetectionTests(NoSleepTestCase):
    def test_attachment_only_messages_prove_the_intent(self):
        message = {"content": "", "attachments": [{"id": "a"}], "embeds": [],
                   "author": {"username": "u"}}
        blank = {"content": "", "attachments": [], "embeds": [],
                 "author": {"username": "u"}}
        self.assertTrue(message.get("content", "").strip()
                        or message.get("attachments") or message.get("embeds"))
        self.assertFalse(blank.get("content", "").strip()
                         or blank.get("attachments") or blank.get("embeds"))


if __name__ == "__main__":
    unittest.main()
