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
    public class GetElementsOfDesignOptionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsOfDesignOptions";

        public GetElementsOfDesignOptionsComponent()
            : base(
                "GetElementsOfDesignOptions",
                "Get the elements associated with the given design options. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionGuids",
                "Identifiers of the design options to query.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DesignOptionGuids",
                "Identifiers of the queried design options.");

            OutGenericTree(
                "ElementGuids",
                "Identifiers of the elements in each design option (one branch per design option).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out DesignOptionsObject designOptions))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    designOptions,
                    ToAddOn,
                    JHelp.Deserialize<GetElementsOfDesignOptionsResponse>,
                    out GetElementsOfDesignOptionsResponse response))
            {
                return;
            }

            var elementsOfDesignOptions =
                response.ElementsOfDesignOptions ?? new List<ElementsOfDesignOption>();

            da.SetDataList(
                0,
                elementsOfDesignOptions.Select(
                    e => new DesignOptionGuidWrapper { DesignOptionId = e.DesignOptionId }));

            var tree = new GH_Structure<GH_ObjectWrapper>();
            for (var i = 0; i < elementsOfDesignOptions.Count; i++)
            {
                var path = new GH_Path(i);
                tree.EnsurePath(path);
                if (elementsOfDesignOptions[i].Elements != null)
                {
                    foreach (var element in elementsOfDesignOptions[i].Elements)
                    {
                        tree.Append(
                            new GH_ObjectWrapper(element),
                            path);
                    }
                }
            }

            da.SetDataTree(
                1,
                tree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsOfDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("1006ae3b-2e73-40b6-9450-ca2f1649c249");
    }
}
