/* 中文 (简体): the Docs page (docs/index.html). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad automation docs | Tapir': 'Archicad 自动化文档 | Tapir',
    'Tapir\'s documentation: every JSON command, the Grasshopper guide, the Archicad API coverage and the Grasshopper coverage, and how a call to Tapir works.':
        'Tapir 的文档：每个 JSON 命令、Grasshopper 指南、Archicad API 覆盖率和 Grasshopper 覆盖率，以及对 Tapir 的一次调用如何工作。',
    '<a href="../">Tapir</a> / Docs': '<a href="../">Tapir</a> / 文档',
    'The <em>reference.</em>': '<em>参考文档。</em>',
    'Every command, what Tapir covers of the Archicad API, and how the Grasshopper components map to it.':
        '每个命令、Tapir 覆盖了 Archicad API 的哪些部分，以及 Grasshopper 运算器与它们的对应关系。',
    'The docs': '文档',

    // The four docs
    'Tapir commands': 'Tapir 命令',
    'Search by name, filter by the version a command came in, and open any command for its input and output schema and a Python example.':
        '按名称搜索，按命令加入的版本筛选，打开任意命令查看其输入和输出结构定义及 Python 示例。',
    'Open the reference': '打开参考',
    'Grasshopper guide': 'Grasshopper 指南',
    'The tool components, one per Archicad tool, with inputs named after the settings dialog. Install, a first definition, and what the buttons do.':
        '工具运算器，每个 Archicad 工具一个，输入端按设置对话框命名。包括安装、第一个定义，以及各按钮的作用。',
    'Read the guide': '阅读指南',
    'Archicad API': 'Archicad API',
    '· <span data-live-value="coverage"></span> covered': '· 已覆盖 <span data-live-value="coverage"></span>',
    'Every function of Graphisoft\'s Archicad API and whether Tapir uses it, is working on it, or skipped it, updated every hour.':
        'Graphisoft Archicad API 的每个函数，以及 Tapir 是否已使用、正在实现或已跳过，每小时更新。',
    'See the coverage': '查看覆盖率',
    'Components': '运算器',
    'Which Tapir commands have a Grasshopper component, and which inputs each component still lacks.':
        '哪些 Tapir 命令有 Grasshopper 运算器，以及每个运算器还缺少哪些输入。',

    // The API limits teaser (its title is in common.js)
    'What the Archicad API can\'t do, such as undo, the clipboard or the rows of a schedule, with what to do instead and the DevKit evidence.':
        'Archicad API 做不到的事，例如撤销、剪贴板或清单中的行，附有替代办法和 DevKit 中的依据。',
    'See the limits': '查看限制',

    // How it works
    'How it works': '工作原理',
    'How it <em>works.</em>': '<em>工作原理。</em>',
    'Tapir is an Archicad add-on that turns native API functions into a stateless JSON server on port 19723. Anything that can send an HTTP request can drive Archicad.':
        'Tapir 是一个 Archicad 插件，它把原生 API 函数变成端口 19723 上的无状态 JSON 服务器。任何能发送 HTTP 请求的程序都能驱动 Archicad。',
    'Grasshopper, Python scripts and AI tools send JSON over HTTP to the Tapir Add-On, which calls Archicad\'s C++ API on the BIM model.':
        'Grasshopper、Python 脚本和 AI 工具通过 HTTP 向 Tapir 插件发送 JSON，插件再调用 Archicad 的 C++ API 操作 BIM 模型。',
    '<strong>Grasshopper</strong><span>The tapir components</span>': '<strong>Grasshopper</strong><span>tapir 运算器</span>',
    '<strong>Python scripts</strong><span>The palette, or your own</span>': '<strong>Python 脚本</strong><span>面板中的或你自己的</span>',
    '<strong>AI and MCP</strong><span>Assistants and MCP servers</span>': '<strong>AI 与 MCP</strong><span>AI 助手和 MCP 服务器</span>',
    '↓ JSON over HTTP ↓': '↓ 通过 HTTP 传输 JSON ↓',
    '<strong>Tapir Add-On</strong><span>A stateless JSON layer: one command per request</span>':
        '<strong>Tapir 插件</strong><span>无状态的 JSON 层：每个请求一个命令</span>',
    '<strong>Archicad</strong><span>The BIM model and its C++ API</span>': '<strong>Archicad</strong><span>BIM 模型及其 C++ API</span>',

    // Anatomy of a call
    'Anatomy of a call': '一次调用的结构',
    'Every Tapir command is sent the same way; only <code>commandName</code> and <code>addOnCommandParameters</code> change. <a href="../start/#python">Your first Python script</a>':
        '每个 Tapir 命令的发送方式都相同，只有 <code>commandName</code> 和 <code>addOnCommandParameters</code> 不同。<a href="../start/#python">你的第一个 Python 脚本</a>',

    // More guides
    'More guides': '更多指南',
    'Install and run a script': '安装并运行脚本',
    'Install Tapir, run a script from the palette and write your first Python script.':
        '安装 Tapir，从面板运行脚本，并编写你的第一个 Python 脚本。',
    'Get started': '快速开始',
    'AI': 'AI',
    'MCP servers and scripts an AI assistant writes for you.': 'MCP 服务器，以及 AI 助手为你编写的脚本。',
    'Use Tapir with AI': '用 AI 使用 Tapir',
    'Learn by watching': '看视频学习',
    'Six videos, from installing Tapir to whole workflows.': '六个视频，从安装 Tapir 到完整的工作流程。',
    'Watch the tutorials': '观看教程'
});
