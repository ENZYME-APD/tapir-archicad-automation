using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCAssignmentsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetIFCAssignments";

        public GetIFCAssignmentsComponent()
            : base(
                "GetIFCAssignments",
                "Returns the IFC assignments of the project: the root IFC groups, and for each object the " +
                "objects assigned to it (children) and the objects it is assigned to (parents). Each object " +
                "is given by its GlobalId, its IFC type and its element where it belongs to one. " +
                "Available from Archicad 28.",
                GroupNames.IFC)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "RootGroupGlobalIds",
                "The IFC GlobalId of each IFC group (e.g. IfcGroup, IfcZone, IfcSystem) that is not " +
                "assigned to another group.");

            OutTexts(
                "RootGroupIFCTypes",
                "The IFC type of each root group.");

            OutTexts(
                "OwnerGlobalIds",
                "The IFC GlobalId of each object that has objects assigned to it.");

            OutTexts(
                "OwnerIFCTypes",
                "The IFC type of each object that has objects assigned to it.");

            OutGenerics(
                "OwnerElementGuids",
                "The element of each object that has objects assigned to it (null when it belongs to none).");

            OutGenericTree(
                "ChildGlobalIds",
                "The IFC GlobalId of the objects assigned to each owner, one branch per owner.");

            OutGenericTree(
                "ChildIFCTypes",
                "The IFC type of the objects assigned to each owner, one branch per owner.");

            OutGenericTree(
                "ChildElementGuids",
                "The element of the objects assigned to each owner, one branch per owner " +
                "(null when it belongs to none).");

            OutTexts(
                "AssignedGlobalIds",
                "The IFC GlobalId of each object that is assigned to other objects.");

            OutTexts(
                "AssignedIFCTypes",
                "The IFC type of each object that is assigned to other objects.");

            OutGenerics(
                "AssignedElementGuids",
                "The element of each object that is assigned to other objects (null when it belongs to none).");

            OutGenericTree(
                "ParentGlobalIds",
                "The IFC GlobalId of the objects each assigned object is assigned to, one branch per object.");

            OutGenericTree(
                "ParentIFCTypes",
                "The IFC type of the objects each assigned object is assigned to, one branch per object.");
        }

        private static string GlobalIdOf(
            JToken ifcObject)
        {
            return ifcObject?["globalId"]?.ToString();
        }

        private static string IFCTypeOf(
            JToken ifcObject)
        {
            return ifcObject?["ifcType"]?.ToString();
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var rootGlobalIds = new List<string>();
            var rootTypes = new List<string>();
            foreach (var group in JsonOutputHelp.Items(
                         response,
                         "rootGroups"))
            {
                rootGlobalIds.Add(GlobalIdOf(group));
                rootTypes.Add(IFCTypeOf(group));
            }

            var ownerGlobalIds = new List<string>();
            var ownerTypes = new List<string>();
            var ownerElements = new List<object>();
            var childGlobalIds = new DataTree<object>();
            var childTypes = new DataTree<object>();
            var childElements = new DataTree<object>();

            var childObjects = JsonOutputHelp.Items(
                response,
                "childObjects");
            for (var i = 0; i < childObjects.Count; i++)
            {
                var item = childObjects[i];
                var owner = item["object"];
                var path = new GH_Path(i);
                childGlobalIds.EnsurePath(path);
                childTypes.EnsurePath(path);
                childElements.EnsurePath(path);

                ownerGlobalIds.Add(GlobalIdOf(owner));
                ownerTypes.Add(IFCTypeOf(owner));
                ownerElements.Add(ElementsStructuredGetterComponent.ElementIdOf(owner));

                if (item["children"] is JArray children)
                {
                    foreach (var child in children)
                    {
                        childGlobalIds.Add(GlobalIdOf(child), path);
                        childTypes.Add(IFCTypeOf(child), path);
                        childElements.Add(ElementsStructuredGetterComponent.ElementIdOf(child), path);
                    }
                }
            }

            var assignedGlobalIds = new List<string>();
            var assignedTypes = new List<string>();
            var assignedElements = new List<object>();
            var parentGlobalIds = new DataTree<object>();
            var parentTypes = new DataTree<object>();

            var parentObjects = JsonOutputHelp.Items(
                response,
                "parentObjects");
            for (var i = 0; i < parentObjects.Count; i++)
            {
                var item = parentObjects[i];
                var assigned = item["object"];
                var path = new GH_Path(i);
                parentGlobalIds.EnsurePath(path);
                parentTypes.EnsurePath(path);

                assignedGlobalIds.Add(GlobalIdOf(assigned));
                assignedTypes.Add(IFCTypeOf(assigned));
                assignedElements.Add(ElementsStructuredGetterComponent.ElementIdOf(assigned));

                if (item["parents"] is JArray parents)
                {
                    foreach (var parent in parents)
                    {
                        parentGlobalIds.Add(GlobalIdOf(parent), path);
                        parentTypes.Add(IFCTypeOf(parent), path);
                    }
                }
            }

            da.SetDataList(0, rootGlobalIds);
            da.SetDataList(1, rootTypes);
            da.SetDataList(2, ownerGlobalIds);
            da.SetDataList(3, ownerTypes);
            da.SetDataList(4, ownerElements);
            da.SetDataTree(5, childGlobalIds);
            da.SetDataTree(6, childTypes);
            da.SetDataTree(7, childElements);
            da.SetDataList(8, assignedGlobalIds);
            da.SetDataList(9, assignedTypes);
            da.SetDataList(10, assignedElements);
            da.SetDataTree(11, parentGlobalIds);
            da.SetDataTree(12, parentTypes);
        }

        // Shares the icon of GetIFCTypeOfElements until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCTypeOfElements;

        public override Guid ComponentGuid =>
            new Guid("17eda2f5-70b6-46f6-ace1-3e29487043b2");
    }
}
