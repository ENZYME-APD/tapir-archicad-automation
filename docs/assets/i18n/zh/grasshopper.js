/* 中文 (简体): the Grasshopper page (grasshopper/index.html). See ../README.md.
   Component, input and button names stay English, as Grasshopper shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad Grasshopper live connection | Tapir': 'Archicad 与 Grasshopper 实时连接 | Tapir',
    'Connect Grasshopper live to a running Archicad with the Tapir plugin: read, change and create Archicad elements from Rhino geometry, without writing code.':
        '用 Tapir 插件将 Grasshopper 与正在运行的 Archicad 实时连接：根据 Rhino 几何体读取、修改和创建 Archicad 构件，无需编写代码。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper':
        '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / Grasshopper',
    'Grasshopper, <em>visually.</em>': 'Grasshopper，<em>可视化。</em>',
    'The Tapir plugin connects Grasshopper to a running Archicad: read elements and their settings, change them, and create new ones from Rhino geometry, without writing code.':
        'Tapir 插件把 Grasshopper 连接到正在运行的 Archicad：读取构件及其设置、修改它们，并从 Rhino 几何体创建新构件，无需编写代码。',

    // What the components do
    'What the components <em>do.</em>': '组件能<em>做什么。</em>',
    'Each component sends a Tapir command to Archicad and gives you the answer as Grasshopper data. You find them on the <strong>Tapir</strong> tab, grouped as in Archicad.':
        '每个组件向 Archicad 发送一个 Tapir 命令，并把结果作为 Grasshopper 数据返回给您。它们位于 <strong>Tapir</strong> 选项卡中，分组方式与 Archicad 相同。',
    '<b>READ</b><strong>Elements and their data</strong><span>Elements by type, story or selection; their properties, classifications, settings and geometry.</span>':
        '<b>读取</b><strong>构件及其数据</strong><span>按类型、楼层或选择获取构件；以及它们的属性、分类、设置和几何。</span>',
    '<b>CHANGE</b><strong>What you pass in</strong><span>Property values, classifications, layers, IDs and tool settings of the elements you pass in.</span>':
        '<b>修改</b><strong>您传入的内容</strong><span>所传入构件的属性值、分类、图层、ID 和工具设置。</span>',
    '<b>CREATE</b><strong>From Rhino geometry</strong><span>Walls, doors, windows, railings, level dimensions and more, from points and curves.</span>':
        '<b>创建</b><strong>来自 Rhino 几何体</strong><span>由点和曲线生成墙、门、窗、栏杆、标高标注等。</span>',
    '<b>PROJECT</b><strong>The rest of the file</strong><span>Stories, attributes, navigator views, layouts, issues, favorites and libraries.</span>':
        '<b>项目</b><strong>文件的其余部分</strong><span>楼层、属性、导航器视图、布图、问题、收藏夹和库。</span>',
    'Which Tapir commands have a component, and which settings each covers, is on the <a href="../gh-coverage/">Grasshopper coverage</a> page.':
        '哪些 Tapir 命令有对应的组件、每个组件覆盖哪些设置，请见 <a href="../gh-coverage/">Grasshopper 覆盖率</a>页面。',

    // Tool components
    'Tool components': '工具组件',
    'One per Archicad <em>tool.</em>': '每个 Archicad <em>工具一个。</em>',
    'A tool component works like the Archicad tool of the same name. One JSON input and one JSON output per panel of its settings dialog, keyed by the labels you see there, such as <code>Wall Height</code>.':
        '工具组件的用法与同名的 Archicad 工具相同。其设置对话框的每个面板对应一个 JSON 输入和一个 JSON 输出，键名是您在那里看到的英文标签，例如 <code>Wall Height</code>。',
    'Placed by <code>StartPoint</code> and <code>EndPoint</code>.': '由 <code>StartPoint</code> 和 <code>EndPoint</code> 放置。',
    'Placed in an <code>OwnerWall</code> at a <code>CenterOffset</code>.': '放置在 <code>OwnerWall</code> 中的 <code>CenterOffset</code> 处。',
    'Placed along a <code>ReferenceLine</code>.': '沿 <code>ReferenceLine</code> 放置。',
    'Placed at a <code>Position</code>, optionally on a parent element.': '放置在 <code>Position</code> 处，可选择附着在父构件上。',
    '<b>REFRESH</b><strong>Reads</strong><span>Reads the elements in <code>ElementGuid</code> and fills every output.</span>':
        '<b>REFRESH</b><strong>读取</strong><span>读取 <code>ElementGuid</code> 中的构件并填充所有输出。</span>',
    '<b>EXECUTE</b><strong>Writes</strong><span>Writes the given settings to those elements. Only the keys you give are written.</span>':
        '<b>EXECUTE</b><strong>写入</strong><span>把给定的设置写入这些构件。只写入您给出的键。</span>',
    '<b>CREATE</b><strong>Makes new elements</strong><span>From the geometry inputs, starting from the tool\'s defaults or from a <code>FavoriteName</code>.</span>':
        '<b>CREATE</b><strong>创建新构件</strong><span>根据几何输入创建，以工具的默认设置或某个 <code>FavoriteName</code> 为起点。</span>',
    'Nothing is written to Archicad until you press Execute or Create. A JSON output read by Refresh can be fed straight back into the same input. Archicad has no command to change railings or level dimensions after they are placed, so for those Execute only writes the ID, layer and draw order.':
        '在您按下 Execute 或 Create 之前，不会向 Archicad 写入任何内容。Refresh 读取的 JSON 输出可以直接送回同一个输入。Archicad 没有在放置后修改栏杆或标高标注的命令，因此对它们 Execute 只写入 ID、图层和绘制顺序。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Note</strong> Walls remembers the walls its Create made, in the <code>.gh</code> file. The next Create updates them instead of making new ones. Turn this off, or forget them, in the component\'s menu.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> 注意</strong> Walls 会在 <code>.gh</code> 文件中记住其 Create 创建的墙。下一次 Create 会更新这些墙，而不是新建。可在组件的菜单中关闭此功能或让它忘记这些墙。',

    // Install
    'Install': '安装',
    'Install the Tapir Add-On in Archicad': '在 Archicad 中安装 Tapir Add-On',
    'The components talk to Archicad through the Add-On. <a href="../start/#install">Get started</a> shows how.':
        '组件通过 Add-On 与 Archicad 通信。<a href="../start/#install">快速开始</a>介绍了安装方法。',
    'Install the <span class="mono">tapir</span> package in Rhino': '在 Rhino 中安装 <span class="mono">tapir</span> 包',
    'In Rhino 7 or 8, open <em>Tools &gt; Package Manager</em> (or type <code>PackageManager</code>), search for <strong>tapir</strong> and click Install. Restart Rhino.':
        '在 Rhino 7 或 8 中打开 <em>Tools &gt; Package Manager</em>（或输入 <code>PackageManager</code>），搜索 <strong>tapir</strong> 并点击 Install。然后重启 Rhino。',
    'Open Archicad, then Grasshopper': '先打开 Archicad，再打开 Grasshopper',
    'With a project open in Archicad, start Grasshopper. The <strong>Tapir</strong> tab appears in its toolbar. With more than one Archicad open, give the port of the one you want to the <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>); the first Archicad uses 19723.':
        '在 Archicad 中打开一个项目后启动 Grasshopper，其工具栏中会出现 <strong>Tapir</strong> 选项卡。如果打开了多个 Archicad，请把目标 Archicad 的端口传给 <strong>ConnectArchicad</strong> 组件（<em>Tapir &gt; General</em>）；第一个 Archicad 使用 19723。',

    // A first definition
    'A first definition': '第一个定义',
    'One <em>wall.</em>': '一面<em>墙。</em>',
    'Build this in an empty Grasshopper canvas, with a project open in Archicad:':
        '在 Archicad 中打开一个项目，然后在空白的 Grasshopper 画布中搭建：',
    'Place two <strong>Point</strong> parameters and set them to 0,0,0 and 5,0,0. Archicad reads the numbers as metres.':
        '放置两个 <strong>Point</strong> 参数，分别设为 0,0,0 和 5,0,0。Archicad 以米为单位读取这些数字。',
    'Place the <strong>Walls</strong> component from <em>Tapir &gt; Design Tools</em>. Connect the points to <code>StartPoint</code> and <code>EndPoint</code>.':
        '从 <em>Tapir &gt; Design Tools</em> 放置 <strong>Walls</strong> 组件，把两个点连接到 <code>StartPoint</code> 和 <code>EndPoint</code>。',
    'Place a <strong>Panel</strong> with the text on the right and connect it to <code>GeometryAndPositioning_JSON</code>.':
        '放置一个 <strong>Panel</strong>，填入右侧的文本，并把它连接到 <code>GeometryAndPositioning_JSON</code>。',
    'Press <strong>Create</strong>. A 5 m wall appears in Archicad, 3 m high and 30 cm thick.':
        '按下 <strong>Create</strong>。Archicad 中会出现一面长 5 m、高 3 m、厚 30 cm 的墙。',
    'Move a point and press Create again: the same wall moves. Change the panel and press Create: the same wall changes.':
        '移动一个点并再次按下 Create：同一面墙会随之移动。修改面板内容并按下 Create：同一面墙会随之改变。',
    'Press <strong>Refresh</strong> to see every setting of the wall in the JSON outputs. Copy a key from there to set it the next time.':
        '按下 <strong>Refresh</strong>，即可在 JSON 输出中看到这面墙的所有设置。从中复制一个键，下次就可以设置它。',

    // Next step
    'Next step': '下一步',
    'Install the Add-On': '安装 Add-On',
    'Install Tapir and the <span class="mono">tapir</span> package, then try the wall above.':
        '安装 Tapir 和 <span class="mono">tapir</span> 包，然后试试上面的墙。',
    'Get started': '快速开始',
    'Coverage': '覆盖率',
    'Which commands have a component, and which inputs each still lacks.':
        '哪些命令有组件，以及每个组件还缺少哪些输入。',
    'See the coverage': '查看覆盖率',
    'Videos': '视频',
    'Watch it work': '看看实际效果',
    'The Archicad and Grasshopper tutorials.': 'Archicad 和 Grasshopper 教程。',
    'Watch the tutorials': '观看教程'
});
