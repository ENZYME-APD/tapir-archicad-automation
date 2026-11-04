using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCSpatialStructureComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetIFCSpatialStructure";

        public GetIFCSpatialStructureComponent()
            : base(
                "GetIFCSpatialStructure",
                "Returns the IfcProject, IfcSite and IfcBuilding objects of the project's IFC model " +
                "(IFC type and GlobalId), and for the given elements the index of the story they are " +
                "assigned to in the IFC model. Available from Archicad 28.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Optional. Elements to get the IFC story index of.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "SpatialIFCTypes",
                "The IFC type of the IfcProject, IfcSite and IfcBuilding objects, in this order " +
                "(null for an object Archicad does not report).");

            OutTexts(
                "SpatialGlobalIds",
                "The IFC GlobalId of the IfcProject, IfcSite and IfcBuilding objects, in this order " +
                "(null for an object Archicad does not report).");

            OutGenerics(
                "ElementGuids",
                "Identifier of each given element (null for failed items).");

            OutIntegers(
                "StoryIndices",
                "The index of the story each element is assigned to in the IFC model (null for failed items).");

            OutErrorMessages(
                "Error message of each element (empty when its story index was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var elementWrappers = da.GetOptionals<GH_ObjectWrapper>(0);
            if (elementWrappers.Count > 0)
            {
                var elements = ElementsObject.ConvertFrom(elementWrappers);
                if (elements?.Elements == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }

                parameters["elements"] = JArray.FromObject(elements.Elements);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var spatialTypes = new List<string>();
            var spatialGlobalIds = new List<string>();
            foreach (var key in new[] { "project", "site", "building" })
            {
                var ifcObject = response[key];
                spatialTypes.Add(ifcObject?["ifcType"]?.ToString());
                spatialGlobalIds.Add(ifcObject?["globalId"]?.ToString());
            }

            var elementGuids = new List<object>();
            var storyIndices = new List<object>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "elements"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    elementGuids.Add(null);
                    storyIndices.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                elementGuids.Add(ElementsStructuredGetterComponent.ElementIdOf(item));
                storyIndices.Add(item.Value<int?>("storyIndex"));
                errorMessages.Add("");
            }

            da.SetDataList(0, spatialTypes);
            da.SetDataList(1, spatialGlobalIds);
            da.SetDataList(2, elementGuids);
            da.SetDataList(3, storyIndices);
            da.SetDataList(4, errorMessages);
        }

        // Shares the icon of GetIFCTypeOfElements until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCTypeOfElements;

        public override Guid ComponentGuid =>
            new Guid("2b5b09aa-c5da-4293-9079-d7aedd6f5001");
    }
}
