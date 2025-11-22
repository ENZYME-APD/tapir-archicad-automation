using System;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class IssueElementTypeValueList : ValueList
    {
        public static string Doc => "Value List for Issue Elements Types.";

        public IssueElementTypeValueList()
            : base(
                nameof(IssueElementType),
                "Value List for Issue Elements Types.",
                GroupNames.Issues)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: IssueElementType.Highlight);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.IssueElementType;

        public override Guid ComponentGuid =>
            new Guid("b40e2d8e-4aa4-467e-b0f8-af2f65fb2df4");
    }
}