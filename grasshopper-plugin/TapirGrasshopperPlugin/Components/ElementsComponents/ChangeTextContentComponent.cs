using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ChangeTextContentComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ChangeTextContent";

        public ChangeTextContentComponent()
            : base(
                "ChangeTextContent",
                "Replaces a range of the content of Text elements and text Labels, keeping the styles of the runs " +
                "and the autotexts in the rest of the content. The changes are applied in order, so several changes " +
                "of one element see the content as the previous one left it.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Text elements or text Labels to change (list an element once per change).");

            InIntegers(
                "From",
                "The position in the content where the replaced range starts (input only 1 to use the same value for all).");

            InIntegers(
                "Range",
                "The number of characters to replace; 0 inserts the content at From. The range may not reach over " +
                "a line end into another paragraph (input only 1 to use the same value for all).");

            InTexts(
                "Contents",
                "The new content of the range; an empty text deletes the range. It must not contain line ends " +
                "(input only 1 to use the same value for all). Optional: without it the ranges are deleted.");

            SetOptionality(3);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each change (empty when it was applied).");
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
                    out List<int> from) ||
                !da.TryGetList(
                    2,
                    out List<int> range))
            {
                return;
            }

            // An empty text deletes the range, so an empty input item is valid.
            var contents = da.OptionalList<string>(3);
            if (contents.Count == 0)
            {
                contents.Add("");
            }

            if (!this.CheckSizes(
                    elementIds.Count,
                    "ElementGuids",
                    ("From", from.Count),
                    ("Range", range.Count),
                    ("Contents", contents.Count)))
            {
                return;
            }

            var contentChanges = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                contentChanges.Add(new JObject
                {
                    ["elementId"] = elementIds[i],
                    ["from"] = from.At(i),
                    ["range"] = range.At(i),
                    ["content"] = contents.At(i) ?? ""
                });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["contentChanges"] = contentChanges },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeTextContent;

        public override Guid ComponentGuid =>
            new Guid("558886e8-79b9-4a18-9565-4d9b1e500caa");
    }
}
