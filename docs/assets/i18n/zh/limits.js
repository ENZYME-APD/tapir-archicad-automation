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
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        '用 <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> 读取楼板的轮廓，再用 <a href="../archicad-addon/#CreateRailings">CreateRailings</a> 沿该边缘创建静态栏杆。楼板改变时，删除栏杆并重新创建。',

    // Navigator and views
    'Navigator and views': '导航器与视图',
    'Add an item to a publisher set': '向发布集（publisher set）添加项目',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        '在 Archicad 的 Organizer 中添加项目，然后用 <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> 发布该集。',
    'Change, rename or create a stored Model View Options combination': '修改、重命名或新建已保存的 Model View Options 组合',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        '用 <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> 读取这些组合；用 <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a> 修改当前窗口的选项。',

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

    // Documents and publishing
    'Documents and publishing': '文档与发布',
    'Read the rows of an Interactive Schedule': '读取 Interactive Schedule 的行',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        '自己生成清单：用 <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> 查找元素，再用 <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> 或 <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a> 读取它们的值。<a href="../scripts/">内置脚本</a> Room Finishes Schedule 和 Material Take-Off 就会生成这样的清单。',
    'Export an Interactive Schedule': '导出 Interactive Schedule',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        '先在 Archicad 中把该清单放进一个发布集（只需一次），然后用 <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> 发布该集。',

    // Archicad's interface
    'Archicad\'s interface': 'Archicad 的界面',
    'Read the text of the Session Report window': '读取 Session Report 窗口中的文本',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        '用 <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> 并设置 windowType \'Report\' 为用户打开该窗口（Archicad 26 起）。',
    'Get the outcome of an Action Center check': '获取 Action Center 检查的结果',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        '用 <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> 运行检查；发现问题时 Archicad 会显示 Action Center。对于热链接检查，用 <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> 读取每个源的状态（Archicad 27 起）。',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        '发现了 Archicad API 做不到的其他事情？请在 issue 中说明，并写出你查过的 DevKit 头文件，它就会被添加到这里。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">提交 issue</a>'
});
