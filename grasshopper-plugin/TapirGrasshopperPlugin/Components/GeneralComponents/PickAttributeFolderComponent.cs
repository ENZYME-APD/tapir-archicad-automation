using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.AttributesComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickAttributeFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickAttributeFolder";

        public PickAttributeFolderComponent()
            : base(
                "PickAttributeFolder",
                "Opens a small dialog with Archicad's attribute folder picker for the given attribute type, waits " +
                "for the user and returns the chosen folder as its path from the root folder, or cancelled. " +
                "Archicad waits for the user, so it only runs on the Execute button. Available from Archicad 26.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "The type of the attributes whose folders the picker lists: Layer, BuildingMaterial, Composite, " +
                "Profile or Surface; Fill and Line need Archicad 27 or newer.");

            InText(
                "Title",
                "The title of the dialog. Optional; default: Pick Attribute Folder.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user closed the dialog with Cancel, false when with OK.");

            OutText(
                "AttributeType",
                "The attribute type of the chosen folder. Empty when cancelled.");

            OutTexts(
                "Path",
                "The chosen folder: the folder names from the root folder down, the root folder itself not " +
                "included (empty for the root folder), as the attribute folder components take a path.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<AttributeTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string attributeType))
            {
                return;
            }

            var parameters = new JObject { ["attributeType"] = attributeType };

            string title = null;
            if (da.GetData(1, ref title) &&
                !string.IsNullOrEmpty(title))
            {
                parameters["title"] = title;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                (bool?)response["cancelled"]);

            var folder = response["attributeFolder"];
            da.SetData(
                1,
                folder?["attributeType"]?.ToString());

            var path = new List<string>();
            if (folder?["path"] is JArray names)
            {
                foreach (var name in names)
                {
                    path.Add(name.ToString());
                }
            }

            da.SetDataList(
                2,
                path);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PickAttributeFolder;

        public override Guid ComponentGuid =>
            new Guid("6b08bbfb-5d0d-43e9-a978-5346d3d594a8");
    }
}
