using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Types.Attributes
{
    // An attribute folder as the folder commands of the add-on take it: the
    // type of the attributes it holds and its path, the folder names from the
    // root folder of that type down to it (empty for the root folder itself).
    public class AttributeFolder
    {
        // The components take and hand out a path as one text with the folder
        // names joined by this separator, like the FolderPaths output of
        // AttributesByType does.
        public const char PathSeparator = '/';

        [JsonProperty("attributeType")]
        public string AttributeType;

        [JsonProperty("path")]
        public List<string> Path;

        public AttributeFolder()
        {
        }

        public AttributeFolder(
            string attributeType,
            string pathText)
        {
            AttributeType = attributeType;
            Path = SplitPath(pathText);
        }

        // "Structure/Walls" -> ["Structure", "Walls"]. The names are trimmed
        // and empty names, as left by a leading, trailing or doubled
        // separator, are dropped, so an empty or blank text is the root folder.
        public static List<string> SplitPath(
            string pathText)
        {
            if (string.IsNullOrWhiteSpace(pathText))
            {
                return new List<string>();
            }

            return pathText
                .Split(PathSeparator)
                .Select(name => name.Trim())
                .Where(name => name.Length > 0)
                .ToList();
        }
    }

    public class AttributeFolderToRename : AttributeFolder
    {
        [JsonProperty("newName")]
        public string NewName;

        public AttributeFolderToRename(
            string attributeType,
            string pathText,
            string newName)
            : base(
                attributeType,
                pathText)
        {
            NewName = newName;
        }
    }

    // Input of CreateAttributeFolders and DeleteAttributeFolders.
    public class AttributeFoldersParameters
    {
        [JsonProperty("attributeFolders")]
        public List<AttributeFolder> AttributeFolders;
    }

    public class RenameAttributeFoldersParameters
    {
        [JsonProperty("attributeFolders")]
        public List<AttributeFolderToRename> AttributeFolders;
    }

    public class MoveAttributesAndFoldersParameters
    {
        // Both lists are optional in the command, so an empty one is left out.
        [JsonProperty("attributeIds", NullValueHandling = NullValueHandling.Ignore)]
        public List<AttributeGuidWrapper> AttributeIds;

        [JsonProperty("attributeFolders", NullValueHandling = NullValueHandling.Ignore)]
        public List<AttributeFolder> AttributeFolders;

        [JsonProperty("targetFolder")]
        public AttributeFolder TargetFolder;
    }
}
