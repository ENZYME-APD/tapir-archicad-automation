/* 中文 (简体): the Get started page (start/index.html). See ../README.md.
   The menu, palette and button names of Archicad and Tapir stay English, as the add-on shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Automate Archicad with Python – get started | Tapir': '用 Python 实现 Archicad 自动化 – 快速开始 | Tapir',
    'Install Tapir, run a script from the Tapir palette in Archicad, add more scripts, write a first Python script and install the Grasshopper plugin.':
        '安装 Tapir，在 Archicad 中从 Tapir 面板运行脚本，添加更多脚本，编写第一个 Python 脚本，并安装 Grasshopper 插件。',
    '<a href="../">Tapir</a> / Get started': '<a href="../">Tapir</a> / 快速开始',
    'Get <em>started.</em>': '<em>快速开始。</em>',
    'From download to your first script in about ten minutes. Steps 1 to 3 need no programming.':
        '从下载到运行第一个脚本，大约十分钟。第 1 到第 3 步无需编程。',
    '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Prefer watching? The <a href="../tutorials/">Tapir video tutorials</a> walk through installing Tapir and using it.':
        '<strong class="callout-label"><span aria-hidden="true">▶</span> 视频</strong> 更喜欢看视频？<a href="../tutorials/">Tapir 视频教程</a>（英文）演示了如何安装和使用 Tapir。',

    // 1. Install
    'Install Tapir': '安装 Tapir',
    '<strong>With the installer (recommended).</strong> It finds every Archicad version on the computer and installs the matching add-on into each, so Archicad loads it on its next start.':
        '<strong>使用安装程序（推荐）。</strong>它会找到电脑上的每个 Archicad 版本，并为每个版本安装匹配的插件，Archicad 下次启动时便会加载它。',
    'Download <span class="os">WINDOWS</span>': '下载 <span class="os">WINDOWS</span>',
    'Download <span class="os">MACOS</span>': '下载 <span class="os">MACOS</span>',
    '<strong>Windows:</strong> the installer asks for administrator rights, because it copies the add-on into the Archicad folder under Program Files.':
        '<strong>Windows：</strong>安装程序会请求管理员权限，因为它要把插件复制到 Program Files 下的 Archicad 文件夹中。',
    '<strong>macOS:</strong> unzip the download and open the app.':
        '<strong>macOS：</strong>解压下载的文件并打开应用。',
    '<strong>Prefer to install by hand?</strong> Download the <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> or <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> file for your Archicad version from the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">latest release</a>, then in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, browse to the file and click <em>OK</em>.':
        '<strong>想手动安装？</strong>从<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">最新版本</a>下载与你的 Archicad 版本对应的 <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> 或 <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> 文件，然后在 Archicad 中：<em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>，选择该文件并点击 <em>OK</em>。',
    'Tapir keeps itself up to date: it tells you when a new version is out and installs it for you.':
        'Tapir 会自动保持最新：有新版本时它会提醒你，并为你安装。',

    // 2. Run a script
    'Run your first script': '运行第一个脚本',
    'Tapir adds a <em>Tapir</em> menu to Archicad. Choose <em>Tapir Palette</em> from it, pick a script in the palette\'s list and click <em>Run Script</em>. The built-in scripts are there from the start: try <em>ProjectHealthCheck</em>, which lists elements without an ID, zones with the same number, unclassified elements and more. It only reads the model.':
        'Tapir 会在 Archicad 中添加一个 <em>Tapir</em> 菜单。从中选择 <em>Tapir Palette</em>，在面板的列表中选一个脚本，然后点击 <em>Run Script</em>。内置脚本一开始就在：试试 <em>ProjectHealthCheck</em>，它会列出没有 ID 的构件、编号相同的区域、未分类的构件等。它只读取模型。',
    'Tapir runs the scripts with <a href="https://docs.astral.sh/uv/">uv</a>, which brings the Python and the packages each script needs. If uv is not installed yet, Tapir offers to install it the first time you run a script.':
        'Tapir 使用 <a href="https://docs.astral.sh/uv/">uv</a> 运行脚本，uv 会带来每个脚本所需的 Python 和软件包。如果还没有安装 uv，Tapir 会在你第一次运行脚本时提出安装。',
    '<em>Manage Shortcuts</em> on the palette puts up to six scripts in the Tapir menu, as <em>Run Script Shortcut 1</em> to <em>6</em>.':
        '面板上的 <em>Manage Shortcuts</em> 可以把最多六个脚本放进 Tapir 菜单，显示为 <em>Run Script Shortcut 1</em> 到 <em>6</em>。',

    // 3. More scripts
    'Add more scripts': '添加更多脚本',
    'The <a href="../scripts/">Scripts</a> page collects scripts shared by the community, each one reviewed before it is listed. The Tapir palette is getting a window to find and install them in one click.':
        '<a href="../scripts/">脚本</a>页面收集了社区分享的脚本，每个脚本在上架前都经过审核。Tapir 面板即将提供一个窗口，一键查找并安装它们。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Until then</strong> If your palette has no such window yet, add a script by hand, as below.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> 在此之前</strong> 如果你的面板还没有这个窗口，请按下面的方法手动添加脚本。',
    'Download the scripts you want into a folder, then tell Tapir about that folder in its configuration file, <code>Documents/Tapir/config.json</code> in your user folder (Tapir writes it the first time it runs). Add the folder to its <code>localFolders</code> list, keeping the rest of the file as it is:':
        '把需要的脚本下载到一个文件夹，然后在 Tapir 的配置文件中告诉它这个文件夹：用户文件夹中的 <code>Documents/Tapir/config.json</code>（Tapir 首次运行时会创建它）。把文件夹加入其中的 <code>localFolders</code> 列表，文件的其余部分保持不变：',
    '<code>path</code> is the folder (on macOS for example <code>/Users/me/Documents/Tapir scripts</code>). In JSON, a Windows path\'s backslashes are doubled.':
        '<code>path</code> 是文件夹（例如 macOS 上的 <code>/Users/me/Documents/Tapir scripts</code>）。在 JSON 中，Windows 路径的反斜杠要写两次。',
    '<code>displayName</code> is the name the palette shows for the folder\'s scripts; without it, the folder\'s name is used.':
        '<code>displayName</code> 是面板为该文件夹的脚本显示的名称；没有它时，使用文件夹的名称。',
    'Optionally, <code>includePattern</code> and <code>excludePattern</code> are regular expressions that choose which files of the folder the palette lists.':
        '可选的 <code>includePattern</code> 和 <code>excludePattern</code> 是正则表达式，用来选择面板列出文件夹中的哪些文件。',
    'The palette lists the folder\'s scripts next to the built-in ones the next time it loads its scripts.':
        '面板下次加载脚本时，会把该文件夹的脚本列在内置脚本旁边。',

    // 4. Python
    'Write a script of your own': '编写自己的脚本',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (with several Archicads open, the next ones use 19724, 19725 and so on). Tapir\'s commands are called through Archicad\'s <code>API.ExecuteAddOnCommand</code> command, in the <code>TapirCommand</code> namespace. Python\'s standard library is enough; there are no packages to install.':
        '项目打开时，Archicad 在 <code>http://127.0.0.1:19723</code> 上响应 JSON 命令（同时打开多个 Archicad 时，后面的依次使用 19724、19725 等）。Tapir 的命令通过 Archicad 的 <code>API.ExecuteAddOnCommand</code> 命令调用，命名空间为 <code>TapirCommand</code>。Python 标准库就足够了，无需安装任何软件包。',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a> sends the same commands and finds the port by itself, if you prefer it. Put your script in a <a href="#marketplace">local folder</a> to run it from the Tapir palette.':
        '如果你愿意，也可以使用 Graphisoft 的 <a href="https://pypi.org/project/archicad/"><code>archicad</code> 软件包</a>，它发送同样的命令，并会自己找到端口。把脚本放进一个<a href="#marketplace">本地文件夹</a>，即可从 Tapir 面板运行它。',
    '<a href="../archicad-addon/">All commands, with schemas and examples</a>':
        '<a href="../archicad-addon/">所有命令，附结构定义和示例</a>',

    // 5. Grasshopper
    'Use it from Grasshopper': '在 Grasshopper 中使用',
    'In Rhino, open the Package Manager (the <code>PackageManager</code> command, or <em>Tools &gt; Package Manager</em>), search for <span class="mono">tapir</span>, install it and restart Rhino. Its components are on the <em>Tapir</em> tab in Grasshopper and work with the Archicad that runs Tapir. The Walls, Doors, Windows and Railings components read and change Archicad elements with the names you know from Archicad\'s settings dialogs.':
        '在 Rhino 中打开 Package Manager（<code>PackageManager</code> 命令，或 <em>Tools &gt; Package Manager</em>），搜索 <span class="mono">tapir</span>，安装后重启 Rhino。它的运算器位于 Grasshopper 的 <em>Tapir</em> 选项卡中，与运行 Tapir 的 Archicad 协同工作。Walls、Doors、Windows 和 Railings 运算器使用你在 Archicad 设置对话框中熟悉的名称来读取和修改 Archicad 构件。',
    '<a href="../grasshopper/">The Grasshopper guide</a>': '<a href="../grasshopper/">Grasshopper 指南</a>',

    // Next step
    'Next step': '下一步',
    'Find a script': '查找脚本',
    'Scripts shared by architects, checked and adapted for any office.':
        '建筑师分享的脚本，经过检查并调整为适用于任何事务所。',
    'Browse scripts': '浏览脚本',
    'The command reference': '命令参考',
    'Every Tapir command with its parameters, its response and a Python example.':
        '每个 Tapir 命令及其参数、响应和 Python 示例。',
    'Open the reference': '打开参考',
    'Stuck?': '遇到问题？',
    'Ask on Discord': '在 Discord 上提问',
    'Architects, developers and the maintainers answer there. Or open an issue on GitHub.':
        '建筑师、开发者和维护者会在那里解答。也可以在 GitHub 上提交 issue。',
    'Join Discord': '加入 Discord'
});
