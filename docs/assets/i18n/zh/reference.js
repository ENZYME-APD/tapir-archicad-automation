/* 中文 (简体): the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Tapir commands - Tapir documentation': 'Tapir 命令 - Tapir 文档',
    'Every JSON command of the Tapir add-on for Archicad, with its parameters, its response and an example.':
        'Archicad 的 Tapir 插件的每个 JSON 命令，附带其参数、响应和示例。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / 命令',
    'Tapir <em>commands.</em>': 'Tapir <em>命令。</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'Tapir 插件在 Archicad 中注册的 JSON 命令，位于 <code>TapirCommand</code> 命名空间，与 Graphisoft 的官方命令并列。每个命令都列出其参数、响应和一个示例。',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script.':
        '第一次使用 Tapir？<a href="../start/">快速开始</a>介绍如何安装并运行第一个脚本。',
    'Calling a <em>command.</em>': '调用<em>命令。</em>',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (the next Archicads opened use 19724, 19725 and so on). A Tapir command is sent through Archicad\'s <code>API.ExecuteAddOnCommand</code> command.':
        '打开项目时，Archicad 在 <code>http://127.0.0.1:19723</code> 上响应 JSON 命令（之后打开的 Archicad 依次使用 19724、19725 等）。Tapir 命令通过 Archicad 的 <code>API.ExecuteAddOnCommand</code> 命令发送。',
    'Python standard library': 'Python 标准库',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>':
        'Graphisoft 的 <a href="https://pypi.org/project/archicad/"><code>archicad</code> 包</a>',
    'The examples of the commands below use <code>aclib</code>, the small helper in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> that sends commands the same way as the standard-library script.':
        '下面各命令的示例使用 <code>aclib</code>，即 <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> 中的小型辅助模块，它发送命令的方式与使用标准库的脚本相同。',
    'All <em>commands.</em>': '全部<em>命令。</em>',

    // The two coverage pages
    'Overview': '概览',
    'By group': '按分组',
    'Checklist': '清单',

    // The Archicad API coverage (api-coverage/)
    'Archicad API coverage - Tapir documentation': 'Archicad API 覆盖率 - Tapir 文档',
    'Which parts of the Archicad API the Tapir add-on uses, kept up to date from every issue, pull request and push.':
        'Tapir 插件使用了 Archicad API 的哪些部分，随每个 issue、pull request 和 push 保持更新。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / API 覆盖率',
    'Archicad API <em>coverage.</em>': 'Archicad API <em>覆盖率。</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Tapir 覆盖了 Graphisoft 的 Archicad API 的哪些内容，逐项列出。',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        '<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir 插件</a>使用了 Archicad API（<span id="archicad-range">每个受支持的 Archicad 版本</span>：C 函数及其旧名称和命令 ID，以及 C++ API）的哪些部分。每个 issue 和对 main 的 push 后几分钟内更新，每个 pull request 后一小时内更新。另请参阅<a href="../archicad-addon/">命令参考</a>。',
    'By part of the API': '按 API 部分',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage - Tapir documentation': 'Grasshopper 覆盖率 - Tapir 文档',
    'Which Tapir commands the components of the Tapir Grasshopper plugin call, and whether they cover the commands\' parameters.':
        'Tapir Grasshopper 插件的运算器调用了哪些 Tapir 命令，以及它们是否覆盖了命令的参数。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / Grasshopper 覆盖率',
    'Grasshopper <em>coverage.</em>': 'Grasshopper <em>覆盖率。</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Grasshopper 运算器调用了哪些 Tapir 命令，以及它们还缺少哪些参数。',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Tapir Grasshopper 插件的运算器调用了 <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir 插件</a>的哪些 JSON 命令，以及这些运算器是否覆盖了命令的参数。每个 issue 和对 main 的 push 后几分钟内更新，每个 pull request 后一小时内更新。这是对插件代码的静态检查：它显示运算器写出了哪些参数，而不保证每个参数都能正常工作。另请参阅<a href="../archicad-addon/">命令参考</a>和 <a href="../api-coverage/">Archicad API 覆盖率</a>。',
    'Parameters of the used commands': '已使用命令的参数'
});
