namespace TapirGrasshopperPlugin.ResponseTypes.IdObjects
{
    public interface IFromGhWrapper
    {
        bool TryCreateFromGhWrapper(
            object wrapper);
    }
}