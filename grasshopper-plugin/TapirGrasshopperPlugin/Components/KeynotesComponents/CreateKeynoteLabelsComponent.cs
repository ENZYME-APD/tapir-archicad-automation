using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Keynotes;

namespace TapirGrasshopperPlugin.Components.KeynotesComponents
{
    public class CreateKeynoteLabelsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateKeynoteLabels";

        public CreateKeynoteLabelsComponent()
            : base(
                "CreateKeynoteLabels",
                "Create Label elements that reference the given keynote items via autotext. " +
                "Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ItemGuids",
                "Identifier of the keynote item referenced by each label (input only 1 to use the same item for all).");

            InPoints(
                "Positions",
                "Reference point of each label (only X and Y are used).");

            InTexts(
                "ContentFields",
                "The keynote fields to include in the label text: Key, Title, Description or Reference. " +
                "Applied to all labels. Optional; defaults to all fields.");

            SetOptionality(2);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> itemWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3d> positions))
            {
                return;
            }

            if (itemWrappers.Count != 1 &&
                itemWrappers.Count != positions.Count)
            {
                this.AddError(
                    "The size of the input ItemGuids must be 1 or equal to the size of the input Positions.");
                return;
            }

            da.TryGetList(
                2,
                out List<string> contentFields);
            contentFields = contentFields ?? new List<string>();

            var itemIds = new List<KeynoteItemGuid>();
            foreach (var wrapper in itemWrappers)
            {
                var id = GuidObject<KeynoteItemGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid item identifier in the ItemGuids input.");
                    return;
                }
                itemIds.Add(id);
            }

            var items = new JArray();
            for (var i = 0; i < positions.Count; i++)
            {
                var item = new JObject
                {
                    ["keynoteItemId"] = new JObject
                    {
                        ["guid"] = itemIds[itemIds.Count == 1 ? 0 : i].Guid
                    },
                    ["position"] = new JObject
                    {
                        ["x"] = positions[i].X,
                        ["y"] = positions[i].Y
                    }
                };
                if (contentFields.Count > 0)
                {
                    item["contentFields"] = new JArray(contentFields);
                }
                items.Add(item);
            }

            var parameters = new JObject { ["labelsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateKeynoteLabels;

        public override Guid ComponentGuid =>
            new Guid("889ee9d2-74c8-4951-bb69-93472d260d7a");
    }
}
