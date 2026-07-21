using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateLayoutSubsetComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateLayoutSubset";

        public CreateLayoutSubsetComponent()
            : base(
                "CreateLayoutSubset",
                "Create Layout Book subsets.",
                GroupNames.Navigator,
                "subsetsData",
                "SubsetsData",
                "One JSON object per subset, e.g. " +
                "{\"name\":\"My Subset\",\"parentNavigatorItemId\":{\"guid\":\"...\"}," +
                "\"numberingStyle\":\"1\",\"startAt\":1,\"customNumbering\":false}.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLayoutSubset;

        public override Guid ComponentGuid =>
            new Guid("c44f0d60-2085-436c-9a53-bcfb9f7ad77f");
    }
}
