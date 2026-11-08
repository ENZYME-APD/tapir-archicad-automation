/* 中文 (简体): the Community page (community/index.html, with the texts community/community.js reads
   from it). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Community – Archicad automation with Tapir': '社区 – 用 Tapir 实现 Archicad 自动化',
    'Ask on Discord, report a bug, test a fix in your Archicad: how Tapir\'s community and its bots turn a report into a release, and how the project grew.':
        '在 Discord 上提问、报告 bug、在你的 Archicad 中测试修复：Tapir 的社区和它的机器人如何把一份报告变成一次发布，以及这个项目如何成长。',
    '<a href="../">Tapir</a> / Community': '<a href="../">Tapir</a> / 社区',
    'Ask. Report. <em>Test.</em>': '提问。报告。<em>测试。</em>',
    'Architects, developers and the maintainers in one place. Bugs and ideas become fixes that someone with your Archicad version tests before they ship.':
        '建筑师、开发者和维护者聚在一起。bug 和想法会变成修复，并在发布前由使用与你相同 Archicad 版本的人测试。',

    // Where to find us
    'Where to find us': '在哪里找到我们',
    'Ask anything': '有问必答',
    'Architects, developers and the maintainers, in one server. Show what you made and talk with the people who build Tapir.':
        '建筑师、开发者和维护者都在同一个服务器里。展示你的作品，和开发 Tapir 的人交流。',
    'Join Discord': '加入 Discord',
    'Learn the API': '学习 API',
    'A video series from the first command to full workflows.': '一套视频教程，从第一个命令讲到完整的工作流程。',
    'Watch the tutorials': '观看教程',
    'AI assistants': 'AI 助手',
    'Talk to your model': '和你的模型对话',
    'MCP servers let Claude and other assistants read and change a project through Tapir.':
        'MCP 服务器让 Claude 和其他助手通过 Tapir 读取和修改项目。',

    // How the bots work
    'From report to release': '从报告到发布',
    'How the bots <em>work.</em>': '机器人如何<em>工作。</em>',
    'Most fixes are written by bots and checked by people. The maintainers set the rules and can stop any change; you decide whether a fix works in your Archicad.':
        '大多数修复由机器人编写、由人检查。维护者制定规则，并且可以叫停任何改动；修复在你的 Archicad 中是否有效，由你来判断。',
    '<b>01</b><strong>Post it</strong><span>Describe the bug or idea on Discord or GitHub. A GitHub account is not needed on Discord.</span>':
        '<b>01</b><strong>发帖</strong><span>在 Discord 或 GitHub 上描述 bug 或想法。在 Discord 上不需要 GitHub 账号。</span>',
    '<b>02</b><strong>An issue</strong><span>Every half hour, Discord posts that read like a bug report or a feature request become GitHub issues, and the bot replies with the link.</span>':
        '<b>02</b><strong>一个 issue</strong><span>每半小时，看起来像 bug 报告或功能请求的 Discord 帖子会变成 GitHub issue，机器人会回复链接。</span>',
    '<b>03</b><strong>A draft fix</strong><span>The issue bot answers, asks for what it is missing, or writes the fix and opens a pull request. It is built for every Archicad version and reviewed.</span>':
        '<b>03</b><strong>修复草案</strong><span>issue 机器人会回答、询问缺少的信息，或者编写修复并创建 pull request。它会为每个 Archicad 版本构建并经过审查。</span>',
    '<b>04</b><strong>You test</strong><span>A comment on the pull request links the built add-on and asks the person who reported it to try it.</span>':
        '<b>04</b><strong>你来测试</strong><span>pull request 上的一条评论会给出构建好的插件的链接，并请报告问题的人试用。</span>',
    '<b>05</b><strong>Released</strong><span>Green and reviewed, it is merged three days later unless a maintainer stops it, and ships in Monday\'s <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">release</a>.</span>':
        '<b>05</b><strong>发布</strong><span>构建通过并经过审查后，除非维护者叫停，它会在三天后合并，并随周一的<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">版本</a>发布。</span>',

    // Report and test
    'Report and test': '报告与测试',
    'Report': '报告',
    'How to report a bug': '如何报告 bug',
    'Post it on Discord or open an issue on GitHub; both reach the same place. A report that can be fixed in one go says:':
        '在 Discord 上发帖，或在 GitHub 上创建 issue，两者会汇集到同一个地方。一份能一次修好的报告会写明：',
    'your Archicad version and build, and its language (for example 29.0.0 (3100) INT);':
        '你的 Archicad 版本、构建号及语言（例如 29.0.0 (3100) INT）；',
    'your Tapir version (the <em>About Tapir</em> dialog shows it);':
        '你的 Tapir 版本（<em>About Tapir</em> 对话框中可以看到）；',
    'what you did: the script or Grasshopper definition, or the command and the JSON you sent;':
        '你做了什么：脚本或 Grasshopper 定义，或者你发送的命令和 JSON；',
    'what you expected and what happened instead, with the error message;':
        '你预期的结果和实际发生的情况，附上错误信息；',
    'whether the project is a Teamwork project, and which libraries are loaded, when it matters.':
        '在相关时，说明项目是否为 Teamwork 项目，以及加载了哪些库。',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Public</strong> Issues and Discord channels are public. Leave out client names, project names and file paths you don\'t want to share.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> 公开</strong> issue 和 Discord 频道都是公开的。请不要写入你不想公开的客户名称、项目名称和文件路径。',
    'Open an issue on GitHub': '在 GitHub 上创建 issue',
    'Post on Discord': '在 Discord 上发帖',
    'Test': '测试',
    'How to test a fix': '如何测试修复',
    'A fix is only known to work once someone has run it in Archicad. The bots can build Tapir but cannot run Archicad, so your test is what matters most.':
        '只有在有人于 Archicad 中运行过之后，才能确定修复有效。机器人可以构建 Tapir，却无法运行 Archicad，所以你的测试最为重要。',
    'Open the pull request linked from your issue. A comment on it lists the downloads, one per Archicad version and platform. Downloading needs a GitHub login.':
        '打开你的 issue 中链接的 pull request。其中一条评论列出了下载文件，每个 Archicad 版本和平台各一个。下载需要登录 GitHub。',
    'Unzip the download for your Archicad version. On macOS it holds another zip with the <code>.bundle</code>.':
        '解压与你的 Archicad 版本对应的下载文件。在 macOS 上，里面还有一个包含 <code>.bundle</code> 的 zip。',
    'In Archicad, open <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, remove or disable the installed Tapir, then <em>Add</em> the <code>.apx</code> (Windows) or <code>.bundle</code> (macOS) and click OK.':
        '在 Archicad 中打开 <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>，移除或停用已安装的 Tapir，然后用 <em>Add</em> 添加 <code>.apx</code>（Windows）或 <code>.bundle</code>（macOS），并点击 OK。',
    'Do what went wrong before, in a copy of your project.':
        '在项目的副本中，重复之前出错的操作。',
    'Say on the pull request whether it works and fixes your case. Afterwards, put your installed Tapir back the same way.':
        '在 pull request 上说明它是否有效、是否解决了你的问题。之后，用同样的方法恢复你原来安装的 Tapir。',
    'Testing often? The <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> tests open pull requests on your own Archicad while it is closed, leaves your Tapir in place and posts the result. It runs on Windows and needs Python and a GitHub account.':
        '经常测试？<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> 会在你的 Archicad 关闭时，用它测试未合并的 pull request，不动你安装的 Tapir，并发布结果。它在 Windows 上运行，需要 Python 和 GitHub 账号。',

    // Momentum (community.js fills in the numbers)
    '{sign}{diff} in {days} days': '{days} 天内 {sign}{diff}',
    'Momentum': '动态',
    'Commits': '提交',
    'Discord messages': 'Discord 消息',
    'Discord members <span class="change"></span>': 'Discord 成员 <span class="change"></span>',
    'People taking part, last 12 months': '参与人数（最近 12 个月）',
    'Top contributors': '主要贡献者',
    'Counted: commits (co-authors included), issues they reported that a merged fix closed, and pull requests they tested in Archicad (passed or failed).':
        '统计内容：提交（包括共同作者）、本人报告并由已合并的修复关闭的 issue，以及本人在 Archicad 中测试过的 pull request（无论通过与否）。',
    '{count} commit': '{count} 次提交',
    '{count} commits': '{count} 次提交',
    '{count} report': '{count} 次报告',
    '{count} reports': '{count} 次报告',
    '{count} test': '{count} 次测试',
    '{count} tests': '{count} 次测试',

    // Growth
    'Commits, issues and pull requests, and Discord messages per month, stacked, with the number of Discord members as a line, {first} to {last}. The table below lists every number.':
        '每月的提交、issue 和 pull request 以及 Discord 消息（堆叠显示），Discord 成员数以折线表示，{first} 至 {last}。下方表格列出了所有数字。',
    'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec': '1月 2月 3月 4月 5月 6月 7月 8月 9月 10月 11月 12月',
    '{month} {year}': '{year}年{month}',
    'Growth': '增长',
    'Growth since <em>2022.</em>': '自 <em>2022</em> 年以来的增长。',
    'Everything that happens around Tapir each month, from the first commit at Enzyme to today. Only numbers are collected, never the text of a message.':
        '每个月围绕 Tapir 发生的一切，从在 Enzyme 的第一次提交到今天。只统计数字，从不收集消息的内容。',
    '<i class="swatch layer-commits"></i>Commits': '<i class="swatch layer-commits"></i>提交',
    '<i class="swatch layer-github"></i>Issues and pull requests': '<i class="swatch layer-github"></i>issue 和 pull request',
    '<i class="swatch layer-discordMessages"></i>Discord messages': '<i class="swatch layer-discordMessages"></i>Discord 消息',
    '<i class="swatch swatch-line"></i>Discord members, right scale': '<i class="swatch swatch-line"></i>Discord 成员（右侧刻度）',
    'Per month, stacked, on a square-root scale so the early years stay visible. Commits by people, without merges and bots; Discord messages include threads and forum posts. Discord members are the people in the server at the end of each month, counted by when they joined; people who later left are not included.':
        '按月堆叠，采用平方根刻度，让早年的数据也清晰可见。提交只计人的提交，不含合并提交和机器人；Discord 消息包括子区和论坛帖子。Discord 成员是每月月底服务器中的人数，按加入时间统计；之后离开的人不计在内。',
    'Show the numbers as a table': '以表格显示数字',
    'Month': '月份',
    'Issues opened': '新建 issue',
    'Pull requests merged': '已合并的 pull request',
    'Discord members': 'Discord 成员',

    // Help build it
    'Help build it': '一起参与',
    'Made a script your office uses? Share it, and any office can install it from the palette.':
        '写了一个你的事务所在用的脚本？分享出来，任何事务所都能从面板中安装它。',
    'How to share': '如何分享',
    'Ask for a feature': '请求新功能',
    'How features are requested, and what is being asked for now.': '如何请求功能，以及目前大家在请求什么。',
    'See the roadmap': '查看路线图',
    'Code': '代码',
    'Write a command': '编写命令',
    'The add-on is C++ on the Archicad API. The development guide shows how to add a command.':
        '插件基于 Archicad API，用 C++ 编写。开发指南介绍了如何添加命令。',
    'Read the guide': '阅读指南',
    'Get involved': '参与进来',
    'We\'re building a motivated, diverse community around Archicad automation.':
        '我们正在围绕 Archicad 自动化建设一个积极而多元的社区。',
    'Say hello on Discord, and tell us what you automate in Archicad.':
        '来 Discord 打个招呼，告诉我们你在 Archicad 中自动化了什么。',
    'Join the Discord': '加入 Discord'
});
