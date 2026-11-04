using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetMarkerLibraryPartOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetMarkerLibraryPartOfElements";

        public SetMarkerLibraryPartOfElementsComponent()
            : base(
                "SetMarkerLibraryPart",
                "Changes which library part the marker of the given Window and Door elements uses, given by " +
                "its name (find the available ones with GetAvailableLibraryParts); only a marker library part " +
                "for the opening's type is accepted. The marker gets the new library part's default " +
                "parameters, which SetMarkerParameters can change afterwards; the opening's own settings are " +
                "left as they are.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Window and Door elements.");

            InTexts(
                "LibraryPartNames",
                "The name of the library part the marker should use, as MarkerParameters and " +
                "GetAvailableLibraryParts report it (input only 1 to use the same library part for all).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its marker was changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> libraryPartNames) ||
                !ElementInputHelp.CheckOneOrMany(
                    this,
                    libraryPartNames,
                    elementIds.Count,
                    "LibraryPartNames"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                var libraryPartName = libraryPartNames.At(i);
                if (string.IsNullOrEmpty(libraryPartName))
                {
                    this.AddError("Empty library part name in the LibraryPartNames input.");
                    return;
                }

                items.Add(
                    new JObject
                    {
                        ["elementId"] = elementIds[i],
                        ["libraryPartName"] = libraryPartName
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementsWithMarkerLibraryPart"] = items },
                ToAddOn,
                da);
        }

        // Shares the icon of ChangeLibraryPartOfElements until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeLibraryPartOfElements;

        public override Guid ComponentGuid =>
            new Guid("e1a9bb94-7937-41ca-ac10-5319ea7ee880");
    }
}
