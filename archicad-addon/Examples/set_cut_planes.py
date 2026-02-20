from dataclasses import dataclass
import aclib


@dataclass
class CutPlane:
    """Defines a plane in 3D space using the plane equation: ax + by + cz + d = 0"""
    pa: float  # a (normal x)
    pb: float  # b (normal y)
    pc: float  # c (normal z)
    pd: float  # d (offset)


def cut_planes_from_aabb(box, offset=0.5):
    """4 Schnittebenen um die BBox"""
    return [
        CutPlane(1, 0, 0, box.get("xMax") + offset),
        CutPlane(-1, 0, 0, -box.get("xMin") + offset),
        CutPlane(0, 1, 0, box.get("yMax") + offset),
        CutPlane(0, -1, 0, -box.get("yMin") + offset),
    ]


def get_union_bbox(bboxes):
    union_box = None
    for bbox in bboxes:
        if union_box is None:
            union_box = bbox["boundingBox3D"]
        else:
            union_box = {
                "xMin": min(union_box["xMin"], bbox["boundingBox3D"]["xMin"]),
                "xMax": max(union_box["xMax"], bbox["boundingBox3D"]["xMax"]),
                "yMin": min(union_box["yMin"], bbox["boundingBox3D"]["yMin"]),
                "yMax": max(union_box["yMax"], bbox["boundingBox3D"]["yMax"]),
                "zMin": min(union_box["zMin"], bbox["boundingBox3D"]["zMin"]),
                "zMax": max(union_box["zMax"], bbox["boundingBox3D"]["zMax"]),
            }
    return union_box


if __name__ == "__main__":
    selected_elements = aclib.RunTapirCommand("GetSelectedElements", {})
    if selected_elements["elements"] == []:
        print("No elements selected.")
        raise SystemExit(1)

    bboxes = aclib.RunTapirCommand(
        "Get3DBoundingBoxes", selected_elements)["boundingBoxes3D"]
    offset = 0.5
    union_box = get_union_bbox(bboxes)
    planes = cut_planes_from_aabb(union_box, offset)
    coordinates = [{"pa": p.pa, "pb": p.pb, "pc": p.pc, "pd": p.pd}
                   for p in planes]

    response = aclib.RunTapirCommand(
        "Set3DCutPlanes",
        {"shapesCount": len(coordinates), "shapeCoordinates": coordinates},
        debug=True
    )
    print(response)
