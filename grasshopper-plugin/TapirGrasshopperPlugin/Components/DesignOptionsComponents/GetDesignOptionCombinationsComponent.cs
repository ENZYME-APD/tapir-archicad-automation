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
    public class GetDesignOptionCombinationsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDesignOptionCombinations";

        public GetDesignOptionCombinationsComponent()
            : base(
                "GetDesignOptionCombinations",
                "Get all design option combinations of the project. Available from Archicad 27.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations.");

            OutTexts(
                "Names",
                "Names of the design option combinations.");

            OutGenericTree(
                "ActiveDesignOptionGuids",
                "Identifiers of the active design options in each combination (one branch per combination). Available from Archicad 29.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<GetDesignOptionCombinationsResponse>,
                    out GetDesignOptionCombinationsResponse response))
            {
                return;
            }

            var combinations =
                response.DesignOptionCombinations ?? new List<DesignOptionCombinationDetails>();

            da.SetDataList(
                0,
                combinations.Select(c => c.DesignOptionCombinationId));

            da.SetDataList(
                1,
                combinations.Select(c => c.Name));

            var tree = new GH_Structure<GH_ObjectWrapper>();
            for (var i = 0; i < combinations.Count; i++)
            {
                var path = new GH_Path(i);
                tree.EnsurePath(path);
                if (combinations[i].ActiveDesignOptions != null)
                {
                    foreach (var option in combinations[i].ActiveDesignOptions)
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
            Properties.Resources.GetDesignOptionCombinations;

        public override Guid ComponentGuid =>
            new Guid("a07d0543-1291-4e00-9d1e-6d120bc47dfc");
    }
}
