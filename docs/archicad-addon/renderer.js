function CreateElement(parentElement, elementType, className, innerHTML) {
    let elem = document.createElement(elementType);
    if (className) {
        elem.className = className;
    }
    if (innerHTML) {
        elem.innerHTML = innerHTML;
    }
    parentElement.appendChild(elem);
    return elem;
}

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

function CreateTextElement(parentElement, elementType, className, text) {
    let elem = CreateElement(parentElement, elementType, className, null);
    elem.textContent = text;
    return elem;
}

function PlainText(html) {
    // Descriptions may contain markup; DOMParser builds an inert document.
    return new DOMParser().parseFromString(html || '', 'text/html').body.textContent;
}

function GroupId(group) {
    return 'group-' + group.name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
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

// The example of a command is archicad-addon/Examples/<GroupFolder>/<CommandName>.py on main.
// It is fetched when its popdown is first opened, so nothing of it is copied into the docs.
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

function RenderExampleScript(container, examplePath, source) {
    let example = SplitExampleScript(source);
    for (let paragraph of example.description.split(/\n[ \t]*\n/)) {
        if (paragraph.trim().length > 0) {
            CreateTextElement(container, 'p', 'command_example_description', paragraph.replace(/\s*\n\s*/g, ' '));
        }
    }
    let code = CreateElement(CreateElement(container, 'pre', null, null), 'code', 'hljs language-python', null);
    if (typeof hljs !== 'undefined' && hljs.getLanguage('python')) {
        // highlight () escapes the source, so the markup it returns is safe to insert.
        code.innerHTML = hljs.highlight(example.code, { language: 'python', ignoreIllegals: true }).value;
    } else {
        code.textContent = example.code;
    }
    let link = CreateTextElement(CreateElement(container, 'div', 'command_example_link', null), 'a', null, 'View on GitHub');
    link.href = gExampleViewBaseUrl + examplePath;
    link.target = '_blank';
    link.rel = 'noopener';
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

function RenderExample(parentElement, examplePath) {
    let details = CreateElement(parentElement, 'details', 'command_example', null);
    CreateTextElement(details, 'summary', null, 'Example');
    let body = CreateElement(details, 'div', 'command_example_body', null);
    let loaded = false;
    details.addEventListener('toggle', function () {
        if (details.open && !loaded) {
            loaded = true;
            LoadExample(body, examplePath);
        }
    });
}

function RenderCommandContent(parentElement, groupName, command, schemaDefinitions) {
    let commandContent = CreateElement(parentElement, 'div', 'command_content', null);
    CreateElement(commandContent, 'div', 'command_description', command.description);
    if (!gGroupsWithoutExamples.includes(groupName)) {
        let examplePath = ExamplePath(groupName, command.name);
        // A plain link to the file, which works even when the example cannot be fetched.
        let exampleFile = CreateTextElement(commandContent, 'div', 'command_example_file', 'Example file: ');
        let exampleLink = CreateTextElement(exampleFile, 'a', null,
            'archicad-addon/Examples/' + ExampleFolder(groupName) + '/' + command.name + '.py');
        exampleLink.href = gExampleViewBaseUrl + examplePath;
        exampleLink.target = '_blank';
        exampleLink.rel = 'noopener';
        RenderExample(commandContent, examplePath);
    }
    try {
        CreateSchemaElement(commandContent, 'Input parameters', command.inputScheme, schemaDefinitions);
        CreateSchemaElement(commandContent, 'Output parameters', command.outputScheme, schemaDefinitions);
    } catch (error) {
        // One broken schema must not break the rest of the page.
        CreateElement(commandContent, 'div', 'command_description', 'Failed to render the schemas of this command.');
        console.error('Failed to render command ' + command.name, error);
    }
}

function RenderCommand(parentElement, groupName, command, schemaDefinitions) {
    parentElement.setAttribute("id", command.name);
    let headerElement = CreateElement(parentElement, 'summary', 'command_header', null);
    let titleElement = CreateElement(headerElement, 'span', 'command_title', null);
    CreateTextElement(titleElement, 'span', 'command_name', command.name);
    CreateTextElement(titleElement, 'span', 'command_version', command.version);
    // The description shows without opening the command; the schemas are
    // rendered when it is first opened, which keeps the page fast.
    CreateTextElement(headerElement, 'span', 'command_summary', PlainText(command.description));
    let rendered = false;
    parentElement.addEventListener("toggle", function () {
        if (parentElement.open && !rendered) {
            rendered = true;
            RenderCommandContent(parentElement, groupName, command, schemaDefinitions);
        }
        if (parentElement.dataset.bulkToggle) {
            // Opened or closed by "Expand all" / "Collapse all": leave the URL alone.
            delete parentElement.dataset.bulkToggle;
            return;
        }
        if (parentElement.open) {
            history.pushState(null, '', window.location.pathname + '#' + command.name);
        } else {
            history.replaceState(null, '', window.location.pathname);
        }
    });
}

function RenderOverview(parentElement, commands, groupElements) {
    let overview = CreateElement(parentElement, 'div', 'overview', null);
    overview.id = 'overview';
    let total = commands.reduce((sum, group) => sum + group.commands.length, 0);
    CreateTextElement(overview, 'div', 'overview_title', total + ' commands in ' + commands.length + ' groups');
    let table = CreateElement(overview, 'table', 'overview_table', null);
    let head = CreateElement(CreateElement(table, 'thead', null, null), 'tr', null, null);
    CreateTextElement(head, 'th', null, 'Group');
    CreateTextElement(head, 'th', 'number', 'Commands');
    CreateTextElement(head, 'th', 'number', 'Newest');
    let body = CreateElement(table, 'tbody', null, null);
    for (let group of commands) {
        let row = CreateElement(body, 'tr', null, null);
        let link = CreateTextElement(CreateElement(row, 'td', null, null), 'a', null, group.name);
        link.href = '#' + GroupId(group);
        let countCell = CreateTextElement(row, 'td', 'number', String(group.commands.length));
        let newest = group.commands.reduce((max, command) => CompareVersions(command.version, max) > 0 ? command.version : max, '0');
        CreateTextElement(row, 'td', 'number', newest);
        groupElements.get(group).overviewCount = countCell;
    }
}

function RenderSidebar(sidebar, commands, groupElements, onVersionChange) {
    // Section links (with live counts) and the version filter. On narrow
    // screens the same element is a drawer opened from the toolbar.
    CreateTextElement(sidebar, 'div', 'sidebar_title', 'Sections');
    let list = CreateElement(sidebar, 'ul', 'sidebar_list', null);
    let overviewLink = CreateTextElement(CreateElement(list, 'li', null, null), 'a', 'sidebar_link', 'Overview');
    overviewLink.href = '#overview';
    for (let group of commands) {
        let link = CreateElement(CreateElement(list, 'li', null, null), 'a', 'sidebar_link', null);
        link.href = '#' + GroupId(group);
        CreateTextElement(link, 'span', null, group.name.replace(/ Commands$/, ''));
        let groupElement = groupElements.get(group);
        groupElement.sidebarCount = CreateTextElement(link, 'span', 'sidebar_count', String(group.commands.length));
        groupElement.sidebarLink = link;
    }

    CreateTextElement(sidebar, 'div', 'sidebar_title', 'Added in');
    let versions = CreateElement(sidebar, 'div', 'sidebar_versions', null);
    versions.setAttribute('role', 'group');
    versions.setAttribute('aria-label', 'Show commands added in this version or later');
    let minors = new Set();
    for (let group of commands) {
        for (let command of group.commands) {
            minors.add(MinorVersion(command.version));
        }
    }
    let buttons = [];
    for (let minor of [''].concat([...minors].sort((a, b) => CompareVersions(b, a)))) {
        let button = CreateTextElement(versions, 'button', 'sidebar_version', minor ? minor + '+' : 'Any');
        button.type = 'button';
        button.title = minor ? 'Added in ' + minor + ' or later' : 'Any version';
        button.setAttribute('aria-pressed', String(minor === ''));
        button.addEventListener('click', function () {
            for (let other of buttons) {
                other.setAttribute('aria-pressed', String(other === button));
            }
            onVersionChange(minor);
        });
        buttons.push(button);
    }

    // Close the drawer after following a link on narrow screens.
    sidebar.addEventListener('click', function (event) {
        if (event.target.closest('a')) {
            document.body.classList.remove('sidebar_open');
        }
    });

    // Highlight the section being read.
    if ('IntersectionObserver' in window) {
        let linkOf = new Map([...groupElements.values()].map(g => [g.section, g.sidebarLink]));
        let visible = new Set();
        let observer = new IntersectionObserver(function (observed) {
            for (let entry of observed) {
                if (entry.isIntersecting) visible.add(entry.target); else visible.delete(entry.target);
            }
            let current = [...linkOf.keys()].find(section => visible.has(section));
            for (let [section, link] of linkOf) {
                if (section === current) link.setAttribute('aria-current', 'true'); else link.removeAttribute('aria-current');
            }
        }, { rootMargin: '-80px 0px -60% 0px' });
        for (let section of linkOf.keys()) {
            observer.observe(section);
        }
    }
}

function RenderToolbar(parentElement, onChange) {
    let toolbar = CreateElement(parentElement, 'div', 'toolbar', null);

    let sectionsButton = CreateTextElement(toolbar, 'button', 'toolbar_button toolbar_sections', '☰ Sections');
    sectionsButton.type = 'button';
    sectionsButton.setAttribute('aria-controls', 'sidebar');
    sectionsButton.addEventListener('click', function () {
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

    let search = CreateElement(toolbar, 'input', 'toolbar_search', null);
    search.type = 'search';
    search.placeholder = 'Search commands, e.g. zone, layout, property';
    search.setAttribute('aria-label', 'Search commands');

    let expandButton = CreateTextElement(toolbar, 'button', 'toolbar_button', 'Expand all');
    expandButton.type = 'button';
    let collapseButton = CreateTextElement(toolbar, 'button', 'toolbar_button', 'Collapse all');
    collapseButton.type = 'button';
    let status = CreateElement(toolbar, 'span', 'toolbar_status', null);
    status.setAttribute('aria-live', 'polite');

    function UpdateToolbarHeight() {
        document.documentElement.style.setProperty('--toolbar-height', toolbar.offsetHeight + 'px');
    }
    UpdateToolbarHeight();
    // The toolbar wraps onto more rows when the window narrows or the status text grows.
    if ('ResizeObserver' in window) {
        new ResizeObserver(UpdateToolbarHeight).observe(toolbar);
    } else {
        window.addEventListener('resize', UpdateToolbarHeight);
    }

    search.addEventListener('input', onChange);
    return { search, expandButton, collapseButton, status };
}

function RenderCommands(parentElement, commands, schemaDefinitions) {
    let groupElements = new Map();
    let minVersion = '';
    let controls = RenderToolbar(parentElement, ApplyFilters);
    let overviewParent = CreateElement(parentElement, 'div', null, null);

    for (let group of commands) {
        let section = CreateElement(parentElement, 'section', 'group', null);
        section.id = GroupId(group);
        let heading = CreateElement(section, 'div', 'group_name', null);
        CreateTextElement(heading, 'span', null, group.name);
        let count = CreateTextElement(heading, 'span', 'group_count', String(group.commands.length));
        let entries = [];
        for (let command of group.commands) {
            let commandContainer = CreateElement(section, 'details', 'command_container', null);
            try {
                RenderCommand(commandContainer, group.name, command, schemaDefinitions);
            } catch (error) {
                // One broken command must not blank out every command after it.
                CreateElement(commandContainer, 'div', 'command_description', 'Failed to render this command.');
                console.error('Failed to render command ' + command.name, error);
            }
            let searchText = (command.name + ' ' + PlainText(command.description) + ' ' + group.name).toLowerCase();
            entries.push({ command, element: commandContainer, searchText });
        }
        groupElements.set(group, { section, count, entries });
    }
    RenderOverview(overviewParent, commands, groupElements);
    let sidebar = document.getElementById('sidebar');
    if (sidebar) {
        RenderSidebar(sidebar, commands, groupElements, function (minor) {
            minVersion = minor;
            ApplyFilters();
        });
    }

    function ApplyFilters() {
        let words = controls.search.value.toLowerCase().split(/\s+/).filter(word => word.length > 0);
        let shown = 0;
        for (let groupElement of groupElements.values()) {
            let groupShown = 0;
            for (let entry of groupElement.entries) {
                let visible = words.every(word => entry.searchText.includes(word)) &&
                    (!minVersion || CompareVersions(entry.command.version, minVersion) >= 0);
                entry.element.hidden = !visible;
                groupShown += visible ? 1 : 0;
            }
            groupElement.section.hidden = groupShown === 0;
            let total = groupElement.entries.length;
            let countText = groupShown === total ? String(total) : groupShown + ' of ' + total;
            groupElement.count.textContent = countText;
            groupElement.overviewCount.textContent = countText;
            if (groupElement.sidebarCount) {
                groupElement.sidebarCount.textContent = groupShown === total ? String(total) : String(groupShown);
                groupElement.sidebarLink.classList.toggle('sidebar_empty', groupShown === 0);
            }
            shown += groupShown;
        }
        let filtering = words.length > 0 || minVersion;
        controls.status.textContent = !filtering ? '' : shown === 0 ? 'No matching commands' : shown + (shown === 1 ? ' command' : ' commands');
    }

    function SetAllOpen(open) {
        for (let groupElement of groupElements.values()) {
            for (let entry of groupElement.entries) {
                if (!entry.element.hidden && entry.element.open !== open) {
                    entry.element.dataset.bulkToggle = 'true';
                    entry.element.open = open;
                }
            }
        }
    }
    controls.expandButton.addEventListener('click', () => SetAllOpen(true));
    controls.collapseButton.addEventListener('click', () => SetAllOpen(false));
}
