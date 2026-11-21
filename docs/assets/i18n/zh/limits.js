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
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        '关闭栏杆的自动楼层可见性，以便设置它显示在哪些楼层',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        '设置网面的 Floor Plan Display、Show Projection、未剖切线和上方线，或剖切线的线型',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        '读取或设置栏杆的平面图符号：其折断线、显示哪些符号构件、它们的二维属性以及符号覆盖',
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
