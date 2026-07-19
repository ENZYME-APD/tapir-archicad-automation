using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class UpdatePropertyDefinitionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdatePropertyDefinitions";

        public UpdatePropertyDefinitionsComponent()
            : base(
                "UpdatePropertyDefinitions",
                "Update the expression(s) of existing expression-based Custom Property Definitions.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the expression-based property definitions to update.");

            inManager.AddTextParameter(
                "Expressions",
                "Expressions",
                "The new expression strings for each property (one branch per property, at least one expression each).",
                GH_ParamAccess.tree);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> propertyWrappers))
            {
                return;
            }

            if (!da.TryGetTree(
                    1,
                    out GH_Structure<GH_String> expressionsTree))
            {
                return;
            }

            var input = new UpdatePropertyDefinitionsParameters
            {
                PropertyDefinitions = new List<PropertyDefinitionUpdate>()
            };

            for (var i = 0; i < propertyWrappers.Count; i++)
            {
                var id = GuidObject<PropertyGuidObject>.CreateFromWrapper(propertyWrappers[i]);
                if (id == null)
                {
                    this.AddError(
                        "Invalid property identifier in the PropertyGuids input.");
                    return;
                }

                var expressions = new List<string>();
                if (i < expressionsTree.Branches.Count)
                {
                    expressions = expressionsTree.Branches[i]
                        .Select(x => x?.Value)
                        .Where(x => x != null)
                        .ToList();
                }

                if (expressions.Count == 0)
                {
                    this.AddError(
                        "Each property must have at least one expression in the Expressions input.");
                    return;
                }

                input.PropertyDefinitions.Add(
                    new PropertyDefinitionUpdate
                    {
                        PropertyId = id,
                        Expressions = expressions
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdatePropertyDefinitions;

        public override Guid ComponentGuid =>
            new Guid("1eaaba4e-9162-4784-8e34-893e05498b43");
    }
}
