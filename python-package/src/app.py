import tapir_py
from tapir_py import core, parts, utility

def get_children(classification):
    if isinstance(classification, parts.ClassificationSystem):
        print("GetAllClassificationsInSystem: {}".format(classification.name))
    elif isinstance(classification, parts.ClassificationItem):
        for child in classification.children:
            if child.children:
                t = "Branch"
            else:
                t = "Leaf"
            print("{}: Child: {}".format(t,child.id))
            
    else:
        print("Nothing to print for {}".format(type(classification)))

archicad = core.Command.create()
result = archicad.GetAllClassificationSystems()
#print(result[0].guid)
ins = archicad.GetAllClassificationsInSystem(result[0])

get_children(ins[0])

"""for system in ins:
    print("\t", system)
    if system.HasChildren:
        for child in system.children:
            print("\t", "\t", child)
            if child.HasChildren:
                for grandchild in child.children:
                    print("\t", "\t", "\t", grandchild)"""


