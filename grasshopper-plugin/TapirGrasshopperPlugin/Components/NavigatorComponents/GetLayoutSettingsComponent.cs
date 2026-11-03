using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetLayoutSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLayoutSettings";

        public GetLayoutSettingsComponent()
            : base(
                "GetLayoutSettings",
                "Get settings of layouts, including their master layout and Layout Info Panel custom data fields. " +
                "Identify each layout either by its database or by its navigator item.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "LayoutDatabaseGuids",
                "Identifiers of the layout databases to query.");

            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the layout navigator items to query.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LayoutNames",
                "Name of each layout.");

            OutNumberList(
                "HorizontalSizes",
                "Horizontal size of each layout in mm.");

            OutNumberList(
                "VerticalSizes",
                "Vertical size of each layout in mm.");

            OutNumberList(
                "LeftMargins",
                "Left margin of each layout in mm.");

            OutNumberList(
                "TopMargins",
                "Top margin of each layout in mm.");

            OutNumberList(
                "RightMargins",
                "Right margin of each layout in mm.");

            OutNumberList(
                "BottomMargins",
                "Bottom margin of each layout in mm.");

            OutTexts(
                "CustomLayoutNumbers",
                "Custom layout number of each layout.");

            OutBooleans(
                "CustomLayoutNumbering",
                "True if the layout uses custom numbering.");

            OutBooleans(
                "DoNotIncludeInNumbering",
                "True if the layout is excluded from numbering.");

            OutBooleans(
                "DisplayMasterLayoutBelow",
                "True if the master layout is displayed below the layout.");

            OutGenerics(
                "MasterLayoutDatabaseGuids",
                "Database identifier of the master layout of each layout (null for master layouts and failed items).");

            OutGenerics(
                "MasterNavigatorItemGuids",
                "Navigator item identifier of the master layout of each layout (null for master layouts and failed items).");

            OutTexts(
                "MasterLayoutNames",
                "Name of the master layout of each layout (null for master layouts and failed items).");

            OutTextTree(
                "CustomDataKeys",
                "Key of each Layout Info Panel custom data field (one branch per layout).");

            OutTextTree(
                "CustomDataNames",
                "Name of each Layout Info Panel custom data field (one branch per layout).");

            OutTextTree(
                "CustomDataValues",
                "Value of each Layout Info Panel custom data field (one branch per layout).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried layout (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var items = new JArray();

            if (da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> dbWrappers))
            {
                foreach (var wrapper in dbWrappers)
                {
                    var id = GuidObject<DatabaseGuidObject>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid database identifier in the LayoutDatabaseGuids input.");
                        return;
                    }
                    items.Add(
                        new JObject
                        {
                            ["databaseId"] = new JObject { ["guid"] = id.Guid }
                        });
                }
            }

            if (da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> navWrappers))
            {
                foreach (var wrapper in navWrappers)
                {
                    var id = GuidObject<NavigatorGuid>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid navigator item identifier in the NavigatorItemGuids input.");
                        return;
                    }
                    items.Add(
                        new JObject
                        {
                            ["navigatorItemId"] = new JObject { ["guid"] = id.Guid }
                        });
                }
            }

            if (items.Count == 0)
            {
                this.AddError(
                    "Provide at least one LayoutDatabaseGuid or NavigatorItemGuid.");
                return;
            }

            var parameters = new JObject { ["layoutDatabaseIds"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var layoutNames = new List<object>();
            var horizontalSizes = new List<object>();
            var verticalSizes = new List<object>();
            var leftMargins = new List<object>();
            var topMargins = new List<object>();
            var rightMargins = new List<object>();
            var bottomMargins = new List<object>();
            var customLayoutNumbers = new List<object>();
            var customLayoutNumbering = new List<object>();
            var doNotIncludeInNumbering = new List<object>();
            var displayMasterLayoutBelow = new List<object>();
            var masterLayoutDatabaseGuids = new List<object>();
            var masterNavigatorItemGuids = new List<object>();
            var masterLayoutNames = new List<object>();
            var customDataKeys = new DataTree<object>();
            var customDataNames = new DataTree<object>();
            var customDataValues = new DataTree<object>();
            var errors = new List<string>();

            var settings = JsonOutputHelp.Items(
                response,
                "layoutSettings");
            for (var i = 0; i < settings.Count; i++)
            {
                var item = settings[i];
                var path = new GH_Path(i);
                customDataKeys.EnsurePath(path);
                customDataNames.EnsurePath(path);
                customDataValues.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    layoutNames.Add(null);
                    horizontalSizes.Add(null);
                    verticalSizes.Add(null);
                    leftMargins.Add(null);
                    topMargins.Add(null);
                    rightMargins.Add(null);
                    bottomMargins.Add(null);
                    customLayoutNumbers.Add(null);
                    customLayoutNumbering.Add(null);
                    doNotIncludeInNumbering.Add(null);
                    displayMasterLayoutBelow.Add(null);
                    masterLayoutDatabaseGuids.Add(null);
                    masterNavigatorItemGuids.Add(null);
                    masterLayoutNames.Add(null);
                    continue;
                }

                errors.Add("");
                layoutNames.Add(JsonOutputHelp.Scalar(item, "layoutName"));
                horizontalSizes.Add(JsonOutputHelp.Scalar(item, "horizontalSize"));
                verticalSizes.Add(JsonOutputHelp.Scalar(item, "verticalSize"));
                leftMargins.Add(JsonOutputHelp.Scalar(item, "leftMargin"));
                topMargins.Add(JsonOutputHelp.Scalar(item, "topMargin"));
                rightMargins.Add(JsonOutputHelp.Scalar(item, "rightMargin"));
                bottomMargins.Add(JsonOutputHelp.Scalar(item, "bottomMargin"));
                customLayoutNumbers.Add(JsonOutputHelp.Scalar(item, "customLayoutNumber"));
                customLayoutNumbering.Add(JsonOutputHelp.Scalar(item, "customLayoutNumbering"));
                doNotIncludeInNumbering.Add(JsonOutputHelp.Scalar(item, "doNotIncludeInNumbering"));
                displayMasterLayoutBelow.Add(JsonOutputHelp.Scalar(item, "displayMasterLayoutBelow"));

                // Master layouts have no master layout of their own, so these fields are absent for them.
                var masterDatabaseGuid = JsonOutputHelp.WrappedGuidOf(item, "masterLayoutDatabaseId");
                masterLayoutDatabaseGuids.Add(
                    masterDatabaseGuid == null
                        ? null
                        : new DatabaseGuidWrapper
                        {
                            DatabaseId = new DatabaseGuidObject { Guid = masterDatabaseGuid }
                        });
                var masterNavigatorGuid = JsonOutputHelp.WrappedGuidOf(item, "masterNavigatorItemId");
                masterNavigatorItemGuids.Add(
                    masterNavigatorGuid == null
                        ? null
                        : new NavigatorGuidWrapper
                        {
                            NavigatorId = new NavigatorGuid { Guid = masterNavigatorGuid }
                        });
                masterLayoutNames.Add(JsonOutputHelp.Scalar(item, "masterLayoutName"));

                if (item["customData"] is JArray customData)
                {
                    foreach (var field in customData)
                    {
                        customDataKeys.Add(
                            JsonOutputHelp.Scalar(field, "customSchemeKey"),
                            path);
                        customDataNames.Add(
                            JsonOutputHelp.Scalar(field, "customSchemeName"),
                            path);
                        customDataValues.Add(
                            JsonOutputHelp.Scalar(field, "customSchemeValue"),
                            path);
                    }
                }
            }

            da.SetDataList(0, layoutNames);
            da.SetDataList(1, horizontalSizes);
            da.SetDataList(2, verticalSizes);
            da.SetDataList(3, leftMargins);
            da.SetDataList(4, topMargins);
            da.SetDataList(5, rightMargins);
            da.SetDataList(6, bottomMargins);
            da.SetDataList(7, customLayoutNumbers);
            da.SetDataList(8, customLayoutNumbering);
            da.SetDataList(9, doNotIncludeInNumbering);
            da.SetDataList(10, displayMasterLayoutBelow);
            da.SetDataList(11, masterLayoutDatabaseGuids);
            da.SetDataList(12, masterNavigatorItemGuids);
            da.SetDataList(13, masterLayoutNames);
            da.SetDataTree(14, customDataKeys);
            da.SetDataTree(15, customDataNames);
            da.SetDataTree(16, customDataValues);
            da.SetDataList(17, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLayoutSettings;

        public override Guid ComponentGuid =>
            new Guid("305c2b76-1b7d-448e-9cd1-d4653dbbaa8b");
    }
}
