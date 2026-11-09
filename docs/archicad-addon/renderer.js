/* The command reference (index.html): a compact list of every command, grouped and collapsed, with a
   search, chips to filter by action and subject, the Added-in filter, and an opened command that
   shows its example, the JSON to send and readable parameter tables. The data comes from
   command_definitions.js (gCommands) and common_schema_definitions.js (gSchemaDefinitions), which
   tools/generate_addon_docs.py writes; nothing here is generated. */

function CreateElement(parentElement, elementType, className, innerHTML) {
    let elem = document.createElement(elementType);
    if (className) {
        elem.className = className;
    }
    if (innerHTML) {
        elem.innerHTML = innerHTML;
    }
    if (parentElement) {
        parentElement.appendChild(elem);
    }
    return elem;
}

function CreateTextElement(parentElement, elementType, className, text) {
    let elem = CreateElement(parentElement, elementType, className, null);
    elem.textContent = text;
    return elem;
}

function PlainText(html) {
    // Descriptions may contain markup; DOMParser builds an inert document.
    return new DOMParser().parseFromString(html || '', 'text/html').body.textContent;
}

// ---- The raw schema tree (json-schema-view-js), shown under "Raw schema" ----

function ResolveReferences(schemaDefinitions, parentNode, parentKey, resolvedKeys) {
    let node = parentNode[parentKey];
    for (let key in node) {
        if (!node.hasOwnProperty(key)) {
            continue;
        }
        let childNode = node[key];
        if (typeof childNode === 'object') {
            ResolveReferences(schemaDefinitions, node, key, resolvedKeys);
        } else if (typeof childNode === 'string' && key == '$ref') {
            let refKey = childNode.substr(2);
            let refValue = JSON.parse(JSON.stringify(schemaDefinitions[refKey]));
            if (resolvedKeys.has(refKey)) {
                parentNode[parentKey] = {
                    type: refKey,
                    description: refValue.description
                };
            } else {
                parentNode[parentKey] = refValue;
                parentNode[parentKey].title = refKey;
                resolvedKeys.add(refKey);
                ResolveReferences(schemaDefinitions, parentNode, parentKey, resolvedKeys);
                resolvedKeys.delete(refKey);
            }
            let descriptions = [parentNode[parentKey].description, node.description]
                .filter(description => typeof description === 'string' && description.length > 0);
            if (descriptions.length > 0) {
                parentNode[parentKey].description = [...new Set(descriptions)].join('\n\n');
            }
        }
    }
}

function EnsureRenderableSchemas(node, visitedNodes) {
    if (node === null || typeof node !== 'object' || visitedNodes.has(node)) {
        return;
    }
    visitedNodes.add(node);
    if (Array.isArray(node.required) && typeof node.properties !== 'object') {
        // JSONSchemaView reads schema.properties[name] for every required name and
        // throws when properties is missing (e.g. a oneOf branch that only lists
        // required fields), which would abort rendering of all commands after it.
        node.properties = {};
    }
    for (let key in node) {
        if (node.hasOwnProperty(key)) {
            EnsureRenderableSchemas(node[key], visitedNodes);
        }
    }
}

function CreateSchemaElement(parentElement, title, schema, schemaDefinitions) {
    if (schema === null) {
        return;
    }
    CreateElement(parentElement, 'div', 'scheme_title', title);
    let schemeContainer = CreateElement(parentElement, 'div', 'scheme_container', null);
    let resolvedObject = {
        schema: JSON.parse(JSON.stringify(schema))
    };
    ResolveReferences(schemaDefinitions, resolvedObject, 'schema', new Set());
    EnsureRenderableSchemas(resolvedObject.schema, new Set());
    let view = new JSONSchemaView(resolvedObject.schema);
    schemeContainer.appendChild(view.render());
}

// ---- Groups, versions, actions and subjects ----

function GroupId(groupName) {
    return 'group-' + groupName.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
}

function VersionKey(version) {
    // "1.7.2" -> [1, 7, 2], so versions compare numerically.
    return String(version).split('.').map(part => parseInt(part, 10) || 0);
}

function CompareVersions(a, b) {
    let keyA = VersionKey(a);
    let keyB = VersionKey(b);
    for (let i = 0; i < Math.max(keyA.length, keyB.length); i++) {
        let diff = (keyA[i] || 0) - (keyB[i] || 0);
        if (diff !== 0) {
            return diff;
        }
    }
    return 0;
}

function MinorVersion(version) {
    return VersionKey(version).slice(0, 2).join('.');
}

// The words of a command name: "GetIFCRelationsOfElements" -> Get, IFC, Relations, Of, Elements.
function NameWords(name) {
    return name.match(/[A-Z]+(?![a-z])|[A-Z][a-z0-9]*|[a-z0-9]+/g) || [name];
}

// What a command does, from the first word of its name, in the order the big groups list them.
var gActions = [
    { key: 'get', label: 'Get', verbs: ['Get', 'Find', 'List', 'Is', 'Has', 'Search'] },
    { key: 'create', label: 'Create', verbs: ['Create', 'Add', 'Place', 'Import'] },
    { key: 'change', label: 'Change', verbs: ['Set', 'Modify', 'Change', 'Rename', 'Move', 'Update', 'Apply', 'Convert'] },
    { key: 'delete', label: 'Delete', verbs: ['Delete', 'Remove', 'Clear'] },
    { key: 'other', label: 'Other', verbs: [] }
];

function CommandAction(name) {
    let first = NameWords(name)[0];
    for (let action of gActions) {
        if (action.verbs.includes(first)) {
            return action.key;
        }
    }
    return 'other';
}

// The subjects people look for most. A command has a subject when it is in one of its groups, or a
// word of its name matches name, or its description has a word matching description.
var gSubjects = [
    { key: 'elements', label: 'Elements', groups: ['Element Commands', 'Element grouping Commands'], name: /^elements?$/i, description: /\belements?\b/i },
    { key: 'walls', label: 'Walls', groups: [], name: /^walls?$/i, description: /\bwalls?\b/i },
    { key: 'slabs', label: 'Slabs', groups: [], name: /^slabs?$/i, description: /\bslabs?\b/i },
    { key: 'zones', label: 'Zones', groups: [], name: /^zones?$/i, description: /\bzones?\b/i },
    { key: 'properties', label: 'Properties', groups: ['Property Commands'], name: /^propert(y|ies)$/i, description: /\bpropert(y|ies)\b/i },
    { key: 'classifications', label: 'Classifications', groups: ['Classification Commands'], name: /^classifications?$/i, description: /\bclassifications?\b/i },
    { key: 'attributes', label: 'Attributes', groups: ['Attribute Commands'], name: /^attributes?$/i, description: /\battributes?\b/i },
    { key: 'layers', label: 'Layers', groups: [], name: /^layers?$/i, description: /\blayers?\b/i },
    { key: 'views', label: 'Views and layouts', groups: ['Navigator Commands'], name: /^(views?|viewpoints?|layouts?|layouts?book)$/i, description: /\b(views?|viewpoints?|layouts?)\b/i },
    { key: 'library', label: 'Library parts', groups: ['Library Commands'], name: /^(library|libpart|libparts|gdl)$/i, description: /\blibrary parts?\b/i },
    { key: 'issues', label: 'Issues', groups: ['Issue Management Commands'], name: /^issues?$/i, description: /\bissues?\b/i },
    { key: 'teamwork', label: 'Teamwork', groups: ['Teamwork Commands'], name: /^teamwork$/i, description: /\bteamwork\b/i },
    { key: 'ifc', label: 'IFC', groups: ['IFC Commands'], name: /^ifc$/i, description: /\bifc\b/i },
    { key: 'mep', label: 'MEP', groups: ['MEP Commands', 'MEP Commands (extended)'], name: /^mep$/i, description: /\bmep\b/i }
];

function CommandSubjects(groupName, command, descriptionText) {
    let words = NameWords(command.name);
    let subjects = new Set();
    for (let subject of gSubjects) {
        if (subject.groups.includes(groupName) ||
            words.some(word => subject.name.test(word)) ||
            subject.description.test(descriptionText)) {
            subjects.add(subject.key);
        }
    }
    return subjects;
}

// "MEP Commands" and "MEP Commands (extended)" are one group on the page; the groups registered in
// AddOnMain.cpp do not change.
function DisplayGroupName(groupName) {
    return groupName === 'MEP Commands (extended)' ? 'MEP Commands' : groupName;
}

function DisplayGroupLabel(groupName) {
    return groupName.replace(/ Commands$/, '');
}

// Groups with more commands than this get sub-headings by action.
var gSplitGroupSize = 40;

// The noun a command is about, for "Related": GetZones -> zone, GetDetailsOfElements -> detail.
function CommandNoun(name) {
    let words = NameWords(name);
    if (words.length < 2) {
        return '';
    }
    let noun = words[1].toLowerCase();
    if (/ies$/.test(noun)) {
        return noun.slice(0, -3) + 'y';
    }
    if (/[^s]s$/.test(noun)) {
        return noun.slice(0, -1);
    }
    return noun;
}

// ---- The example ----

// The example of a command is archicad-addon/Examples/<GroupFolder>/<CommandName>.py on main.
// It is fetched when the command is first opened, so nothing of it is copied into the docs.
var gExampleRawBaseUrl = 'https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/main/archicad-addon/Examples/';
var gExampleViewBaseUrl = 'https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/archicad-addon/Examples/';
// The same as GROUPS_WITHOUT_EXAMPLES in tools/check_examples.py.
var gGroupsWithoutExamples = ['Developer Commands'];

function ExampleFolder(groupName) {
    // The Examples subfolder of a command group, the same rule as ExampleFolder in
    // tools/check_examples.py: "Custom Data Commands" -> "CustomData", "MEP Commands (extended)"
    // -> "MEP", "Script UI Commands" -> "ScriptUI". The parenthesised part and a trailing
    // "Commands" are dropped, the words are joined in PascalCase, all-caps words stay as they are.
    let name = groupName.replace(/\s*\([^)]*\)/g, '').trim().replace(/\s+Commands$/, '');
    return name.split(/\s+/).filter(word => word.length > 0)
        .map(word => word === word.toUpperCase() ? word : word.charAt(0).toUpperCase() + word.substring(1))
        .join('');
}

function ExamplePath(groupName, commandName) {
    return encodeURIComponent(ExampleFolder(groupName)) + '/' + encodeURIComponent(commandName) + '.py';
}

function CleanDocstring(text) {
    // The same as Python's inspect.cleandoc: the first line is stripped, the common
    // indentation of the other lines is removed, blank lines at both ends are dropped.
    let lines = text.replace(/\r\n?/g, '\n').split('\n');
    let indents = lines.slice(1).filter(line => line.trim().length > 0).map(line => line.match(/^[ \t]*/)[0].length);
    let indent = indents.length > 0 ? Math.min(...indents) : 0;
    lines = [lines[0].trim()].concat(lines.slice(1).map(line => line.substring(indent).trimEnd()));
    while (lines.length > 0 && lines[0] === '') lines.shift();
    while (lines.length > 0 && lines[lines.length - 1] === '') lines.pop();
    return lines.join('\n');
}

function SplitExampleScript(source) {
    // The leading module docstring is the description; the rest is the code.
    let match = source.match(/^﻿?(?:[ \t]*(?:#[^\n]*)?\r?\n)*[ \t]*[rRuU]?("""|''')([\s\S]*?)\1[ \t]*(?:\r?\n|$)/);
    if (!match) {
        return { description: '', code: source.replace(/^\s*\n/, '').trimEnd() };
    }
    let code = source.substring(match[0].length).replace(/^(?:[ \t]*\r?\n)+/, '').trimEnd();
    return { description: CleanDocstring(match[2]), code: code };
}

function RenderCode(parentElement, code, language) {
    // site.js adds a Copy button to every <pre> in <main>, also to the ones added later.
    let codeElement = CreateElement(CreateElement(parentElement, 'pre', null, null), 'code', 'hljs language-' + language, null);
    if (typeof hljs !== 'undefined' && hljs.getLanguage(language)) {
        // highlight () escapes the source, so the markup it returns is safe to insert.
        codeElement.innerHTML = hljs.highlight(code, { language: language, ignoreIllegals: true }).value;
    } else {
        codeElement.textContent = code;
    }
    return codeElement;
}

function RenderExampleScript(container, examplePath, source) {
    let example = SplitExampleScript(source);
    for (let paragraph of example.description.split(/\n[ \t]*\n/)) {
        if (paragraph.trim().length > 0) {
            CreateTextElement(container, 'p', 'command_example_description', paragraph.replace(/\s*\n\s*/g, ' '));
        }
    }
    RenderCode(container, example.code, 'python');
}

function LoadExample(container, examplePath) {
    let status = CreateTextElement(container, 'div', 'command_example_status', 'Loading the example…');
    fetch(gExampleRawBaseUrl + examplePath, { cache: 'no-cache' })
        .then(function (response) {
            if (response.status === 404) {
                status.textContent = 'No example yet.';
                return;
            }
            if (!response.ok) {
                throw new Error('HTTP ' + response.status);
            }
            return response.text().then(function (source) {
                status.remove();
                RenderExampleScript(container, examplePath, source);
            });
        })
        .catch(function (error) {
            status.textContent = 'Could not load the example (' + error.message + ').';
        });
}

// ---- Schemas as tables ----

function RefName(schema) {
    if (schema && typeof schema.$ref === 'string' && schema.$ref.indexOf('#/') === 0) {
        return schema.$ref.substring(2);
    }
    return null;
}

function Definition(name) {
    return (typeof gSchemaDefinitions !== 'undefined' && gSchemaDefinitions[name]) || null;
}

// The schema itself, or the definition it refers to (one level).
function Resolved(schema) {
    let name = RefName(schema);
    return name && Definition(name) ? Definition(name) : schema;
}

// The innermost item of an array (of arrays): the schema whose fields are listed under it.
function ArrayItem(schema) {
    let depth = 0;
    let current = schema;
    while (current && depth < 8) {
        let resolved = Resolved(current);
        if (!resolved || !resolved.items || (resolved.type && resolved.type !== 'array')) {
            break;
        }
        current = resolved.items;
        depth++;
    }
    return current;
}

function TypeText(schema, depth) {
    depth = depth || 0;
    if (!schema || typeof schema !== 'object' || depth > 6) {
        return 'any';
    }
    let refName = RefName(schema);
    if (refName) {
        let definition = Definition(refName);
        if (!definition) {
            return refName;
        }
        if (definition.type === 'array' || definition.items) {
            return refName + ' (' + TypeText(definition, depth + 1) + ')';
        }
        if (definition.type && definition.type !== 'object' && !definition.properties) {
            return refName + ' (' + TypeText(definition, depth + 1) + ')';
        }
        return refName;
    }
    if (schema.type === 'array' || (!schema.type && schema.items)) {
        return 'array of ' + (schema.items ? TypeText(schema.items, depth + 1) : 'any');
    }
    let branches = schema.oneOf || schema.anyOf;
    if (Array.isArray(branches)) {
        return 'one of: ' + branches.map(branch => TypeText(branch, depth + 1)).join(' | ');
    }
    if (Array.isArray(schema.allOf)) {
        return schema.allOf.map(branch => TypeText(branch, depth + 1)).join(' and ');
    }
    if (Array.isArray(schema.type)) {
        return schema.type.join(' or ');
    }
    if (schema.type === 'string' && schema.format) {
        return 'string (' + schema.format + ')';
    }
    if (schema.type) {
        return schema.type;
    }
    if (schema.properties) {
        return 'object';
    }
    if (schema.enum) {
        return 'enum';
    }
    return 'any';
}

function ValueText(value) {
    return typeof value === 'string' ? value : JSON.stringify(value);
}

// The description of a field, with its enum values and limits; a $ref's own description wins over
// the definition's, as in the expanded schemas.
function DescriptionText(schema) {
    let resolved = Resolved(schema) || {};
    let item = Resolved(ArrayItem(schema)) || {};
    let parts = [];
    let description = (schema && schema.description) || resolved.description;
    if (description) {
        parts.push(description);
    }
    let values = resolved.enum || (item !== resolved ? item.enum : null);
    if (Array.isArray(values)) {
        parts.push('One of: ' + values.map(ValueText).join(', ') + '.');
    }
    if (resolved.const !== undefined) {
        parts.push('Always ' + ValueText(resolved.const) + '.');
    }
    let limits = [];
    let sources = [schema || {}, resolved];
    function Limit(key, label) {
        for (let source of sources) {
            if (source[key] !== undefined) {
                limits.push(label + ' ' + ValueText(source[key]));
                return;
            }
        }
    }
    Limit('minimum', 'minimum');
    Limit('exclusiveMinimum', 'greater than');
    Limit('maximum', 'maximum');
    Limit('exclusiveMaximum', 'less than');
    Limit('minLength', 'minimum length');
    Limit('minItems', 'minimum items');
    Limit('maxItems', 'maximum items');
    Limit('default', 'default');
    if (limits.length > 0) {
        let text = limits.join(', ');
        parts.push(text.charAt(0).toUpperCase() + text.substring(1) + '.');
    }
    return parts.join(' ');
}

// True when a schema (or the item of an array, or a definition) has fields to list under it.
function HasFields(schema, depth) {
    depth = depth || 0;
    if (!schema || depth > 6) {
        return false;
    }
    let item = ArrayItem(schema);
    let resolved = Resolved(item);
    if (!resolved) {
        return false;
    }
    if (resolved !== item) {
        return HasFields(resolved, depth + 1);
    }
    return (resolved.properties && Object.keys(resolved.properties).length > 0) ||
        Array.isArray(resolved.oneOf) || Array.isArray(resolved.anyOf) || Array.isArray(resolved.allOf);
}

var gRowCounter = 0;

// A table of the fields of a schema. A field whose type is a shared definition shows its name and
// opens in place: its rows are built when it is first opened. Nested fields are named parent.child.
function SchemaTable(parentElement, rootSchema, options) {
    options = options || {};
    let wrapper = CreateElement(parentElement, 'div', 'params_wrapper', null);
    let table = CreateElement(wrapper, 'table', 'params', null);
    let head = CreateElement(CreateElement(table, 'thead', null, null), 'tr', null, null);
    CreateTextElement(head, 'th', 'params_name', 'Name');
    CreateTextElement(head, 'th', 'params_type', 'Type');
    let requiredHead = CreateTextElement(head, 'th', 'params_required', 'Req.');
    requiredHead.title = 'Required';
    CreateTextElement(head, 'th', 'params_description', 'Description');
    let tbody = CreateElement(table, 'tbody', null, null);

    // ctx: { owner: id of the nearest field opened in place, owners: all of them, refs: definitions above }
    // label: what the name column shows; path: the name the fields under it start with.
    function AddRow(rows, label, path, schema, required, level, ctx, isOption) {
        let row = document.createElement('tr');
        row.dataset.owner = ctx.owner;
        row.dataset.owners = ctx.owners;
        let nameCell = CreateElement(row, 'td', 'params_name', null);
        nameCell.style.paddingLeft = (0.6 + level * 1.0) + 'em';
        if (isOption) {
            CreateTextElement(nameCell, 'span', 'params_option', label);
        } else {
            CreateTextElement(nameCell, 'code', null, label);
        }
        let typeCell = CreateElement(row, 'td', 'params_type', null);
        let requiredCell = CreateTextElement(row, 'td', 'params_required', required ? '✓' : '');
        if (required) {
            requiredCell.title = 'Required';
        }
        CreateTextElement(row, 'td', 'params_description', DescriptionText(schema));
        rows.push(row);

        let item = ArrayItem(schema);
        let refName = RefName(item);
        if (refName && Definition(refName) && HasFields(Definition(refName)) && !ctx.refs.includes(refName)) {
            // A shared definition: its name, opened in place.
            let button = CreateElement(typeCell, 'button', 'params_ref', null);
            button.type = 'button';
            button.setAttribute('aria-expanded', 'false');
            button.title = 'Show the fields of ' + refName;
            CreateTextElement(button, 'span', 'params_ref_text', TypeText(schema));
            let id = 'r' + (++gRowCounter);
            row.dataset.id = id;
            let built = false;
            button.addEventListener('click', function () {
                let open = button.getAttribute('aria-expanded') !== 'true';
                if (open && !built) {
                    built = true;
                    let childRows = [];
                    let childCtx = { owner: id, owners: ctx.owners + ' ' + id, refs: ctx.refs.concat([refName]) };
                    AddFields(childRows, path, Definition(refName), level + 1, childCtx);
                    let next = row.nextSibling;
                    for (let childRow of childRows) {
                        tbody.insertBefore(childRow, next);
                    }
                }
                button.setAttribute('aria-expanded', String(open));
                for (let other of tbody.querySelectorAll('tr')) {
                    if (open) {
                        if (other.dataset.owner === id) {
                            other.hidden = false;
                        }
                    } else if ((' ' + other.dataset.owners + ' ').indexOf(' ' + id + ' ') >= 0) {
                        other.hidden = true;
                        let nested = other.querySelector('button.params_ref');
                        if (nested) {
                            nested.setAttribute('aria-expanded', 'false');
                        }
                    }
                }
            });
        } else {
            CreateTextElement(typeCell, 'span', null, TypeText(schema));
            if (!refName || !Definition(refName)) {
                AddFields(rows, path, item, level + 1, ctx);
            }
        }
        return row;
    }

    function AddFields(rows, path, schema, level, ctx) {
        let resolved = Resolved(schema);
        if (!resolved || typeof resolved !== 'object') {
            return;
        }
        if (resolved !== schema) {
            // An inline field never refers to a definition here (AddRow opens those in place),
            // except the root of a table and the branches of a oneOf.
            let refName = RefName(schema);
            if (ctx.refs.includes(refName)) {
                return;
            }
            ctx = { owner: ctx.owner, owners: ctx.owners, refs: ctx.refs.concat([refName]) };
        }
        let properties = resolved.properties || {};
        let required = Array.isArray(resolved.required) ? resolved.required : [];
        for (let key of Object.keys(properties)) {
            if (level === 0 && options.skipKeys && options.skipKeys.includes(key)) {
                continue;
            }
            let keyPath = path ? path + '.' + key : key;
            AddRow(rows, keyPath, keyPath, properties[key], required.includes(key), level, ctx, false);
        }
        let branches = resolved.oneOf || resolved.anyOf;
        if (Array.isArray(branches)) {
            branches.forEach(function (branch, index) {
                if (level === 0 && options.skipBranch && options.skipBranch(branch)) {
                    return;
                }
                let label = (resolved.oneOf ? 'one of, ' : 'any of, ') + 'option ' + (index + 1);
                AddRow(rows, label, path, branch, false, level, ctx, true);
            });
        }
        if (Array.isArray(resolved.allOf)) {
            for (let part of resolved.allOf) {
                AddFields(rows, path, part, level, ctx);
            }
        }
    }

    let rows = [];
    let rootItem = rootSchema;
    let root = Resolved(rootSchema);
    let ctx = { owner: '', owners: '', refs: [] };
    if (root && (root.properties || root.oneOf || root.anyOf || root.allOf)) {
        AddFields(rows, '', rootItem, 0, ctx);
    } else if (root) {
        // Not an object: one row for the whole value.
        AddRow(rows, '(the value)', '', rootSchema, true, 0, ctx, true);
    }
    for (let row of rows) {
        tbody.appendChild(row);
    }
    if (rows.length === 0) {
        wrapper.remove();
        return null;
    }
    return table;
}

// A value of a schema built from its required fields, for "JSON to send".
function SampleValue(schema, refs, depth) {
    if (!schema || typeof schema !== 'object' || depth > 12) {
        return null;
    }
    let refName = RefName(schema);
    if (refName) {
        if (refs.includes(refName) || !Definition(refName)) {
            return null;
        }
        return SampleValue(Definition(refName), refs.concat([refName]), depth + 1);
    }
    if (schema.const !== undefined) {
        return schema.const;
    }
    if (Array.isArray(schema.enum) && schema.enum.length > 0) {
        return schema.enum[0];
    }
    if (schema.default !== undefined) {
        return schema.default;
    }
    let branches = schema.oneOf || schema.anyOf;
    if (Array.isArray(branches) && branches.length > 0) {
        return SampleValue(branches[0], refs, depth + 1);
    }
    if (Array.isArray(schema.allOf)) {
        let merged = {};
        for (let part of schema.allOf) {
            let value = SampleValue(part, refs, depth + 1);
            if (value && typeof value === 'object' && !Array.isArray(value)) {
                Object.assign(merged, value);
            }
        }
        return merged;
    }
    let type = Array.isArray(schema.type) ? schema.type[0] : schema.type;
    if (type === 'array' || (!type && schema.items)) {
        let item = SampleValue(schema.items, refs, depth + 1);
        return item === null ? [] : [item];
    }
    if (type === 'object' || (!type && schema.properties)) {
        let result = {};
        let properties = schema.properties || {};
        for (let key of (Array.isArray(schema.required) ? schema.required : [])) {
            if (properties[key]) {
                result[key] = SampleValue(properties[key], refs, depth + 1);
            }
        }
        return result;
    }
    if (type === 'string') {
        return schema.format === 'uuid' ? '00000000-0000-0000-0000-000000000000' : '';
    }
    if (type === 'number' || type === 'integer') {
        if (schema.minimum !== undefined) {
            return schema.minimum;
        }
        if (schema.exclusiveMinimum !== undefined) {
            return schema.exclusiveMinimum + 1;
        }
        return 0;
    }
    if (type === 'boolean') {
        return false;
    }
    return null;
}

function RequestJson(command) {
    let parameters = command.inputScheme ? SampleValue(command.inputScheme, [], 0) : null;
    let body = {
        command: 'API.ExecuteAddOnCommand',
        parameters: {
            addOnCommandId: { commandNamespace: 'TapirCommand', commandName: command.name },
            addOnCommandParameters: parameters && typeof parameters === 'object' ? parameters : {}
        }
    };
    return JSON.stringify(body, null, 2);
}

// The standard result wrapper, left out of "Returns".
var gResultWrappers = ['ExecutionResult', 'SuccessfulExecutionResult', 'FailedExecutionResult'];

function RenderReturns(parentElement, outputScheme) {
    CreateTextElement(parentElement, 'h4', 'command_section_title', 'Returns');
    let wrapperLine = 'Returns <code>{"success": true}</code>, or <code>{"success": false, "error": {…}}</code> when it fails.';
    let failureLine = 'Or <code>{"success": false, "error": {…}}</code> when it fails.';
    if (!outputScheme) {
        CreateTextElement(parentElement, 'p', 'command_returns_line', 'Nothing.');
        return;
    }
    if (gResultWrappers.includes(RefName(outputScheme))) {
        CreateElement(parentElement, 'p', 'command_returns_line', wrapperLine);
        return;
    }
    let root = Resolved(outputScheme) || {};
    let properties = root.properties || {};
    let hasSuccess = properties.success !== undefined;
    let branches = root.oneOf || root.anyOf || [];
    let hasFailureBranch = branches.some(branch => gResultWrappers.includes(RefName(branch)));
    let table = SchemaTable(parentElement, outputScheme, {
        skipKeys: hasSuccess ? ['success', 'error'] : [],
        skipBranch: branch => gResultWrappers.includes(RefName(branch))
    });
    if (hasSuccess) {
        CreateElement(parentElement, 'p', 'command_returns_line', table ? 'Also ' + wrapperLine.charAt(0).toLowerCase() + wrapperLine.substring(1) : wrapperLine);
    } else if (hasFailureBranch) {
        CreateElement(parentElement, 'p', 'command_returns_line', failureLine);
    } else if (!table) {
        CreateTextElement(parentElement, 'p', 'command_returns_line', 'Nothing.');
    }
}

// ---- One command ----

function CommandUrl(name) {
    return window.location.pathname + window.location.search + '#' + name;
}

function RenderCommandContent(parentElement, entry, page) {
    let command = entry.command;
    let content = CreateElement(parentElement, 'div', 'command_content', null);
    CreateElement(content, 'div', 'command_description', command.description);

    // The example and the JSON to send, first.
    CreateTextElement(content, 'h4', 'command_section_title', 'Example');
    if (!gGroupsWithoutExamples.includes(entry.groupName)) {
        let examplePath = ExamplePath(entry.groupName, command.name);
        let exampleBody = CreateElement(content, 'div', 'command_example_body', null);
        LoadExample(exampleBody, examplePath);
        // A plain link to the file, which works even when the example cannot be fetched.
        let exampleFile = CreateTextElement(content, 'div', 'command_example_file', 'Example file: ');
        let exampleLink = CreateTextElement(exampleFile, 'a', null,
            'archicad-addon/Examples/' + ExampleFolder(entry.groupName) + '/' + command.name + '.py');
        exampleLink.href = gExampleViewBaseUrl + examplePath;
        exampleLink.target = '_blank';
        exampleLink.rel = 'noopener';
    }
    CreateTextElement(content, 'div', 'command_json_title', 'JSON to send to http://127.0.0.1:19723 (the required fields):');
    try {
        RenderCode(content, RequestJson(command), 'json');
    } catch (error) {
        CreateTextElement(content, 'div', 'command_example_status', 'Could not build the JSON of this command.');
        console.error('Failed to build the JSON of ' + command.name, error);
    }

    try {
        CreateTextElement(content, 'h4', 'command_section_title', 'Parameters');
        if (!command.inputScheme || !SchemaTable(content, command.inputScheme)) {
            CreateTextElement(content, 'p', 'command_returns_line', 'None.');
        }
        RenderReturns(content, command.outputScheme);
    } catch (error) {
        // One broken schema must not break the rest of the page.
        CreateTextElement(content, 'div', 'command_description', 'Failed to render the parameters of this command.');
        console.error('Failed to render command ' + command.name, error);
    }

    // The schema tree as it was, built when it is opened.
    let raw = CreateElement(content, 'details', 'command_raw', null);
    CreateTextElement(raw, 'summary', null, 'Raw schema');
    let rawBody = CreateElement(raw, 'div', null, null);
    let rawRendered = false;
    raw.addEventListener('toggle', function () {
        if (raw.open && !rawRendered) {
            rawRendered = true;
            try {
                CreateSchemaElement(rawBody, 'Input parameters', command.inputScheme, gSchemaDefinitions);
                CreateSchemaElement(rawBody, 'Output parameters', command.outputScheme, gSchemaDefinitions);
            } catch (error) {
                CreateTextElement(rawBody, 'div', 'command_description', 'Failed to render the schemas of this command.');
                console.error('Failed to render the schemas of ' + command.name, error);
            }
        }
    });

    let related = page.Related(entry);
    if (related.length > 0) {
        let relatedLine = CreateElement(content, 'div', 'command_related', null);
        CreateTextElement(relatedLine, 'span', 'command_related_label', 'Related: ');
        related.forEach(function (other, index) {
            if (index > 0) {
                relatedLine.appendChild(document.createTextNode(', '));
            }
            let link = CreateTextElement(relatedLine, 'a', null, other.command.name);
            link.href = '#' + other.command.name;
            link.addEventListener('click', function (event) {
                event.preventDefault();
                page.OpenCommand(other.command.name, true);
            });
        });
    }
}

var gLinkIcon = '<svg viewBox="0 0 16 16" width="14" height="14" aria-hidden="true" focusable="false"><path fill="currentColor" d="M7.78 3.03a3.25 3.25 0 0 1 4.6 4.6l-1.6 1.59a.75.75 0 1 1-1.06-1.06l1.6-1.6a1.75 1.75 0 0 0-2.48-2.47l-1.6 1.6A.75.75 0 1 1 6.19 4.6zM9.53 5.47a.75.75 0 0 1 0 1.06l-3 3a.75.75 0 0 1-1.06-1.06l3-3a.75.75 0 0 1 1.06 0M5.28 7.72a.75.75 0 0 1 0 1.06l-1.6 1.6a1.75 1.75 0 0 0 2.48 2.47l1.6-1.6a.75.75 0 1 1 1.06 1.06l-1.6 1.6a3.25 3.25 0 0 1-4.6-4.6l1.6-1.59a.75.75 0 0 1 1.06 0"/></svg>';

function RenderCommand(entry, page) {
    let command = entry.command;
    let details = document.createElement('details');
    details.className = 'command_container';
    details.id = command.name;
    let header = CreateElement(details, 'summary', 'command_header', null);
    CreateTextElement(header, 'span', 'command_name', command.name);
    let link = CreateElement(header, 'a', 'command_link', gLinkIcon);
    link.href = '#' + command.name;
    link.title = 'Copy the link to ' + command.name;
    link.setAttribute('aria-label', 'Copy the link to ' + command.name);
    link.addEventListener('click', function (event) {
        // Inside the summary: copy the link, do not open or close the command.
        event.preventDefault();
        event.stopPropagation();
        let url = new URL(CommandUrl(command.name), window.location.href).href;
        let done = function () {
            link.classList.add('copied');
            link.title = 'Link copied';
            setTimeout(function () {
                link.classList.remove('copied');
                link.title = 'Copy the link to ' + command.name;
            }, 1500);
        };
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(url).then(done, function () {
                history.replaceState(null, '', CommandUrl(command.name));
            });
        } else {
            history.replaceState(null, '', CommandUrl(command.name));
        }
    });
    // One line of the description; the full text shows on hover and when opened.
    let summary = CreateTextElement(header, 'span', 'command_summary', entry.descriptionText);
    summary.title = entry.descriptionText;
    CreateTextElement(header, 'span', 'command_version', command.version);

    let rendered = false;
    details.addEventListener('toggle', function () {
        if (details.open && !rendered) {
            rendered = true;
            try {
                RenderCommandContent(details, entry, page);
            } catch (error) {
                CreateTextElement(details, 'div', 'command_description', 'Failed to render this command.');
                console.error('Failed to render command ' + command.name, error);
            }
        }
        if (details.dataset.bulkToggle) {
            // Opened or closed by "Collapse all": leave the address alone.
            delete details.dataset.bulkToggle;
            return;
        }
        // The address names the opened command, without adding history entries.
        if (details.open) {
            history.replaceState(null, '', CommandUrl(command.name));
        } else if (window.location.hash === '#' + command.name) {
            history.replaceState(null, '', window.location.pathname + window.location.search);
        }
    });
    return details;
}

// ---- The page ----

// Elements a jump scrolls past are shown at once: motion.js reveals elements as they come into view
// and would otherwise leave them blank above the target until it animates them in.
function RevealUpTo(target) {
    for (let element of document.querySelectorAll('.rv:not(.in)')) {
        if (element.contains(target) || (element.compareDocumentPosition(target) & Node.DOCUMENT_POSITION_FOLLOWING)) {
            element.style.transition = 'none';
            element.classList.add('in');
        }
    }
}

function ScrollToElement(element) {
    RevealUpTo(element);
    // Instant, not site.css's smooth scrolling: a smooth scroll over thousands of pixels passes
    // elements still waiting to reveal. (behavior: 'instant' throws in older browsers.)
    let root = document.documentElement;
    let previous = root.style.scrollBehavior;
    root.style.scrollBehavior = 'auto';
    element.scrollIntoView({ block: 'start' });
    root.style.scrollBehavior = previous;
}

function BuildGroups(commands) {
    let groups = [];
    let byName = new Map();
    for (let group of commands) {
        let name = DisplayGroupName(group.name);
        let displayGroup = byName.get(name);
        if (!displayGroup) {
            displayGroup = { name: name, label: DisplayGroupLabel(name), id: GroupId(name), entries: [], open: false, userOpen: false };
            byName.set(name, displayGroup);
            groups.push(displayGroup);
        }
        for (let command of group.commands) {
            let descriptionText = PlainText(command.description);
            displayGroup.entries.push({
                command: command,
                groupName: group.name,
                group: displayGroup,
                descriptionText: descriptionText,
                searchText: (command.name + ' ' + descriptionText + ' ' + group.name).toLowerCase(),
                action: CommandAction(command.name),
                subjects: CommandSubjects(group.name, command, descriptionText),
                noun: CommandNoun(command.name),
                element: null
            });
        }
    }
    return groups;
}

function RenderChips(parentElement, label, items, onChange) {
    let row = CreateElement(parentElement, 'div', 'chips', null);
    row.setAttribute('role', 'group');
    row.setAttribute('aria-label', label);
    CreateTextElement(row, 'span', 'chips_label', label);
    let buttons = [];
    let state = { value: '', buttons: buttons };
    for (let item of items) {
        let button = CreateTextElement(row, 'button', 'chip', item.label);
        button.type = 'button';
        button.setAttribute('aria-pressed', 'false');
        button.addEventListener('click', function () {
            // A second click clears it.
            state.Set(state.value === item.key ? '' : item.key);
            onChange();
        });
        button.dataset.key = item.key;
        buttons.push(button);
    }
    state.Set = function (value) {
        state.value = value;
        for (let button of buttons) {
            button.setAttribute('aria-pressed', String(button.dataset.key === value));
        }
    };
    return state;
}

function RenderCommands(parentElement, commands, schemaDefinitions) {
    let groups = BuildGroups(commands);
    let allEntries = groups.reduce((list, group) => list.concat(group.entries), []);
    let entryByName = new Map(allEntries.map(entry => [entry.command.name, entry]));
    let total = allEntries.length;
    let minVersion = '';
    let versionButtons = [];

    // ---- Toolbar: search, the buttons, the chips and the "Showing" line ----
    let toolbar = CreateElement(parentElement, 'div', 'toolbar', null);
    let toolbarRow = CreateElement(toolbar, 'div', 'toolbar_row', null);
    let groupsButton = CreateTextElement(toolbarRow, 'button', 'toolbar_button toolbar_sections', '☰ Groups');
    groupsButton.type = 'button';
    groupsButton.setAttribute('aria-controls', 'sidebar');
    groupsButton.addEventListener('click', function () {
        document.body.classList.toggle('sidebar_open');
    });
    document.addEventListener('keydown', function (event) {
        if (event.key === 'Escape') {
            document.body.classList.remove('sidebar_open');
        }
    });
    document.addEventListener('click', function (event) {
        if (!event.target.closest('#sidebar, .toolbar_sections')) {
            document.body.classList.remove('sidebar_open');
        }
    });
    let search = CreateElement(toolbarRow, 'input', 'toolbar_search', null);
    search.type = 'search';
    search.placeholder = 'Search commands, e.g. zone, layout, property';
    search.setAttribute('aria-label', 'Search commands');
    let expandButton = CreateTextElement(toolbarRow, 'button', 'toolbar_button', 'Expand all');
    expandButton.type = 'button';
    expandButton.title = 'Open every group';
    let collapseButton = CreateTextElement(toolbarRow, 'button', 'toolbar_button', 'Collapse all');
    collapseButton.type = 'button';
    collapseButton.title = 'Close every group and command';

    let actionChips = RenderChips(toolbar, 'Action', gActions, ApplyFilters);
    let subjectChips = RenderChips(toolbar, 'Subject', gSubjects, ApplyFilters);

    let statusLine = CreateElement(toolbar, 'div', 'toolbar_status', null);
    let statusText = CreateElement(statusLine, 'span', null, null);
    statusText.setAttribute('aria-live', 'polite');
    let clearLink = CreateTextElement(statusLine, 'a', 'toolbar_clear', 'Clear filters');
    clearLink.href = '#all-commands';
    clearLink.hidden = true;
    clearLink.addEventListener('click', function (event) {
        event.preventDefault();
        ClearFilters();
        ApplyFilters();
        search.focus();
    });

    function UpdateToolbarHeight() {
        // On phones only the first row is pinned (style.css), so only it covers the list.
        let pinned = getComputedStyle(toolbar).display === 'contents' ? toolbarRow : toolbar;
        document.documentElement.style.setProperty('--toolbar-height', pinned.offsetHeight + 'px');
    }
    UpdateToolbarHeight();
    // The toolbar wraps onto more rows when the window narrows.
    if ('ResizeObserver' in window) {
        let observer = new ResizeObserver(UpdateToolbarHeight);
        observer.observe(toolbar);
        observer.observe(toolbarRow);
    } else {
        window.addEventListener('resize', UpdateToolbarHeight);
    }
    search.addEventListener('input', ApplyFilters);

    // ---- The groups, collapsed ----
    for (let group of groups) {
        let section = CreateElement(parentElement, 'section', 'group', null);
        section.id = group.id;
        let heading = CreateElement(section, 'h3', 'group_heading', null);
        let toggle = CreateElement(heading, 'button', 'group_toggle', null);
        toggle.type = 'button';
        toggle.setAttribute('aria-expanded', 'false');
        toggle.setAttribute('aria-controls', group.id + '-list');
        CreateTextElement(toggle, 'span', 'group_name', group.label);
        let count = CreateTextElement(toggle, 'span', 'group_count', String(group.entries.length));
        let preview = CreateTextElement(toggle, 'span', 'group_preview',
            group.entries.slice(0, 4).map(entry => entry.command.name).join(', ') + (group.entries.length > 4 ? ', …' : ''));
        let body = CreateElement(section, 'div', 'group_body', null);
        body.id = group.id + '-list';
        body.hidden = true;
        group.section = section;
        group.toggle = toggle;
        group.count = count;
        group.preview = preview;
        group.body = body;
        group.built = false;
        group.subheadings = [];
        toggle.addEventListener('click', function () {
            let open = !group.open;
            SetGroupOpen(group, open, true);
            if (open) {
                ScrollToElement(section);
            }
        });
    }

    // The rows of a group are built when it is first opened (or searched).
    function BuildGroupRows(group) {
        if (group.built) {
            return;
        }
        group.built = true;
        let split = group.entries.length > gSplitGroupSize;
        let list = group.body;
        if (split) {
            for (let action of gActions) {
                let actionEntries = group.entries.filter(entry => entry.action === action.key);
                if (actionEntries.length === 0) {
                    continue;
                }
                let block = CreateElement(group.body, 'div', 'group_action', null);
                let title = CreateTextElement(block, 'h4', 'group_action_title', action.label);
                let blockList = CreateElement(block, 'div', 'group_list', null);
                group.subheadings.push({ block: block, title: title, entries: actionEntries });
                for (let entry of actionEntries) {
                    entry.element = RenderCommand(entry, page);
                    blockList.appendChild(entry.element);
                }
            }
        } else {
            list = CreateElement(group.body, 'div', 'group_list', null);
            for (let entry of group.entries) {
                entry.element = RenderCommand(entry, page);
                list.appendChild(entry.element);
            }
        }
        for (let entry of group.entries) {
            entry.element.hidden = !entry.visible;
        }
        for (let subheading of group.subheadings) {
            subheading.block.hidden = !subheading.entries.some(entry => entry.visible);
        }
    }

    function SetGroupOpen(group, open, byUser) {
        group.open = open;
        if (byUser && !Filtering()) {
            group.userOpen = open;
        }
        if (open) {
            BuildGroupRows(group);
        }
        group.body.hidden = !open;
        group.toggle.setAttribute('aria-expanded', String(open));
        group.section.classList.toggle('group_open', open);
    }

    // ---- The sidebar: the groups (with live counts) and the Added-in filter ----
    let sidebar = document.getElementById('sidebar');
    if (sidebar) {
        sidebar.setAttribute('aria-label', 'Groups');
        CreateTextElement(sidebar, 'div', 'sidebar_title', 'Groups');
        let list = CreateElement(sidebar, 'ul', 'sidebar_list', null);
        let topLink = CreateTextElement(CreateElement(list, 'li', null, null), 'a', 'sidebar_link', 'All commands');
        topLink.href = '#all-commands';
        topLink.addEventListener('click', function (event) {
            let target = document.getElementById('all-commands');
            if (target) {
                event.preventDefault();
                document.body.classList.remove('sidebar_open');
                ScrollToElement(target);
            }
        });
        for (let group of groups) {
            let link = CreateElement(CreateElement(list, 'li', null, null), 'a', 'sidebar_link', null);
            link.href = '#' + group.id;
            CreateTextElement(link, 'span', null, group.label);
            group.sidebarCount = CreateTextElement(link, 'span', 'sidebar_count', String(group.entries.length));
            group.sidebarLink = link;
            link.addEventListener('click', function (event) {
                event.preventDefault();
                document.body.classList.remove('sidebar_open');
                SetGroupOpen(group, true, true);
                ScrollToElement(group.section);
                history.replaceState(null, '', window.location.pathname + window.location.search + '#' + group.id);
            });
        }

        CreateTextElement(sidebar, 'div', 'sidebar_title', 'Added in');
        let versions = CreateElement(sidebar, 'div', 'sidebar_versions', null);
        versions.setAttribute('role', 'group');
        versions.setAttribute('aria-label', 'Show commands added in this version or later');
        let minors = new Set(allEntries.map(entry => MinorVersion(entry.command.version)));
        for (let minor of [''].concat([...minors].sort((a, b) => CompareVersions(b, a)))) {
            let button = CreateTextElement(versions, 'button', 'sidebar_version', minor ? minor + '+' : 'Any');
            button.type = 'button';
            button.title = minor ? 'Added in ' + minor + ' or later' : 'Any version';
            button.setAttribute('aria-pressed', String(minor === ''));
            button.dataset.minor = minor;
            button.addEventListener('click', function () {
                SetMinVersion(minor);
                ApplyFilters();
            });
            versionButtons.push(button);
        }

        // Close the sheet after following a link on narrow screens.
        sidebar.addEventListener('click', function (event) {
            if (event.target.closest('a')) {
                document.body.classList.remove('sidebar_open');
            }
        });

        // Highlight the group being read.
        if ('IntersectionObserver' in window) {
            let visible = new Set();
            let observer = new IntersectionObserver(function (observed) {
                for (let entry of observed) {
                    if (entry.isIntersecting) visible.add(entry.target); else visible.delete(entry.target);
                }
                let current = groups.find(group => visible.has(group.section));
                for (let group of groups) {
                    if (group === current) group.sidebarLink.setAttribute('aria-current', 'true'); else group.sidebarLink.removeAttribute('aria-current');
                }
            }, { rootMargin: '-80px 0px -60% 0px' });
            for (let group of groups) {
                observer.observe(group.section);
            }
        }
    }

    function SetMinVersion(minor) {
        minVersion = minor;
        for (let button of versionButtons) {
            button.setAttribute('aria-pressed', String(button.dataset.minor === minor));
        }
    }

    function SearchWords() {
        return search.value.toLowerCase().split(/\s+/).filter(word => word.length > 0);
    }

    function Filtering() {
        return SearchWords().length > 0 || minVersion !== '' || actionChips.value !== '' || subjectChips.value !== '';
    }

    function ClearFilters() {
        search.value = '';
        SetMinVersion('');
        actionChips.Set('');
        subjectChips.Set('');
    }

    let wasFiltering = false;

    function ApplyFilters() {
        let words = SearchWords();
        let filtering = Filtering();
        let shown = 0;
        for (let group of groups) {
            let groupShown = 0;
            for (let entry of group.entries) {
                entry.visible = words.every(word => entry.searchText.includes(word)) &&
                    (!minVersion || CompareVersions(entry.command.version, minVersion) >= 0) &&
                    (!actionChips.value || entry.action === actionChips.value) &&
                    (!subjectChips.value || entry.subjects.has(subjectChips.value));
                if (entry.element) {
                    entry.element.hidden = !entry.visible;
                }
                groupShown += entry.visible ? 1 : 0;
            }
            for (let subheading of group.subheadings) {
                subheading.block.hidden = !subheading.entries.some(entry => entry.visible);
            }
            group.section.hidden = groupShown === 0;
            let groupTotal = group.entries.length;
            group.count.textContent = groupShown === groupTotal ? String(groupTotal) : groupShown + ' of ' + groupTotal;
            let firstShown = group.entries.filter(entry => entry.visible).slice(0, 4);
            group.preview.textContent = firstShown.map(entry => entry.command.name).join(', ') + (groupShown > 4 ? ', …' : '');
            if (group.sidebarCount) {
                group.sidebarCount.textContent = String(groupShown);
                group.sidebarLink.classList.toggle('sidebar_empty', groupShown === 0);
            }
            // A filter opens just the groups with matches; clearing it brings back the groups the
            // reader opened.
            if (filtering) {
                SetGroupOpen(group, groupShown > 0, false);
            } else if (wasFiltering) {
                SetGroupOpen(group, group.userOpen, false);
            }
            shown += groupShown;
        }
        wasFiltering = filtering;
        statusText.textContent = !filtering ? total + ' commands in ' + groups.length + ' groups'
            : shown === 0 ? 'No matching commands'
            : 'Showing ' + shown + ' of ' + total + ' commands';
        clearLink.hidden = !filtering;
    }

    function SetAllOpen(open) {
        for (let group of groups) {
            if (group.section.hidden) {
                continue;
            }
            if (!open) {
                for (let entry of group.entries) {
                    if (entry.element && entry.element.open) {
                        entry.element.dataset.bulkToggle = 'true';
                        entry.element.open = false;
                    }
                }
            }
            SetGroupOpen(group, open, true);
            group.userOpen = open;
        }
        if (!open && /^#[A-Z]/.test(window.location.hash)) {
            history.replaceState(null, '', window.location.pathname + window.location.search);
        }
    }
    expandButton.addEventListener('click', () => SetAllOpen(true));
    collapseButton.addEventListener('click', () => SetAllOpen(false));

    // ---- Links: #CommandName and #group-… ----
    let page = {
        Related: function (entry) {
            if (!entry.noun) {
                return [];
            }
            return entry.group.entries
                .filter(other => other !== entry && other.noun === entry.noun)
                .slice(0, 5);
        },
        OpenCommand: function (name, scroll) {
            let entry = entryByName.get(name);
            if (!entry) {
                return false;
            }
            if (!entry.visible) {
                ClearFilters();
                ApplyFilters();
            }
            SetGroupOpen(entry.group, true, true);
            entry.group.userOpen = true;
            if (!entry.element.open) {
                entry.element.open = true;
            }
            history.replaceState(null, '', CommandUrl(name));
            if (scroll) {
                ScrollToElement(entry.element);
            }
            return true;
        }
    };

    function OpenFromHash(scroll) {
        let hash = window.location.hash.substring(1);
        try {
            hash = decodeURIComponent(hash);
        } catch (error) {
            // A malformed address: use it as it is.
        }
        if (!hash) {
            return;
        }
        if (page.OpenCommand(hash, scroll)) {
            return;
        }
        // Old links to the second MEP group go to the merged one.
        let group = groups.find(group => group.id === hash) ||
            (hash === GroupId('MEP Commands (extended)') ? groups.find(group => group.id === GroupId('MEP Commands')) : null);
        if (group) {
            SetGroupOpen(group, true, true);
            if (scroll) {
                ScrollToElement(group.section);
            }
        }
    }

    for (let entry of allEntries) {
        entry.visible = true;
    }
    ApplyFilters();
    // Open the linked command at once, and scroll to it when the page has its final layout.
    OpenFromHash(false);
    let scrolledToHash = false;
    function ScrollToHashOnce() {
        if (!scrolledToHash && window.location.hash) {
            scrolledToHash = true;
            OpenFromHash(true);
        }
    }
    if (document.readyState === 'complete') {
        requestAnimationFrame(ScrollToHashOnce);
    } else {
        window.addEventListener('load', function () {
            requestAnimationFrame(ScrollToHashOnce);
        });
    }
    window.addEventListener('hashchange', function () {
        OpenFromHash(true);
    });
}
