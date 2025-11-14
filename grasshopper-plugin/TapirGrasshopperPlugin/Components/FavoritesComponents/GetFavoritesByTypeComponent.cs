//using Grasshopper.Kernel;
//using Newtonsoft.Json;
//using System;

//namespace TapirGrasshopperPlugin.Components.ProjectComponents
//{
//    public class GetFavoritesByTypeComponent : ArchicadAccessorComponent
//    {
//        public static string CommandName => "GetFavoritesByType";

//        public GetFavoritesByTypeComponent ()
//            : base (
//                CommandName,
//                CommandName,
//                "Project"
//            )
//        { }

//        protected override void RegisterInputParams (GH_InputParamManager pManager) { }

//        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
//        {
//            pManager.AddTextParameter (nameof (Hotlink.Location) + "List", "", "", GH_ParamAccess.list);
//            pManager.AddTextParameter ("JsonHierarchy", "", "", GH_ParamAccess.item);
//        }

//        protected override void Solve (IGH_DataAccess DA)
//        {
//            if (!GetResponse (CommandName, null, out HotlinksResponse response)) return;

//            DA.SetDataList (0, response.Hotlinks.GetLocationsRecursively ());
//            DA.SetData (1, JsonConvert.SerializeObject (response, Formatting.Indented));
//        }

//        protected override System.Drawing.Bitmap Icon => Properties.Resources.ProjectLocation;

//        public override Guid ComponentGuid => new Guid ("a0b9722f-bc40-4ac3-afbc-d93e21dd8975");
//    }
//}
