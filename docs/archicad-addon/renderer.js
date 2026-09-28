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
            let refValue = schemaDefinitions[refKey];
            if (resolvedKeys.has(refKey)) {
                parentNode[parentKey] = {
                    type: refKey
                };
            } else {
                parentNode[parentKey] = refValue;
                parentNode[parentKey].title = refKey;
                resolvedKeys.add(refKey);
                ResolveReferences(schemaDefinitions, parentNode, parentKey, resolvedKeys);
                resolvedKeys.delete(refKey)
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
        schema: schema
    };
    ResolveReferences(schemaDefinitions, resolvedObject, 'schema', new Set());
    EnsureRenderableSchemas(resolvedObject.schema, new Set());
    let view = new JSONSchemaView(resolvedObject.schema);
    schemeContainer.appendChild(view.render());
}

function RenderCommand(parentElement, command, schemaDefinitions) {
    parentElement.setAttribute("id", command.name);
    parentElement.addEventListener("toggle", function () {
        if (parentElement.open) {
            history.pushState(null, '', window.location.pathname + '#' + command.name);
        } else {
            history.replaceState(null, '', window.location.pathname);
        }
    })
    let headerElement = CreateElement(parentElement, 'summary', 'command_header', null);
    CreateElement(headerElement, 'span', 'command_name', command.name);
    CreateElement(headerElement, 'span', 'command_version', command.version);
    let commandContent = CreateElement(parentElement, 'div', 'command_content', null);

    CreateElement(commandContent, 'div', 'command_description', command.description);
    CreateSchemaElement(commandContent, 'Input parameters', command.inputScheme, schemaDefinitions);
    CreateSchemaElement(commandContent, 'Output parameters', command.outputScheme, schemaDefinitions);
}

function RenderCommands(parentElement, commands, schemaDefinitions) {
    for (let group of commands) {
        CreateElement(parentElement, 'div', 'group_name', group.name);
        for (command of group.commands) {
            let commandContainer = CreateElement(parentElement, 'details', 'command_container', null);
            try {
                RenderCommand(commandContainer, command, schemaDefinitions);
            } catch (error) {
                // One broken schema must not blank out every command after it.
                CreateElement(commandContainer, 'div', 'command_description', 'Failed to render the schemas of this command.');
                console.error('Failed to render command ' + command.name, error);
            }
        }
    }
}
