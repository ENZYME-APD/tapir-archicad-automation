using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateLayoutComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLayout";

        public CreateLayoutComponent()
            : base(
                "CreateLayout",
                "Create Layouts and their backing master layouts. " +
                "The master layout is given by name or by navigator item id.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "LayoutNames",
                "Names of the layouts to create.");

            InText(
                "MasterLayoutName",
                "Name of the master layout to use. Optional.");

            InGeneric(
                "MasterNavigatorItemGuid",
                "Identifier of the master layout navigator item to use. Optional.");

            InGeneric(
                "ParentNavigatorItemGuid",
                "Identifier of the Layout Book folder/subset to place the layouts in. Optional.");

            InTexts(
                "LayoutParameters",
                "One JSON object per layout with layout parameters " +
                "(e.g. {\"horizontalSize\":420,\"verticalSize\":297,\"leftMargin\":5}). " +
                "Input only 1 to use the same parameters for all layouts. Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> layoutNames))
            {
                return;
            }

            da.TryGet(
                1,
                out string masterLayoutName);

            NavigatorGuid masterId = null;
            if (da.TryGet(
                    2,
                    out GH_ObjectWrapper masterWrapper) &&
                masterWrapper?.Value != null)
            {
                masterId = GuidObject<NavigatorGuid>.CreateFromWrapper(masterWrapper);
                if (masterId == null)
                {
                    this.AddError("Invalid MasterNavigatorItemGuid.");
                    return;
                }
            }

            NavigatorGuid parentId = null;
            if (da.TryGet(
                    3,
                    out GH_ObjectWrapper parentWrapper) &&
                parentWrapper?.Value != null)
            {
                parentId = GuidObject<NavigatorGuid>.CreateFromWrapper(parentWrapper);
                if (parentId == null)
                {
                    this.AddError("Invalid ParentNavigatorItemGuid.");
                    return;
                }
            }

            da.TryGetList(
                4,
                out List<string> layoutParameters);
            layoutParameters = layoutParameters ?? new List<string>();

            if (layoutParameters.Count > 1 &&
                layoutParameters.Count != layoutNames.Count)
            {
                this.AddError(
                    "The size of the input LayoutParameters must be 0, 1 or equal to the size of the input LayoutNames.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < layoutNames.Count; i++)
            {
                var item = new JObject { ["layoutName"] = layoutNames[i] };

                if (!string.IsNullOrEmpty(masterLayoutName))
                {
                    item["masterLayoutName"] = masterLayoutName;
                }

                if (masterId != null)
                {
                    item["masterNavigatorItemId"] = new JObject { ["guid"] = masterId.Guid };
                }

                if (parentId != null)
                {
                    item["parentNavigatorItemId"] = new JObject { ["guid"] = parentId.Guid };
                }

                if (layoutParameters.Count > 0)
                {
                    var json = layoutParameters[layoutParameters.Count == 1 ? 0 : i];
                    try
                    {
                        item["layoutParameters"] = JObject.Parse(json);
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the LayoutParameters input: {ex.Message}");
                        return;
                    }
                }

                items.Add(item);
            }

            var parameters = new JObject { ["layoutsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLayout;

        public override Guid ComponentGuid =>
            new Guid("cc5247ba-7d7c-4190-845d-f11ed11b9c84");
    }
}
