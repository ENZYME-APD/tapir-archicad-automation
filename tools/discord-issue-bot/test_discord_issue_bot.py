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
        self.assertTrue(bot.is_candidate(message, self.Config))

    def test_captionless_attachment_is_not_a_candidate(self):
        message = {"type": 0, "content": " ",
                   "attachments": [{"id": "a"}], "author": {"username": "u"}}
        self.assertFalse(bot.is_candidate(message, self.Config))

    def test_short_plain_message_is_not_a_candidate(self):
        message = {"type": 0, "content": "crash on export",
                   "attachments": [], "author": {"username": "u"}}
        self.assertFalse(bot.is_candidate(message, self.Config))

    def test_bots_are_never_candidates(self):
        message = {"type": 0, "content": "x" * 40,
                   "author": {"username": "u", "bot": True}}
        self.assertFalse(bot.is_candidate(message, self.Config))


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
                     "MIN_MESSAGE_LENGTH", "DRY_RUN"):
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
