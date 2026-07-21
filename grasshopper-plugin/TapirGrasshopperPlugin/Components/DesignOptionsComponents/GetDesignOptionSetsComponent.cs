using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class GetDesignOptionSetsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDesignOptionSets";

        public GetDesignOptionSetsComponent()
            : base(
                "GetDesignOptionSets",
                "Get all design option sets of the project. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DesignOptionSetGuids",
                "Identifiers of the design option sets.");

            OutTexts(
                "Names",
                "Names of the design option sets.");

            OutGenericTree(
                "DesignOptionGuids",
                "Identifiers of the design options in each set (one branch per set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<GetDesignOptionSetsResponse>,
                    out GetDesignOptionSetsResponse response))
            {
                return;
            }

            var sets = response.DesignOptionSets ?? new List<DesignOptionSetDetails>();

            da.SetDataList(
                0,
                sets.Select(s => s.DesignOptionSetId));

            da.SetDataList(
                1,
                sets.Select(s => s.Name));

            var tree = new GH_Structure<GH_ObjectWrapper>();
            for (var i = 0; i < sets.Count; i++)
            {
                var path = new GH_Path(i);
                tree.EnsurePath(path);
                if (sets[i].DesignOptions != null)
                {
                    foreach (var option in sets[i].DesignOptions)
                    {
                        tree.Append(
                            new GH_ObjectWrapper(option),
                            path);
                    }
                }
            }

            da.SetDataTree(
                2,
                tree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDesignOptionSets;

        public override Guid ComponentGuid =>
            new Guid("5d40fc6c-a512-4d76-a0c2-3a6ff211d817");
    }
}
