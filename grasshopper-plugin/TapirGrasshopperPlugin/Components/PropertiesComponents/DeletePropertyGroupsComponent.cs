using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class DeletePropertyGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeletePropertyGroups";

        public DeletePropertyGroupsComponent()
            : base(
                "DeletePropertyGroups",
                "Delete the given Custom Property Groups.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGroupGuids",
                "Identifiers of the property groups to delete.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var input = new DeletePropertyGroupsParameters
            {
                PropertyGroupIds = new List<PropertyGroupGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<PropertyGroupGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid property group identifier in the PropertyGroupGuids input.");
                    return;
                }
                input.PropertyGroupIds.Add(
                    new PropertyGroupGuidWrapper { PropertyGroupId = id });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeletePropertyGroups;

        public override Guid ComponentGuid =>
            new Guid("16b0cce6-3ce8-4a68-a777-fde8e6744e57");
    }
}
