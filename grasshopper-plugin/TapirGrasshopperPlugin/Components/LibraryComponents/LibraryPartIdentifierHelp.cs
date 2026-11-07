using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    // The library parts of the commands that take LibraryPartIdentifier items
    // (GetLibraryPartTypeDetails, IsLibraryPartSubtypeOf): each one is given by
    // exactly one of its index, its name or its unique ID.
    internal static class LibraryPartIdentifierHelp
    {
        public const string IndicesDescription =
            "Index of each loaded library part, as GetAvailableLibraryParts returns it. Indices may change when " +
            "the libraries are reloaded. Optional.";

        public const string NamesDescription =
            "Name of each loaded library part, as GetAvailableLibraryParts returns it in documentName (e.g. " +
            "'Chair 01'). Optional.";

        public const string UnIdsDescription =
            "Full unique ID of each library part, main and revision GUID as '{MAIN-GUID}-{REVISION-GUID}', as " +
            "GetLibraryPartIdentities returns it. Optional.";

        // The identifiers of the Indices, Names and UnIds list inputs starting
        // at firstInput, in that order.
        public static JArray Identifiers(
            IGH_DataAccess da,
            int firstInput)
        {
            var libraryParts = new JArray();
            foreach (var index in da.OptionalList<int>(firstInput))
            {
                libraryParts.Add(new JObject { ["index"] = index });
            }

            foreach (var name in da.OptionalList<string>(firstInput + 1))
            {
                libraryParts.Add(new JObject { ["name"] = name });
            }

            foreach (var unId in da.OptionalList<string>(firstInput + 2))
            {
                libraryParts.Add(new JObject { ["unId"] = unId });
            }

            return libraryParts;
        }
    }
}
