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
    let view = new JSONSchemaView(resolvedObject.schema);
    schemeContainer.appendChild(view.render());
}

function RenderCommand(parentElement, command, schemaDefinitions) {
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
            RenderCommand(commandContainer, command, schemaDefinitions);
        }
    }
}
