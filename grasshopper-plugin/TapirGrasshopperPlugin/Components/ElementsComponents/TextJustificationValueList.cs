using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    public class TextJustificationValueList : ValueList
    {
        public TextJustificationValueList()
            : base(
                "TextJustifications",
                "Value list for the justification of text elements.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: TextJustification.Left);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TextJustificationValueList;

        public override Guid ComponentGuid =>
            new Guid("c7e0b8a1-4d63-4a0e-8b5c-9f2a6d4e1b30");
    }
}
