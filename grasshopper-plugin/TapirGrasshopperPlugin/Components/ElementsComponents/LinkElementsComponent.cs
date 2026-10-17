using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class LinkElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "LinkElements";

        public LinkElementsComponent()
            : base(
                "LinkElements",
                "Create persistent links between pairs of elements, each from an element of FromElementGuids to the element of ToElementGuids at the same index, " +
                "with an optional integer flags value per link. The links are stored in the project and have no effect on geometry; " +
                "they are independent from groups, hotlinks and solid element operations. A link is directional: GetLinksOfElements returns it " +
                "for the element it starts from only, and UnlinkElements removes it. Undoable.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            ElementLinkInputs.Add(this);

            InIntegers(
                "Flags",
                "Flags stored on each link: a 32-bit integer free for your own use, for example to tell apart the kinds of links you create. " +
                "Defaults to 0 (input only 1 to use the same value for all). Optional.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each link (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var links = ElementLinkInputs.Read(
                this,
                da,
                0,
                1);
            if (links == null)
            {
                return;
            }

            da.TryGetList(
                2,
                out List<int> flags);

            if (flags.Count > 1 &&
                flags.Count != links.Count)
            {
                this.AddError(
                    "The size of the input Flags must be 0, 1 or equal to the size of the input FromElementGuids.");
                return;
            }

            for (var i = 0; i < links.Count; i++)
            {
                if (flags.Count == 1)
                {
                    links[i].Flags = flags[0];
                }
                else if (flags.Count > 1)
                {
                    links[i].Flags = flags[i];
                }
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new ElementLinksParameters { Links = links },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.LinkElements;

        public override Guid ComponentGuid =>
            new Guid("7a1c3e5b-9d2f-4b8e-a6c4-2f5d8e1b3a97");
    }
}
