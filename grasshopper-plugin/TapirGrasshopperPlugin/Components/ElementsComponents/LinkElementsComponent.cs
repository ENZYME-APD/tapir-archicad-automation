using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The From/To element inputs LinkElements and UnlinkElements share.
    public static class ElementLinksHelp
    {
        public static void AddLinkInputs(
            Component component)
        {
            component.InGenerics(
                "FromElementGuids",
                "The element each link starts from (input only 1 to use the same element for all).");

            component.InGenerics(
                "ToElementGuids",
                "The element each link points to (input only 1 to use the same element for all).");
        }

        // The links of the From/To inputs at index 0 and 1, each with its
        // fromElementId and toElementId; null after an error.
        public static JArray Links(
            GH_ActiveObject component,
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> fromWrappers) ||
                !da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> toWrappers))
            {
                return null;
            }

            var fromIds = ElementInputHelp.ElementGuids(
                component,
                fromWrappers,
                "FromElementGuids");
            var toIds = ElementInputHelp.ElementGuids(
                component,
                toWrappers,
                "ToElementGuids");
            if (fromIds == null || toIds == null)
            {
                return null;
            }

            var count = Math.Max(
                fromIds.Count,
                toIds.Count);
            if (!ElementInputHelp.CheckOneOrMany(
                    component,
                    fromIds,
                    count,
                    "FromElementGuids") ||
                !ElementInputHelp.CheckOneOrMany(
                    component,
                    toIds,
                    count,
                    "ToElementGuids"))
            {
                return null;
            }

            var links = new JArray();
            for (var i = 0; i < count; i++)
            {
                links.Add(
                    new JObject
                    {
                        ["fromElementId"] = JObject.FromObject(
                            ElementInputHelp.ItemAt(
                                fromIds,
                                i)),
                        ["toElementId"] = JObject.FromObject(
                            ElementInputHelp.ItemAt(
                                toIds,
                                i))
                    });
            }
            return links;
        }
    }

    public class LinkElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "LinkElements";

        public LinkElementsComponent()
            : base(
                "LinkElements",
                "Creates persistent links between pairs of elements, each from one element to another, with " +
                "an optional integer flags value per link. Scripts can record their own relations between " +
                "elements this way (for example which label documents which column) instead of storing " +
                "GUIDs in properties. The links are stored in the project and have no effect on geometry; " +
                "they are independent from groups, hotlinks and solid element operations. A link is " +
                "directional: it is returned by GetLinksOfElements for the element it starts from only, and " +
                "removed by UnlinkElements. Undoable.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            ElementLinksHelp.AddLinkInputs(this);

            InIntegers(
                "Flags",
                "Optional, a 32-bit integer stored on each link, free for the script's own use (input only " +
                "1 to use the same value for all). Default is 0.");

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
            var links = ElementLinksHelp.Links(
                this,
                da);
            if (links == null)
            {
                return;
            }

            var flags = da.GetOptionals<int>(2);
            if (flags.Count > 0)
            {
                if (!ElementInputHelp.CheckOneOrMany(
                        this,
                        flags,
                        links.Count,
                        "Flags"))
                {
                    return;
                }

                for (var i = 0; i < links.Count; i++)
                {
                    links[i]["flags"] = ElementInputHelp.ItemAt(
                        flags,
                        i);
                }
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["links"] = links },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.LinkElements;

        public override Guid ComponentGuid =>
            new Guid("1dc084d6-d485-48a9-a09d-864684576d47");
    }
}
