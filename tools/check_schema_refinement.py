"""Compare main's schemas with the proposed merge for both schema commands.

Read C++ schemas with generate_addon_docs.py and write a JSON report of changed
validation rules, descriptions, and added definition names.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
from io import BytesIO
import json
from pathlib import Path
import subprocess
import sys
import tarfile
from tempfile import TemporaryDirectory


def run(command, repo, env=None):
    result = subprocess.run(command, cwd=repo, env=env, capture_output=True, timeout=900)
    if result.returncode:
        raise ValueError(result.stderr.decode("utf-8", errors="replace").strip())
    return result.stdout


def git(repo, *args, env=None):
    return run(["git", "-c", "core.hooksPath=/dev/null", *args], repo, env)


ANNOTATIONS = {"title", "description", "$comment", "examples"}
SCHEMA_MAPS = {"properties", "patternProperties", "definitions", "$defs", "dependentSchemas"}
SCHEMA_CHILDREN = {
    "items",
    "additionalItems",
    "additionalProperties",
    "propertyNames",
    "contains",
    "not",
    "if",
    "then",
    "else",
    "unevaluatedItems",
    "unevaluatedProperties",
    "allOf",
    "anyOf",
    "oneOf",
    "prefixItems",
}


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate JSON key {key!r}")
        result[key] = value
    return result


def child_path(path, key):
    return f"{path}/{str(key).replace('~', '~0').replace('/', '~1')}"


def schema_role(key):
    if key in SCHEMA_MAPS:
        return "map"
    return "schema" if key in SCHEMA_CHILDREN else "data"


def target_name(ref, common, path):
    if isinstance(ref, str) and ref.startswith("#/"):
        name = ref.removeprefix("#/$defs/") if ref.startswith("#/$defs/") else ref[2:]
        name = name.replace("~1", "/").replace("~0", "~")
        if name in common:
            return name
    raise ValueError(f"{path}: unresolved or unsupported Tapir reference {ref!r}")


def reference_sites(node, common, path, role="schema"):
    """Find schema references and check that their shared definitions exist."""
    if isinstance(node, dict) and role != "data":
        for key, value in sorted(node.items()):
            subpath = child_path(path, key)
            if key == "$ref" and role == "schema":
                yield target_name(value, common, subpath), subpath
            else:
                yield from reference_sites(value, common, subpath, "schema" if role == "map" else schema_role(key))
    elif isinstance(node, list) and role == "schema":
        for index, value in enumerate(node):
            yield from reference_sites(value, common, child_path(path, index))


def check_references(node, common, path):
    for _ in reference_sites(node, common, path):
        pass


def normalized(schema):
    """Omit annotations and normalize equivalent Tapir object forms."""
    result = {key: value for key, value in schema.items() if key not in ANNOTATIONS}
    if result.get("required") == []:
        del result["required"]
    for keyword in ("oneOf", "anyOf"):
        branches = result.get(keyword)
        # Give each alternative its shared fields and its own required fields.
        if (
            branches
            and result.get("type") == "object"
            and result.keys() <= {keyword, "type", "properties", "required", "additionalProperties"}
            and all(isinstance(branch, dict) and branch.keys() == {"required"} for branch in branches)
        ):
            shared = {key: value for key, value in result.items() if key not in {keyword, "required"}}
            required = result.get("required", [])
            return {
                keyword: [
                    {**shared, "required": list(dict.fromkeys(required + branch["required"]))} for branch in branches
                ]
            }
    return result


def schema_annotations(schema):
    return {key: schema[key] for key in sorted(schema.keys() & ANNOTATIONS)} if isinstance(schema, dict) else {}


def split_reference(schema, common, path, annotations=None):
    """Follow $ref links, keeping rules written beside them."""
    visited = set()
    siblings = []
    while isinstance(schema, dict) and "$ref" in schema:
        if annotations is not None:
            notes = schema_annotations(schema)
            if notes:
                annotations.append(notes)
        keywords = normalized(schema)
        name = target_name(keywords.pop("$ref"), common, path)
        if name in visited:
            raise ValueError(f"{path}: reference cycle without a schema body at {name!r}")
        if keywords:
            siblings.append(keywords)
        visited.add(name)
        schema = common[name]
    return schema, siblings


def differences(before, after, base_defs, merge_defs, path, seen, role="schema", *, annotation_changes=None):
    """Compare rules through references; record annotation changes separately."""
    # Keep the original objects alive because normalization can create temporary branches.
    key = id(before), id(after), role
    if key in seen:
        return
    seen[key] = before, after
    if role == "schema":
        a_notes = [] if annotation_changes is not None else None
        b_notes = [] if annotation_changes is not None else None
        a, a_siblings = split_reference(before, base_defs, path, a_notes)
        b, b_siblings = split_reference(after, merge_defs, path, b_notes)
        if a_notes != b_notes:
            annotation_changes.append(
                {"path": child_path(path, "$ref annotations"), "before": a_notes, "after": b_notes}
            )
        if a_siblings or b_siblings:
            yield from differences(
                a_siblings,
                b_siblings,
                base_defs,
                merge_defs,
                child_path(path, "$ref siblings"),
                seen,
                annotation_changes=annotation_changes,
            )
        if a is not before or b is not after:
            yield from differences(a, b, base_defs, merge_defs, path, seen, annotation_changes=annotation_changes)
            return
        if annotation_changes is not None:
            a_notes, b_notes = schema_annotations(a), schema_annotations(b)
            if a_notes != b_notes:
                annotation_changes.append({"path": path, "before": a_notes, "after": b_notes})
        before = normalized(before) if isinstance(before, dict) else before
        after = normalized(after) if isinstance(after, dict) else after
    if isinstance(before, dict) and isinstance(after, dict):
        for name in sorted(before.keys() - after.keys()):
            yield f"{child_path(path, name)}: missing from target"
        for name in sorted(after.keys() - before.keys()):
            yield f"{child_path(path, name)}: only in target"
        for name in sorted(before.keys() & after.keys()):
            child_role = "schema" if role == "map" else schema_role(name) if role == "schema" else "data"
            yield from differences(
                before[name],
                after[name],
                base_defs,
                merge_defs,
                child_path(path, name),
                seen,
                child_role,
                annotation_changes=annotation_changes,
            )
    elif isinstance(before, list) and isinstance(after, list):
        if len(before) != len(after):
            yield f"{path}: list length changed from {len(before)} to {len(after)}"
        for index, (a, b) in enumerate(zip(before, after)):
            yield from differences(
                a, b, base_defs, merge_defs, child_path(path, index), seen, role, annotation_changes=annotation_changes
            )
    elif type(before) is not type(after) or before != after:
        yield f"{path}: {json.dumps(before)} -> {json.dumps(after)}"


def read_generated(path, variable):
    prefix = f"var {variable} = "
    text = path.read_text(encoding="utf-8").strip()
    if not text.startswith(prefix) or not text.endswith(";"):
        raise ValueError(f"Unexpected generated file format in {path}")
    return json.loads(text[len(prefix) : -1], object_pairs_hook=unique_object)


def snapshot(repo, ref, directory, env):
    """Read a revision's schemas using this checkout's documentation generator."""
    archive = git(repo, "archive", ref, "archicad-addon/Sources", "archicad-addon/devkits.json", env=env)
    with tarfile.open(fileobj=BytesIO(archive)) as tar:
        if any(not (member.isfile() or member.isdir()) for member in tar.getmembers()):
            raise ValueError("Source snapshots must contain only regular files and directories")
        tar.extractall(directory, filter="data")
    tools = directory / "tools"
    tools.mkdir()
    (tools / "generate_addon_docs.py").write_bytes((Path(__file__).parent / "generate_addon_docs.py").read_bytes())
    docs = directory / "docs/archicad-addon"
    docs.mkdir(parents=True)
    run([sys.executable, "-I", "tools/generate_addon_docs.py"], directory)
    common = read_generated(docs / "common_schema_definitions.js", "gSchemaDefinitions")
    groups = read_generated(docs / "command_definitions.js", "gCommands")
    commands = {}
    for group in groups:
        for command in group["commands"]:
            name = command["name"]
            if name in commands:
                raise ValueError(f"Duplicate command {name!r} in {ref}")
            commands[name] = {"input": command["inputScheme"], "response": command["outputScheme"]}
    return common, commands


def check_snapshot(common, commands):
    for name, schema in common.items():
        path = child_path("/definitions", name)
        check_references(schema, common, path)
        split_reference(schema, common, path)
    for name, sides in commands.items():
        for side, schema in sides.items():
            check_references(schema, common, child_path(child_path("/commands", name), side))


def compare(base_snapshot, merge_snapshot):
    """Report changed command schemas, changed rules, or removed shared names."""
    base_defs, base_commands = base_snapshot
    merge_defs, merge_commands = merge_snapshot
    check_snapshot(base_defs, base_commands)
    check_snapshot(merge_defs, merge_commands)
    for name in sorted(base_defs):
        path = child_path("/definitions", name)
        if name not in merge_defs:
            yield f"{path}: missing from merge"
        else:
            yield from differences(base_defs[name], merge_defs[name], base_defs, merge_defs, path, {})
    for name in sorted(base_commands.keys() | merge_commands.keys()):
        path = child_path("/commands", name)
        if name not in base_commands:
            yield f"{path}: added in merge"
        elif name not in merge_commands:
            yield f"{path}: missing from merge"
        else:
            for side in ("input", "response"):
                yield from differences(
                    base_commands[name][side],
                    merge_commands[name][side],
                    base_defs,
                    merge_defs,
                    child_path(path, side),
                    {},
                )


def validate(repo, base, head):
    """Compare base with the proposed merge and return differences and commit IDs."""
    tree = git(repo, "merge-tree", "--write-tree", base, head).decode().splitlines()[0]
    with TemporaryDirectory(prefix="tapir-schema-") as temporary:
        directory = Path(temporary)
        with ThreadPoolExecutor(max_workers=2) as pool:
            before = pool.submit(snapshot, repo, base, directory / "base", None)
            after = pool.submit(snapshot, repo, tree, directory / "candidate", None)
            baseline, candidate = before.result(), after.result()
        changes = list(compare(baseline, candidate))
        annotations = []
        for name in sorted(baseline[0].keys() & candidate[0].keys()):
            list(
                differences(
                    baseline[0][name],
                    candidate[0][name],
                    baseline[0],
                    candidate[0],
                    child_path("/definitions", name),
                    {},
                    annotation_changes=annotations,
                )
            )
        for name in sorted(baseline[1].keys() & candidate[1].keys()):
            for side in ("input", "response"):
                list(
                    differences(
                        baseline[1][name][side],
                        candidate[1][name][side],
                        baseline[0],
                        candidate[0],
                        f"/commands/{name}/{side}",
                        {},
                        annotation_changes=annotations,
                    )
                )
    return {
        "base": base,
        "head": head,
        "tree": tree,
        "commands": len(baseline[1]),
        "definitions": len(baseline[0]),
        "added_definitions": sorted(candidate[0].keys() - baseline[0].keys()),
        "structural_differences": changes,
        "annotation_differences": annotations,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    report = validate(Path.cwd(), args.base, args.head)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Compared {report['commands']} commands and {report['definitions']} existing definitions.")
    for change in report["structural_differences"]:
        print(change)
    return int(bool(report["structural_differences"]))


if __name__ == "__main__":
    sys.exit(main())
