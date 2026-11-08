/* 中文 (简体): the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Archicad JSON API commands reference | Tapir': 'Archicad JSON API 命令参考 | Tapir',
    'Every JSON command the Tapir add-on adds to Archicad, with its parameters, its response and a Python example, and how to call one from Python.':
        'Tapir 插件为 Archicad 新增的每个 JSON 命令，附带其参数、响应和 Python 示例，以及如何从 Python 调用命令。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / 命令',
    'Tapir <em>commands.</em>': 'Tapir <em>命令。</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'Tapir 插件在 Archicad 中注册的 JSON 命令，位于 <code>TapirCommand</code> 命名空间，与 Graphisoft 的官方命令并列。每个命令都列出其参数、响应和一个示例。',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script. Looking for something no command does? <a href="../limits/">What the Archicad API can\'t do</a> lists what Graphisoft\'s API does not offer, with what to do instead.':
        '第一次使用 Tapir？<a href="../start/">快速开始</a>介绍如何安装并运行第一个脚本。想找没有任何命令能做的事？<a href="../limits/">Archicad API 做不到的事</a>列出了 Graphisoft 的 API 没有提供的功能，以及替代办法。',
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
    'Archicad API coverage checklist | Tapir': 'Archicad API 覆盖率清单 | Tapir',
    'Which parts of Graphisoft\'s Archicad API the Tapir add-on uses, item by item, kept up to date from every issue, pull request and push to the repository.':
        'Tapir 插件使用了 Graphisoft 的 Archicad API 的哪些部分，逐项列出，并随仓库的每个 issue、pull request 和 push 保持更新。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / API 覆盖率',
    'Archicad API <em>coverage.</em>': 'Archicad API <em>覆盖率。</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Tapir 覆盖了 Graphisoft 的 Archicad API 的哪些内容，逐项列出。',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        '<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir 插件</a>使用了 Archicad API（<span id="archicad-range">每个受支持的 Archicad 版本</span>：C 函数及其旧名称和命令 ID，以及 C++ API）的哪些部分。每个 issue 和对 main 的 push 后几分钟内更新，每个 pull request 后一小时内更新。另请参阅<a href="../archicad-addon/">命令参考</a>。',
    'By part of the API': '按 API 部分',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage of the Tapir commands | Tapir': 'Tapir 命令的 Grasshopper 覆盖率 | Tapir',
    'Which Tapir commands for Archicad the components of the Tapir Grasshopper plugin call, and whether they cover each command\'s parameters or still miss some.':
        'Tapir Grasshopper 插件的运算器调用了哪些用于 Archicad 的 Tapir 命令，以及它们是覆盖了每个命令的参数，还是仍缺少一些。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">文档</a> / Grasshopper 覆盖率',
    'Grasshopper <em>coverage.</em>': 'Grasshopper <em>覆盖率。</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Grasshopper 运算器调用了哪些 Tapir 命令，以及它们还缺少哪些参数。',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Tapir Grasshopper 插件的运算器调用了 <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir 插件</a>的哪些 JSON 命令，以及这些运算器是否覆盖了命令的参数。每个 issue 和对 main 的 push 后几分钟内更新，每个 pull request 后一小时内更新。这是对插件代码的静态检查：它显示运算器写出了哪些参数，而不保证每个参数都能正常工作。另请参阅<a href="../archicad-addon/">命令参考</a>和 <a href="../api-coverage/">Archicad API 覆盖率</a>。',
    'Parameters of the used commands': '已使用命令的参数'
});
