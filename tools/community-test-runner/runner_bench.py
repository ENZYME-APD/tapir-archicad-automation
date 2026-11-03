"""'bench': a speed and token benchmark of Tapir against an Archicad that is already open.

Seeds an empty, untitled project with columns, walls and slabs at growing scales (100, 1000, 5000
elements by default), times every case of the suite at each scale, measures how big each answer is
and how many tokens it costs an agent (the whole answer, and the first page the Tapir MCP server
hands out), runs bursts of parallel calls, then deletes what it made. --read-only measures the
reads on the open project as it is and creates nothing. Results are a JSON file that a later run
compares against (bench-compare), plus a Markdown report. The suite is data: benchmarks/*.json.

Tokens are estimated (GUIDs and the rest of the text are counted separately). --exact-tokens counts
them with Anthropic's count_tokens endpoint (needs the anthropic package and an API key; sends the
measured answers, synthetic unless --read-only, to the API).
"""
from __future__ import annotations

import json
import math
import platform
import re
import statistics
import time
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from pathlib import Path
from typing import Any, Callable

import tapir_test_runner as r

SUITE_DIR = Path(__file__).resolve().parent / "benchmarks"
DEFAULT_SUITE = SUITE_DIR / "standard.json"
MCP_PAGE_SIZE = 100  # the Tapir MCP server returns a list of more than 100 items one page at a time
GUID = re.compile(r"[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}")
GUID_TOKENS = 24       # a GUID splits into many short tokens
CHARS_PER_TOKEN = 3.4  # the rest of indented JSON
EXPRESSION = re.compile(r"\$(\w+)(?:\.(\w+))?([+-]\d+(?:\.\d+)?)?")
TIMEOUT = 600.0
EMPTY_TYPES = ["Wall", "Column", "Slab", "Beam", "Roof", "Stair", "Zone", "Object"]


# --- measuring -------------------------------------------------------------------------------

def estimate_tokens(text: str) -> int:
    guids = len(GUID.findall(text))
    return round(guids * GUID_TOKENS + (len(text) - guids * 36) / CHARS_PER_TOKEN)


def mcp_text(result: Any, page: bool) -> str:
    """What an agent reads: the MCP server sends a result as indented JSON, a long list one page at a time."""
    if page and isinstance(result, dict):
        lists = [k for k, v in result.items() if isinstance(v, list) and len(v) > MCP_PAGE_SIZE]
        if lists:
            key = max(lists, key=lambda k: len(result[k]))
            result = dict(result, **{key: result[key][:MCP_PAGE_SIZE], "next_page_token": "x" * 40})
    return json.dumps(result, indent=2, ensure_ascii=False)


def exact_counter(model: str) -> Callable[[str], int]:
    import anthropic  # only for --exact-tokens

    client = anthropic.Anthropic()

    def count(text: str) -> int:
        return client.messages.count_tokens(model=model, messages=[{"role": "user", "content": text}]).input_tokens
    overhead = count("x") - 1
    return lambda text: count(text) - overhead


def error_of(result: Any) -> str | None:
    if not isinstance(result, dict):
        return None
    error = result.get("__envelopeError__") or result.get("error")
    if error:
        return str(error.get("message", error) if isinstance(error, dict) else error)[:160]
    return None


def item_errors(result: Any) -> int:
    """Failures inside an answer that succeeded as a whole: executionResults or list items with an error."""
    if not isinstance(result, dict):
        return 0
    count = 0
    for value in result.values():
        if isinstance(value, list):
            count += sum(1 for v in value if isinstance(v, dict) and ("error" in v or v.get("success") is False))
    return count


def largest_list(result: Any) -> int:
    if not isinstance(result, dict):
        return 0
    return max((len(v) for v in result.values() if isinstance(v, list)), default=0)


def percentile(values: list[float], q: float) -> float:
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, max(0, math.ceil(q * len(ordered)) - 1))]


def timing(ms: list[float]) -> dict:
    return {"min": round(min(ms), 2), "p50": round(statistics.median(ms), 2), "p95": round(percentile(ms, 0.95), 2),
            "max": round(max(ms), 2), "mean": round(statistics.fmean(ms), 2)}


def fill(value: Any, variables: dict) -> Any:
    """'$name', '$name.key' and '$x+0.8' take values from variables; {"$each": list, "as": item} maps a list."""
    if isinstance(value, dict):
        if "$each" in value:
            return [fill(value["as"], dict(variables, item=item)) for item in variables[value["$each"]]]
        return {k: fill(v, variables) for k, v in value.items()}
    if isinstance(value, list):
        return [fill(v, variables) for v in value]
    if isinstance(value, str) and (m := EXPRESSION.fullmatch(value)):
        name, key, offset = m.groups()
        found = variables[name][key] if key else variables[name]
        return found + float(offset) if offset else found
    return value


def uses(value: Any, name: str) -> bool:
    return f'"${name}' in json.dumps(value) or f'"$each": "{name}"' in json.dumps(value)


# --- the run ---------------------------------------------------------------------------------

class Bench:
    def __init__(self, send: Callable[..., dict], suite: dict, log: Callable[[str], None] = print,
                 count_tokens: Callable[[str], int] | None = None, stop: Callable[[], bool] = lambda: False):
        self.send, self.suite, self.log, self.count_tokens, self.stop = send, suite, log, count_tokens, stop
        self.variables: dict[str, Any] = {}
        self.created: list[dict] = []
        self.next_index = 0  # grid cell of the next seeded element
        self.results: dict[str, Any] = {"cases": [], "seed": [], "stress": [], "notes": []}

    def call(self, command: str, params: dict | None) -> tuple[float, Any]:
        start = time.perf_counter()
        try:
            result = self.send(command, params or {}, TIMEOUT)
        except r.TapirError as exc:
            result = {"__envelopeError__": {"message": str(exc)}}
        return (time.perf_counter() - start) * 1000, result

    def tokens(self, result: Any) -> dict:
        full, page = mcp_text(result, False), mcp_text(result, True)
        estimate, page_estimate = estimate_tokens(full), estimate_tokens(page)
        out = {"bytes": len(json.dumps(result, separators=(",", ":"), ensure_ascii=False).encode("utf-8")),
               "tokens": estimate, "mcpPageTokens": page_estimate, "tokenSource": "estimate"}
        if self.count_tokens:
            exact_page = self.count_tokens(page)
            ratio = exact_page / max(1, page_estimate)
            out.update(mcpPageTokens=exact_page, tokenSource="exact",
                       tokens=self.count_tokens(full) if len(full) < 1_500_000 else round(estimate * ratio))
        return out

    # cases

    def case(self, spec: dict, scale: int | None, repeat: int, warmup: int) -> None:
        params = fill(spec.get("params", {}), self.variables)
        for _ in range(warmup):
            self.call(spec["command"], params)
        times, result = [], None
        for _ in range(repeat):
            ms, result = self.call(spec["command"], params)
            times.append(ms)
        error = error_of(result)
        elements = len(self.variables["elements"]) if uses(spec.get("params", {}), "elements") else largest_list(result)
        row = {"id": spec["id"], "command": spec["command"], "scale": scale, "runs": repeat, "elements": elements,
               "ms": timing(times), "error": error, "itemErrors": item_errors(result)}
        if not error:
            row.update(self.tokens(result))
            if elements:
                row["msPerElement"] = round(row["ms"]["p50"] / elements, 3)
                row["tokensPerElement"] = round(row["tokens"] / elements, 1)
        self.results["cases"].append(row)
        self.log(line(row))

    def setup(self) -> None:
        for step in self.suite.get("setup", []):
            _, result = self.call(step["command"], fill(step.get("params", {}), self.variables))
            if error_of(result):
                self.results["notes"].append(f"setup {step['command']} failed: {error_of(result)}")
            for name, path in step.get("save", {}).items():
                try:
                    self.variables[name] = r.resolve(result, path)
                except (KeyError, IndexError, TypeError, ValueError):
                    self.variables[name] = []

    def seed_to(self, scale: int) -> None:
        need = scale - len(self.created)
        if need <= 0:
            return
        seeds = self.suite["seed"]
        counts = [int(need * s["share"]) for s in seeds]
        counts[0] += need - sum(counts)
        index = self.next_index
        for spec, count in zip(seeds, counts):
            made, start = 0, time.perf_counter()
            for first in range(0, count, spec.get("chunk", 500)):
                items = []
                for i in range(index, index + min(spec.get("chunk", 500), count - first)):
                    items.append(fill(spec["item"], {"x": (i % 50) * 2.0, "y": (i // 50) * 2.0, "i": i}))
                index += len(items)
                self.next_index = index
                _, result = self.call(spec["command"], {spec["listKey"]: items})
                if error_of(result):
                    raise RuntimeError(f"seeding with {spec['command']} failed: {error_of(result)}")
                ids = [{"elementId": e["elementId"]} for e in result.get("elements", []) if isinstance(e, dict) and "elementId" in e]
                self.created.extend(ids)
                made += len(ids)
            seconds = time.perf_counter() - start
            row = {"scale": scale, "command": spec["command"], "asked": count, "created": made, "seconds": round(seconds, 3),
                   "perSecond": round(made / seconds, 1) if seconds else None}
            self.results["seed"].append(row)
            self.log(f"seed {spec['command']} {made}/{count} in {seconds:.1f} s ({row['perSecond']}/s)")

    def stress(self, spec: dict) -> None:
        params = fill(spec.get("params", {}), self.variables)
        for workers in spec["concurrency"]:
            times: list[float] = []
            errors = 0
            start = time.perf_counter()
            with ThreadPoolExecutor(workers) as pool:
                for ms, result in pool.map(lambda _: self.call(spec["command"], params), range(spec["calls"])):
                    times.append(ms)
                    errors += error_of(result) is not None
            seconds = time.perf_counter() - start
            row = {"id": spec["id"], "command": spec["command"], "concurrency": workers, "calls": spec["calls"],
                   "seconds": round(seconds, 3), "perSecond": round(spec["calls"] / seconds, 1), "errors": errors, "ms": timing(times)}
            self.results["stress"].append(row)
            self.log(f"stress {spec['id']} x{workers}: {row['perSecond']}/s, p95 {row['ms']['p95']:.0f} ms, {errors} errors")

    def teardown(self) -> None:
        start, failed = time.perf_counter(), 0
        for first in range(0, len(self.created), 1000):
            _, result = self.call("DeleteElements", {"elements": self.created[first:first + 1000]})
            failed += item_errors(result) + (len(self.created[first:first + 1000]) if error_of(result) else 0)
        seconds = time.perf_counter() - start
        self.results["seed"].append({"scale": len(self.created), "command": "DeleteElements", "asked": len(self.created),
                                     "created": len(self.created) - failed, "seconds": round(seconds, 3),
                                     "perSecond": round(len(self.created) / seconds, 1) if seconds else None})
        self.log(f"deleted {len(self.created) - failed}/{len(self.created)} elements in {seconds:.1f} s")
        self.created = []

    def run(self, scales: list[int] | None = None, read_only: bool = False, repeat: int | None = None) -> dict:
        suite = self.suite
        scales = sorted(scales or suite["scales"])
        repeat_default, warmup = repeat or suite.get("repeat", 5), suite.get("warmup", 1)
        meta = self.results["meta"] = environment(self.send)
        meta.update(suite=suite.get("title"), suiteVersion=suite.get("version"), readOnly=read_only,
                    scales=scales, started=datetime.now().isoformat(timespec="seconds"))
        if read_only:
            project = r.resolve(self.send("GetAllElements", {}, TIMEOUT), "elements") or []
            scales = sorted({min(s, len(project)) for s in scales} - {0}) or [0]
            meta["scales"] = scales
        else:
            ok, reason = r.project_is_scratch(self.send, {"requiresEmpty": EMPTY_TYPES})
            if not ok:
                raise RuntimeError(f"{reason} (use --read-only to measure an existing project without changing it)")
        started = time.perf_counter()
        self.setup()
        try:
            for spec in [c for c in suite["cases"] if c.get("scope") == "once"]:
                if self.stop():
                    break
                self.case(spec, None, spec.get("repeat", repeat_default), warmup)
            for scale in scales:
                if self.stop():
                    break
                if read_only:
                    self.variables["elements"] = [{"elementId": e["elementId"]} for e in project[:scale]]
                else:
                    self.seed_to(scale)
                    self.variables["elements"] = self.created[:scale]
                for spec in [c for c in suite["cases"] if c.get("scope") == "scale"]:
                    if self.stop():
                        break
                    if read_only and spec.get("write"):
                        continue
                    self.case(spec, scale, spec.get("repeat", repeat_default), warmup)
            for spec in suite.get("stress", []):
                if not self.stop():
                    self.stress(spec)
        finally:
            if self.created:
                self.teardown()
        meta["seconds"] = round(time.perf_counter() - started, 1)
        meta["stopped"] = self.stop()
        return self.results


def environment(send: Callable[..., dict]) -> dict:
    product = send("API.GetProductInfo", {}, r.DEFAULT_TIMEOUT)
    return {"archicad": " ".join(str(v) for v in (product.get("version"), product.get("languageCode")) if v),
            "archicadBuild": product.get("buildNumber"),
            "tapir": send("GetAddOnVersion", {}, r.DEFAULT_TIMEOUT).get("version", "?"),
            "machine": platform.node(), "os": platform.platform(), "cpu": platform.processor(), "python": platform.python_version()}


# --- reporting -------------------------------------------------------------------------------

def kb(n: int) -> str:
    return f"{n / 1024:.1f} KB" if n < 1024 * 1024 else f"{n / 1024 / 1024:.2f} MB"


def line(row: dict) -> str:
    where = f"{row['id']}@{row['scale']}" if row.get("scale") is not None else row["id"]
    if row.get("error"):
        return f"{where}: ERROR {row['error']}"
    text = f"{where}: p50 {row['ms']['p50']:.0f} ms, {kb(row['bytes'])}, ~{row['tokens']:,} tok"
    if row.get("tokensPerElement"):
        text += f" ({row['tokensPerElement']}/element)"
    if row["mcpPageTokens"] != row["tokens"]:
        text += f", MCP page {row['mcpPageTokens']:,}"
    return text + (f", {row['itemErrors']} item errors" if row.get("itemErrors") else "")


def headline(results: dict) -> str:
    m = results["meta"]
    cases = results["cases"]
    errors = sum(1 for c in cases if c.get("error"))
    tokens = sum(c.get("tokens", 0) for c in cases)
    return (f"tapir-bench: AC{m['archicad']} tapir {m['tapir']} | scales {'/'.join(map(str, m['scales']))}"
            f"{' read-only' if m.get('readOnly') else ''} | {len(cases)} cases, {errors} errors | ~{tokens:,} tok in total"
            f" | {m.get('seconds', 0):.0f} s{' (stopped)' if m.get('stopped') else ''}")


def report(results: dict) -> str:
    m = results["meta"]
    out = [f"# {headline(results)}", "",
           f"{m['started']} on {m['machine']} ({m['os']}), Archicad build {m['archicadBuild']}, suite "
           f"'{m['suite']}' v{m['suiteVersion']}. Tokens: {results['cases'][0].get('tokenSource', 'estimate') if results['cases'] else '-'}"
           " (the whole answer as indented JSON; 'MCP page' = what the Tapir MCP server returns first).", ""]
    out += ["## Cases", "", "| case | scale | p50 ms | p95 ms | ms/element | size | tokens | tokens/element | MCP page | errors |",
            "|---|---|---|---|---|---|---|---|---|---|"]
    for c in results["cases"]:
        if c.get("error"):
            out.append(f"| {c['id']} | {c['scale'] or '-'} | | | | | | | | {c['error']} |")
            continue
        out.append(f"| {c['id']} | {c['scale'] or '-'} | {c['ms']['p50']:.1f} | {c['ms']['p95']:.1f} | {c.get('msPerElement', '')} "
                   f"| {kb(c['bytes'])} | {c['tokens']:,} | {c.get('tokensPerElement', '')} | {c['mcpPageTokens']:,} | {c['itemErrors'] or ''} |")
    heavy = sorted((c for c in results["cases"] if not c.get("error")), key=lambda c: -c["tokens"])[:5]
    if heavy:
        out += ["", "Most tokens: " + ", ".join(f"{c['id']}@{c['scale'] or '-'} ~{c['tokens']:,}" for c in heavy)]
    if results["seed"]:
        out += ["", "## Writes", "", "| command | up to scale | elements | seconds | per second |", "|---|---|---|---|---|"]
        out += [f"| {s['command']} | {s['scale']} | {s['created']}/{s['asked']} | {s['seconds']} | {s['perSecond']} |" for s in results["seed"]]
    if results["stress"]:
        out += ["", "## Parallel calls", "", "| case | parallel | calls | per second | p50 ms | p95 ms | errors |", "|---|---|---|---|---|---|---|"]
        out += [f"| {s['id']} | {s['concurrency']} | {s['calls']} | {s['perSecond']} | {s['ms']['p50']:.1f} | {s['ms']['p95']:.1f} | {s['errors']} |"
                for s in results["stress"]]
    if results["notes"]:
        out += ["", "## Notes", ""] + [f"- {n}" for n in results["notes"]]
    return "\n".join(out) + "\n"


def compare(old: dict, new: dict, threshold: float = 15.0, min_ms: float = 5.0) -> tuple[list[str], int]:
    """Lines for every case whose time or tokens moved more than threshold %; and how many got worse.
    Times below min_ms of difference are noise and never count."""
    lines, worse = [], 0
    before = {(c["id"], c["scale"]): c for c in old["cases"] if not c.get("error")}
    for c in new["cases"]:
        b = before.get((c["id"], c["scale"]))
        where = f"{c['id']}@{c['scale']}" if c["scale"] is not None else c["id"]
        if c.get("error"):
            lines.append(f"  x {where}: now fails: {c['error']}")
            worse += b is not None
            continue
        if b is None:
            continue
        for label, was, now, floor in (("p50", b["ms"]["p50"], c["ms"]["p50"], min_ms), ("tokens", b["tokens"], c["tokens"], 1)):
            change = (now - was) / max(was, 1e-9) * 100
            if abs(change) > threshold and abs(now - was) >= floor:
                mark = "x" if change > 0 else "+"
                worse += change > 0
                lines.append(f"  {mark} {where} {label} {was:,.0f} -> {now:,.0f} ({change:+.0f}%)")
    return lines, worse


def results_dir() -> Path:
    return r.default_config_path().parent / "benchmarks"


def save(results: dict, folder: Path | None = None) -> Path:
    folder = folder or results_dir()
    folder.mkdir(parents=True, exist_ok=True)
    m = results["meta"]
    stem = f"{m['started'].replace(':', '').replace('-', '')}_AC{m['archicad'].replace(' ', '')}_tapir{m['tapir']}" + ("_readonly" if m.get("readOnly") else "")
    path = folder / f"{stem}.json"
    path.write_text(json.dumps(results, indent=1), encoding="utf-8")
    path.with_suffix(".md").write_text(report(results), encoding="utf-8")
    return path


def previous(path: Path) -> dict | None:
    """The last earlier result of the same Archicad, suite version and mode, to compare a new run with."""
    new = json.loads(path.read_text(encoding="utf-8"))["meta"]
    for other in sorted(path.parent.glob("*.json"), reverse=True):
        if other.name >= path.name:
            continue
        old = json.loads(other.read_text(encoding="utf-8"))
        m = old.get("meta", {})
        if (m.get("archicad"), m.get("suiteVersion"), m.get("readOnly"), m.get("scales")) == (
                new["archicad"], new["suiteVersion"], new["readOnly"], new["scales"]):
            old["_path"] = str(other)
            return old
    return None


def run_and_save(send: Callable[..., dict], suite_path: Path = DEFAULT_SUITE, log: Callable[[str], None] = print,
                 scales: list[int] | None = None, read_only: bool = False, repeat: int | None = None,
                 out: Path | None = None, exact_model: str | None = None, stop: Callable[[], bool] = lambda: False) -> Path:
    """One whole run: measures, writes <time>_AC<v>_tapir<v>.json and .md, logs the change against the last run."""
    suite = json.loads(Path(suite_path).read_text(encoding="utf-8"))
    counter = exact_counter(exact_model) if exact_model else None
    results = Bench(send, suite, log, counter, stop).run(scales, read_only, repeat)
    path = save(results, out)
    log(headline(results))
    old = previous(path)
    if old:
        lines, worse = compare(old, results)
        log(f"against {Path(old['_path']).name}: {worse} worse" + (":" if lines else ", nothing moved more than 15%"))
        for text in lines:
            log(text)
    log(f"report: {path.with_suffix('.md')}")
    return path


def main(args) -> int:
    if args.mode == "bench-compare":
        old, new = (json.loads(Path(p).read_text(encoding="utf-8")) for p in (args.old, args.new))
        lines, worse = compare(old, new, args.threshold)
        print(f"{headline(old)}\n{headline(new)}\n{worse} worse" + ("\n" + "\n".join(lines) if lines else ""))
        return 1 if worse else 0
    port = args.port or r.find_port()
    if port is None:
        raise RuntimeError(f"no Archicad with Tapir answers on ports {r.PORTS.start}-{r.PORTS.stop - 1}")
    scales = [int(s) for s in args.scales.split(",")] if args.scales else ([100] if args.quick else None)
    path = run_and_save(r.Tapir(port), Path(args.suite), print if not args.brief else brief_log, scales, args.read_only,
                        2 if args.quick else None, Path(args.out) if args.out else None,
                        args.model if args.exact_tokens else None)
    errors = sum(1 for c in json.loads(path.read_text(encoding="utf-8"))["cases"] if c.get("error"))
    return 1 if errors else 0


def brief_log(text: str) -> None:
    """--brief: only the headline, errors, the comparison and where the report is."""
    if text.startswith(("tapir-bench:", "against ", "  ", "report:")) or "ERROR" in text:
        print(text)
