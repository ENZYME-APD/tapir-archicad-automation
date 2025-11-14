using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetModelViewOptionsComponent : ArchicadAccessorComponent
    {
        public static string CommandName => "GetModelViewOptions";

        public GetModelViewOptionsComponent () : base (
                  nameof (ModelViewOptionsResponse.ModelViewOptions),
                  nameof (ModelViewOptionsResponse.ModelViewOptions),
                  ModelViewOptionsResponse.Doc,
                  GroupNames.Navigator )
        { }

        protected override void RegisterInputParams (GH_InputParamManager pManager) { }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(nameof(ModelViewOption.Name) + "s", "", "", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            if (!GetResponse(CommandName, null, out ModelViewOptionsResponse response)) return;

            DA.SetDataList(0, response.ModelViewOptions.Select(x => x.Name));
        }

        public override Guid ComponentGuid => new Guid("e00d6e32-408a-4378-afe8-1eddd0ffcb5b");
    }
}
