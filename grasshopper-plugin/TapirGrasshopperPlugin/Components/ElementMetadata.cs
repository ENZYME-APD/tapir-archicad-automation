using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.ArchiCad;

namespace TapirGrasshopperPlugin.Components
{
    // Embeds the "Tapir GH" metadata properties (the path of the Grasshopper
    // definition, the time of the last update and the instance guid of the
    // creating component) into elements created from Grasshopper, and finds
    // the elements a component stamped earlier so a new run can replace them
    // instead of duplicating them. Shared between CreateElementsComponentBase
    // and the creator components with a custom Solve.
    //
    // The property group and the definitions are created on demand through
    // the existing property commands of the add-on; the definitions are made
    // available for every classification item, so stamping only fails for
    // unclassified elements (reported as a warning, not an error).
    internal sealed class ElementMetadata
    {
        internal const string MetadataGroupName = "Tapir GH";
        internal const string SourceDefinitionPropertyName = "Source Definition";
        internal const string LastUpdatedPropertyName = "Last Updated";
        internal const string ComponentGuidPropertyName = "Component Guid";

        internal const string EmbedMetadataInputName = "EmbedMetadata";
        internal const string EmbedMetadataDescription =
            "Embed the \"" + MetadataGroupName + "\" metadata properties (source definition path, " +
            "last update time, creating component guid) into the created elements as custom " +
            "properties. The property group and definitions are created in the Archicad " +
            "project when missing.";

        internal const string ReplaceExistingInputName = "ReplaceExisting";
        internal const string ReplaceExistingDescription =
            "Delete the elements created earlier by this component - found through the \"" +
            ComponentGuidPropertyName + "\" metadata property - after the new elements are " +
            "created, instead of keeping them as duplicates.";

        private readonly GH_Component _component;
        private readonly Func<string, JObject, CommandResponse> _toAddOn;
        private readonly Func<string, JObject, CommandResponse> _toArchicad;

        internal ElementMetadata(
            GH_Component component,
            Func<string, JObject, CommandResponse> toAddOn,
            Func<string, JObject, CommandResponse> toArchicad)
        {
            _component = component;
            _toAddOn = toAddOn;
            _toArchicad = toArchicad;
        }

        // The elements stamped earlier with this component's instance guid.
        // Returns an empty array when the metadata properties do not exist
        // yet; a failed query also yields an empty array (with a warning), so
        // nothing gets deleted based on incomplete data.
        internal JArray FindPreviouslyCreatedElements()
        {
            if (!TryGetMetadataPropertyIds(
                    out Dictionary<string, string> propertyIds,
                    out _))
            {
                AddWarning(
                    "Failed to query the metadata properties; the previously created elements are kept.");
                return new JArray();
            }

            var componentGuidPropertyId = propertyIds[ComponentGuidPropertyName];
            if (componentGuidPropertyId == null)
            {
                return new JArray();
            }

            if (!TrySendCommand(
                    "GetAllElements",
                    new JObject(),
                    _toAddOn,
                    out JObject allElementsResponse) ||
                !(allElementsResponse?["elements"] is JArray allElements))
            {
                AddWarning(
                    "Failed to query the elements; the previously created elements are kept.");
                return new JArray();
            }

            if (allElements.Count == 0)
            {
                return new JArray();
            }

            var parameters = new JObject
            {
                ["elements"] = allElements.DeepClone(),
                ["properties"] = new JArray
                {
                    new JObject
                    {
                        ["propertyId"] = new JObject { ["guid"] = componentGuidPropertyId }
                    }
                }
            };

            if (!TrySendCommand(
                    "GetPropertyValuesOfElements",
                    parameters,
                    _toAddOn,
                    out JObject valuesResponse) ||
                !(valuesResponse?["propertyValuesForElements"] is JArray valuesForElements))
            {
                AddWarning(
                    "Failed to query the metadata property values; the previously created elements are kept.");
                return new JArray();
            }

            var previousElements = new JArray();
            var ownGuid = _component.InstanceGuid.ToString();
            for (var i = 0; i < valuesForElements.Count && i < allElements.Count; i++)
            {
                var value =
                    valuesForElements[i]?["propertyValues"] is JArray propertyValues &&
                    propertyValues.Count > 0
                        ? propertyValues[0]?["propertyValue"]?["value"]?.ToString()
                        : null;
                if (value != null &&
                    string.Equals(value, ownGuid, StringComparison.OrdinalIgnoreCase))
                {
                    previousElements.Add(allElements[i].DeepClone());
                }
            }
            return previousElements;
        }

        // Stamps the metadata onto the elements of a creator command's
        // response. The elements exist already at this point, so failures
        // only add a warning instead of failing the component.
        internal void StampCreatedElements(
            JObject creationResponse)
        {
            var createdElementGuids = new List<string>();
            if (creationResponse?["elements"] is JArray elements)
            {
                foreach (var element in elements)
                {
                    var guid = element?["elementId"]?["guid"]?.ToString();
                    if (guid != null)
                    {
                        createdElementGuids.Add(guid);
                    }
                }
            }

            if (createdElementGuids.Count == 0)
            {
                return;
            }

            if (!TryEnsureMetadataProperties(
                    out Dictionary<string, string> propertyIds))
            {
                AddWarning(
                    "Failed to create the \"" + MetadataGroupName +
                    "\" metadata properties; the created elements were not stamped.");
                return;
            }

            var values = new List<KeyValuePair<string, string>>
            {
                new KeyValuePair<string, string>(
                    propertyIds[SourceDefinitionPropertyName],
                    _component.OnPingDocument()?.FilePath ?? ""),
                new KeyValuePair<string, string>(
                    propertyIds[LastUpdatedPropertyName],
                    DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss")),
                new KeyValuePair<string, string>(
                    propertyIds[ComponentGuidPropertyName],
                    _component.InstanceGuid.ToString())
            };

            var elementPropertyValues = new JArray();
            foreach (var elementGuid in createdElementGuids)
            {
                foreach (var value in values)
                {
                    elementPropertyValues.Add(
                        new JObject
                        {
                            ["elementId"] = new JObject { ["guid"] = elementGuid },
                            ["propertyId"] = new JObject { ["guid"] = value.Key },
                            ["propertyValue"] = new JObject { ["value"] = value.Value }
                        });
                }
            }

            if (!TrySendCommand(
                    "SetPropertyValuesOfElements",
                    new JObject { ["elementPropertyValues"] = elementPropertyValues },
                    _toAddOn,
                    out JObject response))
            {
                AddWarning("Failed to embed the metadata into the created elements.");
                return;
            }

            var failedCount = 0;
            if (response?["executionResults"] is JArray executionResults)
            {
                foreach (var result in executionResults)
                {
                    if (result?["success"]?.ToObject<bool>() != true)
                    {
                        failedCount++;
                    }
                }
            }
            if (failedCount > 0)
            {
                AddWarning(
                    $"Failed to embed {failedCount} of {elementPropertyValues.Count} metadata property values " +
                    "(the metadata properties are only available for classified elements).");
            }
        }

        // Deletes the elements found by FindPreviouslyCreatedElements. A
        // failure (e.g. elements reserved by others in Teamwork) only adds a
        // warning.
        internal void DeletePreviouslyCreatedElements(
            JArray previousElements)
        {
            if (previousElements == null || previousElements.Count == 0)
            {
                return;
            }

            if (!TrySendCommand(
                    "DeleteElements",
                    new JObject { ["elements"] = previousElements },
                    _toAddOn,
                    out JObject response) ||
                response?["success"]?.ToObject<bool>() != true)
            {
                var message = response?["error"]?["message"]?.ToString();
                AddWarning(
                    "Failed to delete the previously created elements" +
                    (message == null ? "." : ": " + message));
            }
        }

        // Ids of the metadata property definitions by name; a missing
        // definition maps to null. groupExists tells whether any property of
        // the metadata group was seen. Returns false when the query failed.
        private bool TryGetMetadataPropertyIds(
            out Dictionary<string, string> propertyIds,
            out bool groupExists)
        {
            propertyIds = new Dictionary<string, string>
            {
                [SourceDefinitionPropertyName] = null,
                [LastUpdatedPropertyName] = null,
                [ComponentGuidPropertyName] = null
            };
            groupExists = false;

            if (!TrySendCommand(
                    "GetAllProperties",
                    new JObject(),
                    _toAddOn,
                    out JObject response) ||
                !(response?["properties"] is JArray properties))
            {
                return false;
            }

            foreach (var property in properties)
            {
                if (property?["propertyGroupName"]?.ToString() != MetadataGroupName)
                {
                    continue;
                }
                groupExists = true;

                var name = property["propertyName"]?.ToString();
                if (name != null &&
                    propertyIds.ContainsKey(name) &&
                    propertyIds[name] == null)
                {
                    propertyIds[name] = property["propertyId"]?["guid"]?.ToString();
                }
            }

            return true;
        }

        // Makes sure the metadata property group and definitions exist,
        // creating the missing ones. Returns false when they could not be
        // created.
        private bool TryEnsureMetadataProperties(
            out Dictionary<string, string> propertyIds)
        {
            if (!TryGetMetadataPropertyIds(
                    out propertyIds,
                    out bool groupExists))
            {
                return false;
            }

            var missingNames = new List<string>();
            foreach (var pair in propertyIds)
            {
                if (pair.Value == null)
                {
                    missingNames.Add(pair.Key);
                }
            }
            if (missingNames.Count == 0)
            {
                return true;
            }

            if (!groupExists)
            {
                // The per-item result is ignored on purpose: the call fails
                // when the group exists but is empty (an empty group is
                // invisible to GetAllProperties), and the definitions below
                // find the group by name either way.
                TrySendCommand(
                    "CreatePropertyGroups",
                    new JObject
                    {
                        ["propertyGroups"] = new JArray
                        {
                            new JObject
                            {
                                ["propertyGroup"] = new JObject
                                {
                                    ["name"] = MetadataGroupName,
                                    ["description"] = "Metadata of the elements created by the Tapir Grasshopper plugin."
                                }
                            }
                        }
                    },
                    _toAddOn,
                    out _);
            }

            if (!TryGetAllClassificationItemIds(out JArray availability))
            {
                return false;
            }

            var descriptions = new Dictionary<string, string>
            {
                [SourceDefinitionPropertyName] = "Path of the Grasshopper definition the element was created from.",
                [LastUpdatedPropertyName] = "Time the element was last created or updated from Grasshopper.",
                [ComponentGuidPropertyName] = "Instance guid of the Grasshopper component the element was created by."
            };

            var definitions = new JArray();
            foreach (var name in missingNames)
            {
                definitions.Add(
                    new JObject
                    {
                        ["propertyDefinition"] = new JObject
                        {
                            ["name"] = name,
                            ["description"] = descriptions[name],
                            ["type"] = "string",
                            ["isEditable"] = true,
                            ["defaultValue"] = new JObject
                            {
                                ["basicDefaultValue"] = new JObject
                                {
                                    ["type"] = "string",
                                    ["status"] = "userUndefined"
                                }
                            },
                            ["group"] = new JObject { ["name"] = MetadataGroupName },
                            ["availability"] = availability.DeepClone()
                        }
                    });
            }

            if (!TrySendCommand(
                    "CreatePropertyDefinitions",
                    new JObject { ["propertyDefinitions"] = definitions },
                    _toAddOn,
                    out JObject response) ||
                !(response?["propertyIds"] is JArray createdIds))
            {
                return false;
            }

            for (var i = 0; i < missingNames.Count && i < createdIds.Count; i++)
            {
                var guid = createdIds[i]?["propertyId"]?["guid"]?.ToString();
                if (guid == null)
                {
                    AddWarning(
                        $"Failed to create the metadata property \"{missingNames[i]}\": " +
                        (createdIds[i]?["error"]?["message"]?.ToString() ?? "unknown error"));
                    return false;
                }
                propertyIds[missingNames[i]] = guid;
            }

            foreach (var pair in propertyIds)
            {
                if (pair.Value == null)
                {
                    return false;
                }
            }
            return true;
        }

        // Every classification item of every system, used as the availability
        // of the created metadata properties.
        private bool TryGetAllClassificationItemIds(
            out JArray classificationItemIds)
        {
            classificationItemIds = new JArray();

            if (!TrySendCommand(
                    "GetAllClassificationSystems",
                    new JObject(),
                    _toArchicad,
                    out JObject systemsResponse) ||
                !(systemsResponse?["classificationSystems"] is JArray systems))
            {
                return false;
            }

            foreach (var system in systems)
            {
                var systemId = system?["classificationSystemId"];
                if (systemId == null)
                {
                    continue;
                }

                if (!TrySendCommand(
                        "GetAllClassificationsInSystem",
                        new JObject { ["classificationSystemId"] = systemId.DeepClone() },
                        _toArchicad,
                        out JObject itemsResponse))
                {
                    return false;
                }

                CollectClassificationItemIds(
                    itemsResponse?["classificationItems"] as JArray,
                    classificationItemIds);
            }
            return true;
        }

        private static void CollectClassificationItemIds(
            JArray classificationItems,
            JArray result)
        {
            if (classificationItems == null)
            {
                return;
            }

            foreach (var item in classificationItems)
            {
                var classificationItem = item?["classificationItem"];
                var itemId = classificationItem?["classificationItemId"];
                if (itemId != null)
                {
                    result.Add(
                        new JObject { ["classificationItemId"] = itemId.DeepClone() });
                }
                CollectClassificationItemIds(
                    classificationItem?["children"] as JArray,
                    result);
            }
        }

        private bool TrySendCommand(
            string commandName,
            JObject parameters,
            Func<string, JObject, CommandResponse> sendCommand,
            out JObject response)
        {
            var commandResponse = sendCommand.Invoke(
                commandName,
                parameters);
            response = commandResponse.Succeeded ? commandResponse.Result : null;
            return commandResponse.Succeeded;
        }

        private void AddWarning(
            string message)
        {
            _component.AddRuntimeMessage(
                GH_RuntimeMessageLevel.Warning,
                message);
        }
    }
}
