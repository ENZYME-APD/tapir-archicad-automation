using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetNoteAutotextsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetNoteAutotexts";

        public GetNoteAutotextsComponent()
            : base(
                "GetNoteAutotexts",
                "Retrieves the autotexts available in the notes of an element type (e.g. a Dimension), " +
                "each with the element types it is available for. The autotexts are identified by the " +
                "numeric value of their API_NoteAutotextID. Needs Archicad 26 or later.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "NoteOwnerType",
                "The type of the element that owns the note, e.g. Dimension, RadialDimension, " +
                "LevelDimension or AngleDimension.");

            InText(
                "DimensionAppearance",
                "Optional dimension type (Linear, Cumulative, Baseline or Elevation) whose autotexts " +
                "are listed when NoteOwnerType is Dimension. Defaults to Linear. Leave it empty " +
                "for other owner types.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "AutotextIds",
                "The numeric value of the API_NoteAutotextID of each autotext, in ascending order.");

            OutTextTree(
                "ElementTypes",
                "The element types each autotext is available for, one branch per autotext.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string noteOwnerType) ||
                string.IsNullOrWhiteSpace(noteOwnerType))
            {
                return;
            }

            var parameters = new JObject
            {
                ["noteOwnerType"] = noteOwnerType.Trim()
            };

            if (da.TryGet(
                    1,
                    out string dimensionAppearance) &&
                !string.IsNullOrWhiteSpace(dimensionAppearance))
            {
                parameters["dimensionAppearance"] = dimensionAppearance.Trim();
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var autotextIds = new List<object>();
            var elementTypes = new DataTree<string>();

            var items = JsonOutputHelp.Items(
                response,
                "noteAutotexts");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementTypes.EnsurePath(path);

                autotextIds.Add(item.Value<int?>("autotextId"));

                if (item["elementTypes"] is JArray types)
                {
                    foreach (var type in types)
                    {
                        elementTypes.Add(
                            type.ToString(),
                            path);
                    }
                }
            }

            da.SetDataList(
                0,
                autotextIds);

            da.SetDataTree(
                1,
                elementTypes);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetNoteAutotexts;

        public override Guid ComponentGuid =>
            new Guid("3fd4e405-c142-466e-8ee5-e1f91e8f8746");
    }
}
