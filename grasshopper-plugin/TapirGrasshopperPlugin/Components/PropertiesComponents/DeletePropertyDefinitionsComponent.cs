using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class DeletePropertyDefinitionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeletePropertyDefinitions";

        public DeletePropertyDefinitionsComponent()
            : base(
                "DeletePropertyDefinitions",
                "Delete the given Custom Property Definitions.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the property definitions to delete.");
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

            var input = new DeletePropertyDefinitionsParameters
            {
                PropertyIds = new List<PropertyGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<PropertyGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid property identifier in the PropertyGuids input.");
                    return;
                }
                input.PropertyIds.Add(
                    new PropertyGuidWrapper { PropertyId = id });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeletePropertyDefinitions;

        public override Guid ComponentGuid =>
            new Guid("b6ca0582-df75-4a1d-9222-94ed7fa597e1");
    }
}
