/* 中文 (简体): the Tapir and AI page (ai/index.html). See ../README.md.
   The two prompts are translated too (an AI tool understands them in any language), the Python
   example stays as it is. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Control Archicad with AI assistants (MCP) | Tapir': '用 AI 助手（MCP）控制 Archicad | Tapir',
    'Control Archicad with Claude and other AI assistants through Tapir: the Tapir MCP servers, a prompt to start with, and how to let an AI write a script.':
        '通过 Tapir 用 Claude 和其他 AI 助手控制 Archicad：Tapir 的 MCP 服务器、一个入门提示词，以及如何让 AI 编写脚本。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / AI': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / AI',
    'Tapir and <em>AI.</em>': 'Tapir 与 <em>AI。</em>',
    'An AI assistant can read your Archicad project and change it through Tapir. You ask in plain words; it sends the commands to the Archicad running on your computer.':
        'AI 助手可以通过 Tapir 读取并修改您的 Archicad 项目。您用日常语言提出要求，它把命令发送到您电脑上运行的 Archicad。',

    // Two ways
    'Two ways to <em>connect.</em>': '两种<em>连接方式。</em>',
    'Both talk to the same Tapir commands on port 19723, on your own computer.':
        '两者都在您自己的电脑上，通过端口 19723 使用相同的 Tapir 命令。',
    'While you chat': '边聊边用',
    'An MCP server': 'MCP 服务器',
    'The AI tool calls Tapir\'s commands itself, while you chat. Works with tools that support the Model Context Protocol (MCP), such as Claude Desktop.':
        '在您聊天时，AI 工具自己调用 Tapir 的命令。适用于支持 Model Context Protocol (MCP) 的工具，例如 Claude Desktop。',
    'Tapir MCP servers': 'Tapir 的 MCP 服务器',
    'Any AI tool': '任何 AI 工具',
    'A script the AI writes': '由 AI 编写的脚本',
    'Any AI tool can write a short Python script that sends Tapir commands. You read it, then run it from the Tapir palette or a terminal.':
        '任何 AI 工具都能编写一个发送 Tapir 命令的简短 Python 脚本。您先阅读它，再从 Tapir 面板或终端运行。',
    'How it works': '工作原理',

    // MCP servers
    'MCP servers': 'MCP 服务器',
    'Tapir MCP <em>servers.</em>': 'Tapir 的 MCP <em>服务器。</em>',
    'These are community projects, not part of Tapir. Each has its own install steps in its README. Which one Tapir recommends is still being decided in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, so they are listed here without a recommendation.':
        '这些是社区项目，不属于 Tapir。每个项目的 README 中都有各自的安装步骤。Tapir 推荐哪一个仍在 <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a> 中讨论，因此这里只列出它们，不作推荐。',
    'By SzamosiMate': '作者 SzamosiMate',
    'The MCP server for Tapir that the others started from. It is also published on PyPI as the <span class="mono">tapir-archicad-mcp</span> package.':
        '其他项目所基于的 Tapir MCP 服务器。它也以 <span class="mono">tapir-archicad-mcp</span> 包的形式发布在 PyPI 上。',
    'Its repository': '代码仓库',
    'The package on PyPI': 'PyPI 上的包',
    'By the community': '来自社区',
    'Its forks': '它的分支',
    'Versions of it that others changed and published. Check when each was last updated.':
        '其他人修改并发布的版本。请留意每个版本的最近更新时间。',
    'See the forks': '查看分支',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Careful</strong> An MCP server can change your model. Try it on a copy of your project first, and read what the AI is about to do before you allow it.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> 小心</strong> MCP 服务器可以修改您的模型。请先在项目副本上试用，并在允许之前阅读 AI 将要执行的操作。',

    // A prompt to start with
    'A prompt to start with': '入门提示词',
    'Start by <em>reading.</em>': '从<em>读取开始。</em>',
    'Open a project in Archicad, with the Tapir Add-On loaded, and give your AI tool this prompt. It only reads, so it is safe to try on any project.':
        '在已加载 Tapir Add-On 的 Archicad 中打开一个项目，然后把这个提示词交给您的 AI 工具。它只读取数据，因此可以放心地在任何项目上尝试。',
    'When the answer looks right, ask for a change in the same plain words, and say exactly which elements it may change: "Set the property X to Y on the selected walls only."':
        '如果回答看起来正确，再用同样的日常语言提出修改，并明确说明它可以修改哪些构件：“仅将所选墙的属性 X 设为 Y。”',
    'prompt': '提示词',
    'read only': '只读',
    'Use Tapir to look at the Archicad project I have open. Do not change anything; only read. 1. List the stories with their names and levels. 2. Count the walls, slabs, columns, doors and windows on each story. 3. Show the result as one table, one row per story. Tell me which Tapir commands you used.':
        '用 Tapir 查看我打开的 Archicad 项目。\n不要修改任何内容，只读取。\n1. 列出所有楼层及其名称和标高。\n2. 统计每个楼层的墙、楼板、柱、门和窗的数量。\n3. 用一张表显示结果，每个楼层一行。\n告诉我你用了哪些 Tapir 命令。',

    // When the AI writes a script
    'When the AI writes a script': '当 AI 编写脚本时',
    'Describe it, <em>run it.</em>': '描述它，<em>运行它。</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        '没有 MCP 服务器时，描述任务，把命令列表交给 AI，然后运行它编写的脚本。',
    '<b>01</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b>01</b><strong>描述您的需求</strong><span>例如：“提取 2 层所有复合外墙并计算它们的净体积。”</span>',
    '<b>02</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b>02</b><strong>提供命令列表</strong><span>把<a href="../archicad-addon/">命令参考</a>和下面可运行的示例交给 AI。要求它只使用 Python 标准库，并使用端口 19723。</span>',
    '<b>03</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b>03</b><strong>先阅读，再运行</strong><span>从 Tapir 面板或终端运行脚本之前先阅读它。会修改模型的脚本，请先在项目副本上试用。</span>',
    'for Claude or any AI tool': '适用于 Claude 或任何 AI 工具',
    'Write a Python script that uses only the standard library and sends Tapir commands to Archicad on http://127.0.0.1:19723, like the example I attach. Use the Tapir command reference I attach. Extract all composite exterior walls on level 2 and calculate their net volume. Only read the model; change nothing.':
        '编写一个只使用标准库的 Python 脚本，\n像我附上的示例那样，向\nhttp://127.0.0.1:19723 上的 Archicad 发送 Tapir 命令。\n使用我附上的 Tapir 命令参考。\n\n提取 2 层所有复合外墙，\n并计算它们的净体积。\n\n只读取模型，不要修改任何内容。',

    // Next step
    'Next step': '下一步',
    'Install Tapir': '安装 Tapir',
    'Install Tapir, then give your AI tool the prompt above.': '安装 Tapir，然后把上面的提示词交给您的 AI 工具。',
    'Get started': '快速开始',
    'The command reference': '命令参考',
    'The list to give the AI: every command with its schema.': '要交给 AI 的列表：每个命令及其模式。',
    'Open the reference': '打开参考',
    'Show what you built': '展示您的作品',
    'Ask about MCP servers and share your prompts.': '询问 MCP 服务器相关问题，分享您的提示词。',
    'Join Discord': '加入 Discord'
});
