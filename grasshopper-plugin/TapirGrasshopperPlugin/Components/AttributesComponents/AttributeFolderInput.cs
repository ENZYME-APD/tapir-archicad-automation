using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    // The attribute folder commands identify a folder by its attribute type and
    // its path: the folder names from the root folder down, the root folder
    // itself not included. A Grasshopper tree holds one path per branch.
    internal static class AttributeFolderInput
    {
        public const string AttributeTypesDescription =
            "Attribute type of each folder (input only 1 to use the same type for all): Layer, Line, Fill, " +
            "Composite, Surface, LayerCombination, ZoneCategory, Profile, PenTable, MEPSystem, " +
            "OperationProfile or BuildingMaterial.";

        public const string PathsDescription =
            "Path of each folder, one branch per folder: the folder names from the root folder down, " +
            "the root folder itself not included.";

        // One folder name list per branch; empty items are dropped, so an
        // empty branch is the root folder.
        public static List<List<string>> Paths(
            GH_Structure<GH_String> tree)
        {
            var paths = new List<List<string>>();
            if (tree == null)
            {
                return paths;
            }

            foreach (var branch in tree.Branches)
            {
                paths.Add(
                    branch
                        .Where(x => x != null && !string.IsNullOrEmpty(x.Value))
                        .Select(x => x.Value)
                        .ToList());
            }

            return paths;
        }

        public static JObject Folder(
            string attributeType,
            List<string> path)
        {
            return new JObject
            {
                ["attributeType"] = attributeType,
                ["path"] = new JArray(path)
            };
        }

        // Reads the AttributeTypes list and the Paths tree into the items of an
        // "attributeFolders" array; false (with an error on the component) when
        // they do not match.
        public static bool TryGetFolders(
            GH_ActiveObject component,
            IGH_DataAccess da,
            int typesIndex,
            int pathsIndex,
            out List<JObject> folders)
        {
            folders = new List<JObject>();

            if (!da.TryGetList(
                    typesIndex,
                    out List<string> attributeTypes))
            {
                return false;
            }

            if (!da.TryGetTree(
                    pathsIndex,
                    out GH_Structure<GH_String> pathTree))
            {
                return false;
            }

            var paths = Paths(pathTree);
            if (paths.Count == 0)
            {
                component.AddError("The input Paths is empty.");
                return false;
            }

            if (attributeTypes.Count != 1 &&
                attributeTypes.Count != paths.Count)
            {
                component.AddError(
                    "The size of the input AttributeTypes must be 1 or equal to the number of branches in the input Paths.");
                return false;
            }

            for (var i = 0; i < paths.Count; i++)
            {
                folders.Add(
                    Folder(
                        attributeTypes[attributeTypes.Count == 1 ? 0 : i],
                        paths[i]));
            }

            return true;
        }
    }
}
