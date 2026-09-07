using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateGroups";

        public CreateGroupsComponent()
            : base(
                "CreateGroups",
                "Create groups of the passed elements (one branch per group, at least two elements each).",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenericTree(
                "ElementGuids",
                "Identifiers of the elements to group (one branch per group).");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "GroupGuids",
                "Identifier of each created group.");

            OutErrorMessages(
                "Error message of each group (empty when it was created successfully).");
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

            var input = new CreateGroupsParameters
            {
                ElementGroups = new List<ElementGroupParameters>()
            };

            foreach (var branch in tree.Branches)
            {
                var elements = new List<ElementGuidWrapper>();
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
                    elements.Add(new ElementGuidWrapper { ElementId = id });
                }

                if (elements.Count < 2)
                {
                    this.AddError(
                        "Each group (branch) must contain at least two elements.");
                    return;
                }

                input.ElementGroups.Add(
                    new ElementGroupParameters { Elements = elements });
            }

            SetCadValuesWithCreatedIds<GroupGuid>(
                CommandName,
                input,
                ToAddOn,
                da,
                "groupGuids",
                "groupId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGroups;

        public override Guid ComponentGuid =>
            new Guid("00650c57-8a8d-4a8b-b18e-d3b089e7e33d");
    }
}
