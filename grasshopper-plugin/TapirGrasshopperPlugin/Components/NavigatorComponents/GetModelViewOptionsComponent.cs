using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetModelViewOptionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetModelViewOptions";

        public GetModelViewOptionsComponent()
            : base(
                nameof(ModelViewOptionsResponse.ModelViewOptions),
                "Gets all model view options.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(nameof(ModelViewOption.Name) + "s");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<ModelViewOptionsResponse>,
                    out ModelViewOptionsResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ModelViewOptions.Select(x => x.Name));
        }

        public override Guid ComponentGuid =>
            new Guid("e00d6e32-408a-4378-afe8-1eddd0ffcb5b");
    }
}