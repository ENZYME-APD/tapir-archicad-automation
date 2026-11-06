using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class GetDesignOptionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDesignOptions";

        public GetDesignOptionsComponent()
            : base(
                "GetDesignOptions",
                "Get all design options of the project. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DesignOptionGuids",
                "Identifiers of the design options.");

            OutTexts(
                "Names",
                "Names of the design options.");

            OutTexts(
                "Ids",
                "String ids of the design options.");

            OutTexts(
                "OwnerSetNames",
                "Names of the owner design option sets.");

            OutBooleans(
                "IsEditable",
                "Whether each design option can be renamed, deleted or changed now (for example it is not reserved by someone else in Teamwork).");

            OutTexts(
                "NotEditableReasons",
                "The reason why each design option is not editable (empty when it is editable).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<GetDesignOptionsResponse>,
                    out GetDesignOptionsResponse response))
            {
                return;
            }

            var designOptions = response.DesignOptions ?? new List<DesignOptionDetails>();

            da.SetDataList(
                0,
                designOptions.Select(
                    o => new DesignOptionGuidWrapper { DesignOptionId = o.DesignOptionId }));

            da.SetDataList(
                1,
                designOptions.Select(o => o.Name));

            da.SetDataList(
                2,
                designOptions.Select(o => o.Id));

            da.SetDataList(
                3,
                designOptions.Select(o => o.OwnerSetName));

            da.SetDataList(
                4,
                designOptions.Select(o => o.IsEditable));

            da.SetDataList(
                5,
                designOptions.Select(o => o.NotEditableError?.Message));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("0718a135-f6e8-488a-8ff2-d20bb0fc1be5");
    }
}
