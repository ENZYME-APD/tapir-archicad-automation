import aclib

revision_issues = aclib.RunTapirCommand('GetRevisionIssues')
revision_changes = aclib.RunTapirCommand('GetRevisionChanges')
document_revisions = aclib.RunTapirCommand('GetDocumentRevisions')

def get_navigator_item_guids_from_tree(current_branch, navigator_item_guids, navigator_item_type):
    for navigator_item in current_branch:
        navigator_item = navigator_item['navigatorItem']
        if navigator_item['type'] == navigator_item_type:
            navigator_item_guids.append(navigator_item['navigatorItemId'])
        children = navigator_item.get('children')
        if children:
            get_navigator_item_guids_from_tree(children, navigator_item_guids, navigator_item_type)


navigator_tree_id = {'navigatorTreeId': {'type': 'LayoutBook'}}

navigator_tree = aclib.RunCommand('API.GetNavigatorItemTree', navigator_tree_id)

layout_guids = []
get_navigator_item_guids_from_tree(navigator_tree['navigatorTree']['rootItem']['children'], layout_guids, navigator_item_type='LayoutItem')

layout_elements = [{'navigatorItemId': guid} for guid in layout_guids]
layout_databases = aclib.RunTapirCommand('GetDatabaseIdFromNavigatorItemId', {'navigatorItemIds': layout_elements})

current_revision_changes_of_layouts = aclib.RunTapirCommand('GetCurrentRevisionChangesOfLayouts',
                                                            {'layoutDatabaseIds': layout_databases['databases']})
