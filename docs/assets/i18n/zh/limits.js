/* 中文 (简体): the API limits page (limits/index.html). The Evidence column (headers and function names)
   stays English. See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'What the Archicad API can\'t do | Tapir': 'Archicad API 做不到的事 | Tapir',
    'What people ask for that Graphisoft\'s Archicad API does not offer, such as undo or the clipboard, each with what to do instead and the DevKit evidence.':
        '大家提出、而 Graphisoft 的 Archicad API 并未提供的功能，例如撤销或剪贴板，每项都附有替代办法和 DevKit 中的依据。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API limits': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / API 限制',
    'What the Archicad API <em>can\'t do.</em>': 'Archicad API <em>做不到的事。</em>',
    'Tapir uses the Archicad API that Graphisoft publishes for add-ons, and covers all of it that can be a JSON command. What that API does not offer, Tapir cannot offer either. This page lists what people ask for that is not in it, with what to do instead.':
        'Tapir 使用 Graphisoft 为插件发布的 Archicad API，并覆盖了其中所有能做成 JSON 命令的部分。这个 API 没有提供的，Tapir 也无法提供。本页列出大家想要但 API 中没有的功能，以及可以改用的办法。',

    // The list
    'The <em>list.</em>': '<em>清单。</em>',
    'It is also what Tapir would like the Archicad API to offer, and the list Tapir sends to Graphisoft. Every row was checked in the headers of the Archicad API DevKits of Archicad 25 to 30, the versions Tapir is built for; the Evidence column names the headers and functions.':
        '这也是 Tapir 希望 Archicad API 提供的功能，以及 Tapir 发给 Graphisoft 的清单。每一行都在 Archicad 25 到 30（Tapir 所构建的版本）的 Archicad API DevKit 头文件中核实过；“依据”一列列出了相关的头文件和函数。',
    'What people want': '大家想要的',
    'Status': '状态',
    'What to do instead': '替代办法',
    'Evidence': '依据',
    'Issue': 'Issue',
    'Not possible': '无法实现',
    'Partly possible': '部分可行',
    'Workaround': '有变通办法',
    'Nothing yet': '暂无',
    'Not documented': '无文档说明',

    // Elements
    'Elements': '元素',
    'Explode an element into its parts, as Edit &gt; Reshape &gt; Explode into Current View does':
        '像 Edit &gt; Reshape &gt; Explode into Current View 那样把元素分解为各个部分',
    'A camera with a name of its own, or an axonometric camera': '有自己名称的相机，或轴测相机',
    'Name the Camera Set the camera is in. For an axonometric view, set the 3D window with <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.':
        '为相机所在的 Camera Set 命名。要得到轴测视图，请用 <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a> 设置 3D 窗口。',
    'Remove one direction of a trim between two roofs or shells that trim each other':
        '只删除两个相互修剪的屋顶或壳体之间修剪的一个方向',
    'Remove the trim with <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, which removes both directions, then trim the other direction again with <a href="../archicad-addon/#TrimElements">TrimElements</a>.':
        '用 <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a> 删除修剪（会删除两个方向），然后用 <a href="../archicad-addon/#TrimElements">TrimElements</a> 重新修剪另一个方向。',
    'Read which slab or stair edge a railing is linked to, or link a railing to one':
        '读取栏杆关联到哪个楼板或楼梯的边缘，或将栏杆关联到某个边缘',
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        '关闭栏杆的自动楼层可见性，以便设置它显示在哪些楼层',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        '设置网面的 Floor Plan Display、Show Projection、未剖切线和上方线，或剖切线的线型',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        '读取或设置栏杆的平面图符号：其折断线、显示哪些符号构件、它们的二维属性以及符号覆盖',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        '用 <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> 读取楼板的轮廓，再用 <a href="../archicad-addon/#CreateRailings">CreateRailings</a> 沿该边缘创建静态栏杆。楼板改变时，删除栏杆并重新创建。',
    'Move a window or door into another wall': '把窗或门移到另一面墙中',
    'Delete it and create it in the other wall with <a href="../archicad-addon/#CreateWindows">CreateWindows</a> or <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; a favorite (<code>favoriteName</code>) gives it the same settings.':
        '删除它，然后用 <a href="../archicad-addon/#CreateWindows">CreateWindows</a> 或 <a href="../archicad-addon/#CreateDoors">CreateDoors</a> 在另一面墙中重新创建；收藏夹（<code>favoriteName</code>）可赋予它相同的设置。',
    'Move a skylight onto another roof or shell': '把天窗移到另一个屋顶或壳体上',
    'Delete it and create it on the other roof or shell with <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.':
        '删除它，然后用 <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a> 在另一个屋顶或壳体上重新创建。',
    'Turn an automatic zone into a manual one': '把自动区域改为手动区域',
    'Delete the zone and create it again as a manual zone, from its polygon, with <a href="../archicad-addon/#CreateZones">CreateZones</a>.':
        '删除该区域，然后用 <a href="../archicad-addon/#CreateZones">CreateZones</a> 按其多边形重新创建为手动区域。',
    'Change the height of a slanted curtain wall': '修改倾斜幕墙的高度',
    'Create it again with <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, with its <code>angle</code> and the new <code>height</code>.':
        '用 <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a> 重新创建，给出其 <code>angle</code> 和新的 <code>height</code>。',
    'Move the lines or the arc of an angle dimension': '移动角度标注的线或弧',
    'Delete it and create it again with <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.':
        '删除它，然后用 <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a> 重新创建。',
    'Set the position or the offset of a stair\'s walking line': '设置楼梯行走线的位置或偏移',
    'Anchor the sill or header of a window or door to the project zero or to a reference level': '将窗或门的窗台或过梁定位到项目零点或参考标高',
    'Anchor it to a story with <code>verticalLink</code> <code>SillToStory</code> or <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> or <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, and work out the height from the story level (<a href="../archicad-addon/#GetStories">GetStories</a>).':
        '用 <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> 或 <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a> 中的 <code>verticalLink</code> <code>SillToStory</code> 或 <code>HeaderToStory</code> 将其锚定到楼层，再根据楼层标高（<a href="../archicad-addon/#GetStories">GetStories</a>）算出高度。',
    'Read or set the joint type (Vertical Cut and the others) and the dx offset of a stair structure':
        '读取或设置楼梯结构的连接类型（Vertical Cut 等）和 dx 偏移',
    'Change the baseline of an existing stair (move a flight end, a landing or a winder) and keep its GUID':
        '更改现有楼梯的基线（移动梯段端点、休息平台或转角踏步），并保留其 GUID',
    'Recreate the stair on the new baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, which keeps its settings, Element ID, classifications, categories, properties and solid element operations, and returns the old and the new GUID. Its labels are lost.':
        '用 <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> 在新基线上重新创建楼梯，它会保留楼梯的设置、Element ID、分类、类别、属性和实体元素操作，并返回旧的和新的 GUID。楼梯的标签会丢失。',
    'Set the left and right boundaries or the walking line of a stair (a custom outline, or a walking line off the middle of the flight)':
        '设置楼梯的左右边界或行走线（自定义轮廓，或不在梯段中间的行走线）',
    'Archicad lays them out from the baseline and the flight width: change the baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> or the flight width with <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. The walking line stays in the middle of the flight. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> reads them.':
        'Archicad 根据基线和梯段宽度生成它们：用 <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> 更改基线，或用 <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a> 更改梯段宽度。行走线始终位于梯段中间。<a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> 可以读取它们。',
    'Set a label’s optimised position, “use text pen as uniform pen” of a symbol label, or a perpendicular text direction':
        '设置标签的优化位置、符号标签的“use text pen as uniform pen”，或垂直的文字方向',
    'Set the text direction (parallel, horizontal, vertical or a custom angle) with textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> and <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. When the symbol label’s library part has a parameter for the other two, set it with <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; otherwise set them in the Label tool settings or a favorite, and create the labels with favoriteName.':
        '用 <a href="../archicad-addon/#CreateLabels">CreateLabels</a> 和 <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a> 中的 textWay 设置文字方向（平行、水平、垂直或自定义角度）。如果符号标签的库部件有对应另外两项的参数，请用 <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a> 设置；否则请在标签工具设置或某个收藏中设置，并用 favoriteName 创建标签。',
    'Create a radial dimension of an arc, a circle, a curved wall or a curved slab edge': '为圆弧、圆、弧形墙或弧形楼板边缘创建半径标注',
    'Place the radial dimension in Archicad and read it with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, which returns how Archicad stored it. Those readings are what a create command needs.':
        '在 Archicad 中放置半径标注，再用 <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> 读取它，它会返回 Archicad 存储该标注的方式。创建命令需要的正是这些读数。',

    // Design options
    'Design options': '设计选项',
    'Read which design option, or the Main Model, is the default that new elements go to': '读取新元素默认放入哪个设计选项（或 Main Model）',
    'Create a temporary element, read its design option with <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, then move it to the Main Model with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (a null GUID as <code>designOptionId</code>) and delete it with <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; an element in an option that is not shown cannot be deleted. Set the default with <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.':
        '创建一个临时元素，用 <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a> 读取它的设计选项，然后用 <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> 把它移到 Main Model（<code>designOptionId</code> 用空 GUID），再用 <a href="../archicad-addon/#DeleteElements">DeleteElements</a> 删除它；位于未显示选项中的元素无法删除。用 <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a> 设置默认选项。',
    'Read or change which design options the current window shows, as the eye icons of the Design Options palette do':
        '读取或更改当前窗口显示哪些设计选项，就像 Design Options 面板中的眼睛图标那样',
    'Work with saved views: read and set the combination a View Map view shows with <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> and <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, and the options of a combination with <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.':
        '请使用保存的视图：用 <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> 和 <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a> 读取和设置 View Map 视图显示的组合，用 <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a> 设置组合中的选项。',
    'Duplicate a design option or a design option set, as the duplicate button of the Design Options palette does':
        '复制设计选项或设计选项集，就像 Design Options 面板中的复制按钮那样',
    'Create the set and its options with <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> and <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. For the elements, list them with <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copy them with <a href="../archicad-addon/#CopyElements">CopyElements</a> and move the copies to the new option with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.':
        '用 <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> 和 <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a> 创建选项集及其选项。对于元素，用 <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a> 列出它们，用 <a href="../archicad-addon/#CopyElements">CopyElements</a> 复制，再用 <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> 把副本移到新选项中。',
    'Lock or colour the passive elements of design options, as the Design Options palette does':
        '锁定或着色设计选项中的被动元素，就像 Design Options 面板那样',

    // Navigator and views
    'Navigator and views': '导航器与视图',
    'Add an item to a publisher set': '向发布集（publisher set）添加项目',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        '在 Archicad 的 Organizer 中添加项目，然后用 <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> 发布该集。',
    'Rename a publisher set': '重命名发布器集',
    'Rename the set in Archicad\'s Organizer. <a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> reads the names, and <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> publishes a set by its name.':
        '在 Archicad 的管理器中重命名该集。<a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> 读取名称，<a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> 按名称发布一个集。',
    'Change, rename or create a stored Model View Options combination': '修改、重命名或新建已保存的 Model View Options 组合',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        '用 <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> 读取这些组合；用 <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a> 修改当前窗口的选项。',
    'Make a 3D style the current one': '将某个 3D 样式设为当前样式',
    'Give a view the 3D style with <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) and open that view with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, from Archicad 27).':
        '用 <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a>（<code>d3styleName</code>）为某个视图指定该 3D 样式，再用 <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a>（<code>navigatorItemId</code>，Archicad 27 起）打开该视图。',
    'Read or set the floor plan cut plane or the structural analytical model settings saved in a view':
        '读取或设置视图中保存的平面图剖切面或结构分析模型设置',
    'Set them in the view\'s View Settings in Archicad. The project\'s floor plan cut plane can be set with <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.':
        '请在 Archicad 中该视图的 View Settings 里设置。项目的平面图剖切面可以用 <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a> 设置。',
    'Tell a clone folder of the View Map from a normal folder': '区分 View Map 中的克隆文件夹和普通文件夹',
    'Give the clone folders a name or ID of their own and find them by it in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.':
        '给克隆文件夹起一个专属的名称或 ID，然后在 <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a> 中按它查找。',
    'Change the settings of an existing Layout Book subset (prefix, numbering style, start at, ...)':
        '更改 Layout Book 中现有子集的设置（前缀、编号样式、起始值、...）',
    'Read the subset with <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create a subset with the new settings with <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> and move the layouts into it with <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. The old, empty subset is deleted in Archicad.':
        '用 <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a> 读取子集，用 <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> 按新设置创建子集，再用 <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a> 把布图移入其中。旧的空子集需在 Archicad 中删除。',

    // Project and files
    'Project and files': '项目与文件',
    'Undo or redo, as Edit &gt; Undo does': '像 Edit &gt; Undo 那样撤销或重做',
    'Most Tapir commands change the model in an undo step, so the user can undo them in Archicad; the command reference says which cannot be undone. Save the project with <a href="../archicad-addon/#SaveProject">SaveProject</a> before a script that may need to be taken back.':
        '大多数 Tapir 命令在一个撤销步骤中修改模型，因此用户可以在 Archicad 中撤销它们；命令参考注明了哪些无法撤销。在运行可能需要撤回的脚本之前，先用 <a href="../archicad-addon/#SaveProject">SaveProject</a> 保存项目。',
    'Copy and paste through the clipboard': '通过剪贴板复制和粘贴',
    'In a project, copy 2D elements to another view with <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Between projects, save the elements as a module with <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, place it in the other project with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, and, to keep plain elements, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        '在同一项目中，用 <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a> 把 2D 元素复制到另一个视图。在项目之间，用 <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a> 把元素保存为模块，用 <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> 和 <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> 将其放入另一个项目；如要保留为普通元素，再用 <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>。',
    'Merge a file into the project, as File &gt; Interoperability &gt; Merge does': '像 File &gt; Interoperability &gt; Merge 那样把文件合并到项目中',
    'Place the file as a hotlink with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, then <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        '用 <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> 和 <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> 把文件作为热链接放入，然后用 <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>。',
    'Delete one offset modifier of a profile': '删除轮廓的某一个偏移修改器',
    'Rebuild the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>\' <code>replaceSkins</code> and <code>newSkins</code>, which drops every modifier, then add the ones to keep again with <code>newProfileModifiers</code> (Archicad 27 and later).':
        '用 <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> 的 <code>replaceSkins</code> 和 <code>newSkins</code> 重建轮廓（这会删除所有修改器），然后用 <code>newProfileModifiers</code> 重新添加要保留的修改器（Archicad 27 及以后）。',
    'Import an IFC translator from a file into the project, or export one to a file': '将 IFC 转换器从文件导入项目，或将其导出到文件',
    'Keep the office translator in the project template, so new projects have it, and bring it into older projects once with Import… in the IFC Translators dialog. A script can check that a project has it with <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> and export with it by name with <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.':
        '把事务所的转换器放在项目模板中，让新项目都带有它；对旧项目，在 IFC Translators 对话框中用 Import… 导入一次。脚本可以用 <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> 检查项目中是否有它，并用 <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a> 按名称使用它导出。',

    // Documents and publishing
    'Documents and publishing': '文档与发布',
    'Read the rows of an Interactive Schedule': '读取 Interactive Schedule 的行',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        '自己生成清单：用 <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> 查找元素，再用 <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> 或 <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a> 读取它们的值。<a href="../scripts/">内置脚本</a> Room Finishes Schedule 和 Material Take-Off 就会生成这样的清单。',
    'Export an Interactive Schedule': '导出 Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        '先在 Archicad 中把该清单放进一个发布集（只需一次），然后用 <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> 发布该集。',
    'Choose the output format (PDF, DWG, DXF …) of a publisher set per call': '每次调用时选择发布集的输出格式（PDF、DWG、DXF …）',
    'Make one publisher set per format in Archicad’s Organizer once, then publish the one you need with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; its answer lists the files written (<code>writtenFiles</code>).':
        '在 Archicad 的 Organizer 中为每种格式各建一个发布集（只需一次），然后用 <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> 发布需要的那个；它的返回结果会列出写出的文件（<code>writtenFiles</code>）。',
    'Create a revision Change without placing a Change Marker, or change the ID or description of an existing Change':
        '在不放置 Change Marker 的情况下创建修订 Change，或更改现有 Change 的 ID 或说明',
    'Create the Change together with its first marker with <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). To change an existing Change, use Archicad’s Change Manager.':
        '用 <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a>（createChange）在创建第一个标记的同时创建 Change。要更改现有的 Change，请使用 Archicad 的 Change Manager。',
    'Set the custom scheme fields of a revision Change': '设置修订 Change 的自定义方案字段',
    'Fill them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> reads them back.':
        '请在 Archicad 的 Change Manager 中填写；<a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> 可以读回它们。',
    'Add elements to a revision Change, or remove them from it': '向修订 Change 添加元素，或从中移除元素',
    'Add them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> reads which Changes an element belongs to.':
        '请在 Archicad 的 Change Manager 中添加；<a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> 可以读取元素属于哪些 Change。',

    // Library parts and GDL
    'Library parts and GDL': '库部件与 GDL',
    'Run a 2D GDL script given as text, as <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> does with a 3D script':
        '运行以文本给出的 2D GDL 脚本，就像 <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> 对 3D 脚本所做的那样',
    'Save the script in a library part with <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (or <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), then draw its 2D symbol with <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.':
        '用 <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a>（或 <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>）把脚本保存到库部件中，再用 <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> 绘制它的 2D 符号。',
    'Get the error message and the line number when a GDL script fails':
        'GDL 脚本失败时获取错误信息和行号',
    'Check whether the script runs with <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) or <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). To see the error, open the library part in Archicad’s GDL editor with <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (from Archicad 27).':
        '用 <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a>（3D）或 <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>（2D）检查脚本能否运行。要查看错误，用 <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> 在 Archicad 的 GDL 编辑器中打开该库部件（Archicad 27 起）。',

    // Archicad's interface
    'Archicad\'s interface': 'Archicad 的界面',
    'Read the text of the Session Report window': '读取 Session Report 窗口中的文本',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        '用 <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> 并设置 windowType \'Report\' 为用户打开该窗口（Archicad 26 起）。',
    'Get the outcome of an Action Center check': '获取 Action Center 检查的结果',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        '用 <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> 运行检查；发现问题时 Archicad 会显示 Action Center。对于热链接检查，用 <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> 读取每个源的状态（Archicad 27 起）。',
    'Load a new add-on (.apx or .bundle) while Archicad runs, from an add-on or a script':
        '在 Archicad 运行时，从插件或脚本中加载新的插件（.apx 或 .bundle）',
    'Add it in Archicad\'s Add-On Manager (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). An add-on that is already loaded is replaced only after Archicad quits, as the Tapir Installer does when it updates Tapir.':
        '请在 Archicad 的 Add-On Manager 中添加（Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add）。已加载的插件只有在 Archicad 退出后才能被替换，Tapir Installer 更新 Tapir 时就是这样做的。',

    'Set the profile modifier values, or the stretched width and height, of a wall, beam or column that uses a profile':
        '设置使用轮廓的墙、梁或柱的轮廓修改器值，或其拉伸后的宽度和高度',
    'Read them with <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. To change them, change the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (a copy for these elements, or the profile itself), or set them in the settings of the element in Archicad.':
        '用 <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a> 读取它们。要更改它们，请用 <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> 修改轮廓（为这些构件建一个副本，或修改轮廓本身），或在 Archicad 中构件的设置里设置。',
    'Read or set how a grid element is shown in sections, elevations and interior elevations: its vertical extent and its markers at the top and bottom':
        '读取或设置轴网元素在剖面、立面和室内立面中的显示方式：其竖向范围以及顶部和底部的标记',
    'Select the grid elements in Archicad and set it for all of them at once in their settings dialog.':
        '在 Archicad 中选择这些轴网元素，并在其设置对话框中一次性为所有元素设置。',
    'Read the settings of an IFC translator, such as its type mapping and property mapping, for example to check them as the IFC conformity check of the Missing BIM Data report does':
        '读取 IFC 转换器的设置，例如其类型映射和属性映射，以便像“缺失 BIM 数据”报告的 IFC 合规性检查那样检查它们',
    'Check what the translator gives the elements instead: their IFC type with <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a>, and from Archicad 28 the properties and classification references the preview translator maps to them with <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> and <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> shows whether the properties a mapping needs exist in the project.':
        '改为检查转换器给元素的结果：用 <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a> 读取它们的 IFC 类型；从 Archicad 28 起，用 <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> 和 <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a> 读取预览转换器为它们映射的属性和分类引用。映射所需的属性是否存在于项目中，可用 <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> 查看。',
    'Find the graphic override rules, Interactive Schedules and SAF translators that refer to a property or classification that no longer exists, as the Missing BIM Data report does':
        '找出引用已不存在的属性或分类的图形覆盖规则、交互式清单和 SAF 转换器，就像“缺失 BIM 数据”报告那样',
    'For graphic override rules (from Archicad 27): read the criterion of each rule with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) and compare the properties and classifications it tests with <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> and <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. For Interactive Schedules and SAF translators: nothing yet.':
        '图形覆盖规则（从 Archicad 27 起）：用 <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a>（includeCriterion）读取每条规则的条件，并将其检查的属性和分类与 <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> 和 <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a> 的结果比较。交互式清单和 SAF 转换器：暂无办法。',
    'Export and import, all at once, the settings that the dialogs of Archicad save as XML (properties, classifications, the Layout Book, graphic overrides, Model View Options, IFC and DWG translators), to keep several projects in sync':
        '一次性导出和导入 Archicad 对话框保存为 XML 的设置（属性、分类、图册、图形覆盖、模型视图选项、IFC 和 DWG 转换器），以便让多个项目保持同步',
    'Copy what the API can write, one kind at a time, in one script run against each project: properties and classifications from their XML files with <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> and <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; attributes from another project with <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; graphic override rules with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> and <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; Project Info with <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> and <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; Favorites with <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> and <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. The Layout Book, Model View Options and the translators: nothing yet.':
        '在对每个项目运行的一个脚本中，逐类复制 API 能写入的内容：用 <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> 和 <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a> 从 XML 文件导入属性和分类；用 <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a> 从另一个项目导入属性；用 <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> 和 <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a> 复制图形覆盖规则；用 <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> 和 <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a> 复制项目信息；用 <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> 和 <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a> 复制收藏夹。图册、模型视图选项和转换器：暂无办法。',


    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        '发现了 Archicad API 做不到的其他事情？请在 issue 中说明，并写出你查过的 DevKit 头文件，它就会被添加到这里。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">提交 issue</a>'
});
