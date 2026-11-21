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

    // Check the names first
    'Check the names first': '先核对名称',
    'Check names, <em>don\'t guess.</em>': '核对名称，<em>不要猜。</em>',
    'Your words rarely match the names in the project: "fire rating" may be a property called <span class="mono">FireRating</span> in one template and <span class="mono">Feuerwiderstand</span> in another, and "level 2" may be the story "02 First Floor". An AI that guesses the name still gets an answer, only a wrong one: no values found, so "no doors have a fire rating".':
        '您的说法很少与项目中的名称一致：“耐火等级”在一个模板里可能是名为 <span class="mono">FireRating</span> 的属性，在另一个模板里却是 <span class="mono">Feuerwiderstand</span>；“2 层”可能是名为“02 First Floor”的楼层。靠猜测名称的 AI 仍然会得到答案，只是错误的答案：找不到任何值，于是得出“没有门有耐火等级”。',
    '<b aria-hidden="true">1</b><strong>Look the words up first</strong><span>Before a command whose parameters come from your words (a story, property, classification, layer, building material, composite, surface, zone category or element type), the AI sends those words to <a href="../archicad-addon/#ResolveNames">ResolveNames</a>, with the kind of each when it knows it. It only reads.</span>':
        '<b aria-hidden="true">1</b><strong>先查这些词</strong><span>在执行参数来自您的说法（楼层、属性、分类、图层、建筑材料、复合结构、表面、区域类别或元素类型）的命令之前，AI 会先把这些词发给 <a href="../archicad-addon/#ResolveNames">ResolveNames</a>，知道类型时也一并说明。这个命令只读取。</span>',
    '<b aria-hidden="true">2</b><strong>Use what it returns</strong><span><span class="mono">found</span>: the item named exactly so, with the ids the other commands take (story index, attribute id, elementType, propertyId, classification ids). <span class="mono">closeMatches</span>: only similar names, best first; the AI asks you which one you meant instead of picking one itself.</span>':
        '<b aria-hidden="true">2</b><strong>使用它返回的结果</strong><span><span class="mono">found</span>：名称完全一致的条目，并附带其他命令需要的 id（楼层索引、attribute id、elementType、propertyId、分类 id）。<span class="mono">closeMatches</span>：只有相似的名称，最接近的排在前面；AI 会问您指的是哪一个，而不是自己挑一个。</span>',
    '<b aria-hidden="true">3</b><strong>Say "not in this project"</strong><span><span class="mono">notFound</span>: nothing in the project is close, so the project has no such item. The AI says "this project has no X" instead of guessing a name or reporting an empty result as the answer.</span>':
        '<b aria-hidden="true">3</b><strong>说“项目中没有”</strong><span><span class="mono">notFound</span>：项目中没有相近的名称，所以项目里没有这样的条目。AI 会说“这个项目没有 X”，而不是猜一个名称，或把空结果当作答案。</span>',
    'Add these lines to your prompt, or to the instructions of your MCP server, so the AI checks the names every time. They work the same when the AI writes a script: the script sends <span class="mono">ResolveNames</span> first and stops with a message on <span class="mono">notFound</span>.':
        '把这几行加到您的提示词或 MCP 服务器的说明中，AI 每次都会核对名称。AI 编写脚本时同样适用：脚本先发送 <span class="mono">ResolveNames</span>，遇到 <span class="mono">notFound</span> 时给出提示并停止。',
    'Before a change, ask for a preview. The commands that delete or replace, such as <a href="../archicad-addon/#DeleteElements">DeleteElements</a>, <a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a> and <a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a>, take <span class="mono">"dryRun": true</span>: nothing changes, and the response lists what would. The command reference says which commands have it.':
        '修改之前，先要求预览。删除或替换类的命令，例如 <a href="../archicad-addon/#DeleteElements">DeleteElements</a>、<a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a> 和 <a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a>，接受 <span class="mono">"dryRun": true</span>：不会做任何更改，响应会列出将要更改的内容。命令参考中写明了哪些命令支持它。',
    'add to any request': '添加到任何请求中',
    'Before you use a name taken from my words (a story, property, classification, layer, building material, composite, surface, zone category or element type), check it with the Tapir command ResolveNames. - found: use the ids it returns. - closeMatches: ask me which one I mean. - notFound: tell me this project has no such item. Never guess a name. Before a command that deletes or replaces, run it with "dryRun": true if it has that input, and show me what it would change.':
        '在使用取自我说法的名称（楼层、属性、分类、图层、\n建筑材料、复合结构、表面、区域类别或元素类型）之前，\n先用 Tapir 命令 ResolveNames 核对。\n- found：使用它返回的 id。\n- closeMatches：问我指的是哪一个。\n- notFound：告诉我这个项目没有这样的条目。\n  绝不要猜测名称。\n在执行删除或替换类的命令之前，如果它有 "dryRun": true\n这个输入，就先这样运行，并告诉我它会更改什么。',

    // When the AI writes a script
    'When the AI writes a script': '当 AI 编写脚本时',
    'Describe it, <em>run it.</em>': '描述它，<em>运行它。</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        '没有 MCP 服务器时，描述任务，把命令列表交给 AI，然后运行它编写的脚本。',
    '<b aria-hidden="true">1</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b aria-hidden="true">1</b><strong>描述您的需求</strong><span>例如：“提取 2 层所有复合外墙并计算它们的净体积。”</span>',
    '<b aria-hidden="true">2</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b aria-hidden="true">2</b><strong>提供命令列表</strong><span>把<a href="../archicad-addon/">命令参考</a>和下面可运行的示例交给 AI。要求它只使用 Python 标准库，并使用端口 19723。</span>',
    '<b aria-hidden="true">3</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b aria-hidden="true">3</b><strong>先阅读，再运行</strong><span>从 Tapir 面板或终端运行脚本之前先阅读它。会修改模型的脚本，请先在项目副本上试用。</span>',
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
