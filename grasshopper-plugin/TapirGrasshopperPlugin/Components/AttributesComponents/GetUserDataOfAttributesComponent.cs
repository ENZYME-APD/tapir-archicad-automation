using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetUserDataOfAttributesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetUserDataOfAttributes";

        public GetUserDataOfAttributesComponent()
            : base(
                "GetUserDataOfAttributes",
                "Reads the key-value pairs stored on attributes by SetUserDataOfAttributes. An attribute without " +
                "user data returns no pairs; one holding user data in another layout returns an error.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to read the user data of.");
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "Keys",
                "Keys of the pairs stored on each attribute (one branch per attribute).");

            OutTextTree(
                "Values",
                "Values of the pairs stored on each attribute (one branch per attribute).");

            OutErrorMessages(
                "Error message of each attribute (empty when its user data was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AttributeGuidItemsObject input))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = JsonOutputHelp.Items(
                response,
                "userDataOfAttributes");
            var keys = new DataTree<object>();
            var values = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                keys.EnsurePath(path);
                values.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (!(item["pairs"] is JArray pairs))
                {
                    continue;
                }

                foreach (var pair in pairs)
                {
                    keys.Add(
                        JsonOutputHelp.Scalar(pair, "key"),
                        path);
                    values.Add(
                        JsonOutputHelp.Scalar(pair, "value"),
                        path);
                }
            }

            da.SetDataTree(0, keys);
            da.SetDataTree(1, values);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUserDataOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("186c81e4-9a3c-46a4-986d-67a1fc420805");
    }
}
