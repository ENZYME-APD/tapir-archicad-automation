using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateElementSetsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateElementSets";

        public CreateElementSetsComponent()
            : base(
                "CreateElementSets",
                "Create element sets of the given elements (one branch per element set). An element set is a " +
                "hidden, persistent collection of elements (it is not a group and has no effect on selection or " +
                "editing).",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenericTree(
                "ElementGuids",
                "Identifiers of the elements to put into the element sets (one branch per element set).");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementSetGuids",
                "Identifier of each created element set.");

            OutErrorMessages(
                "Error message of each element set (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            // A generic parameter hands its tree out as IGH_Goo; asking for GH_ObjectWrapper
            // makes GetDataTree throw (see #634). The items are converted one by one below.
            if (!da.TryGetTree(
                    0,
                    out GH_Structure<IGH_Goo> tree))
            {
                return;
            }

            var elementSets = new JArray();
            foreach (var branch in tree.Branches)
            {
                var elements = new JArray();
                foreach (var goo in branch)
                {
                    var wrapper = goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo);
                    var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError(
                            "Invalid element identifier in the ElementGuids input.");
                        return;
                    }
                    elements.Add(
                        new JObject
                        {
                            ["elementId"] = new JObject { ["guid"] = id.Guid }
                        });
                }

                elementSets.Add(new JObject { ["elements"] = elements });
            }

            SetCadValuesWithCreatedIds<ElementSetGuid>(
                CommandName,
                new JObject { ["elementSets"] = elementSets },
                ToAddOn,
                da,
                "elementSets",
                "elementSetId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateElementSets;

        public override Guid ComponentGuid =>
            new Guid("60f12693-52ee-486c-9086-f271d573694f");
    }
}
