/* 中文 (简体): the Roadmap page (roadmap/index.html). See ../README.md.
   Command names, the label enhancement and the names of the issue form and of the old list stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Tapir roadmap – Archicad automation': 'Tapir 路线图 – Archicad 自动化',
    'How to ask for a feature in Tapir, the Archicad add-on, what people are asking for now, what shipped from the old list and which ideas are still open.':
        '如何向 Archicad 插件 Tapir 提出功能请求、目前大家在请求什么、旧清单中哪些已经发布，以及哪些想法仍待实现。',
    '<a href="../">Tapir</a> / <a href="../community/">Community</a> / Roadmap':
        '<a href="../">Tapir</a> / <a href="../community/">社区</a> / 路线图',
    'What\'s <em>next.</em>': '接下来<em>是什么。</em>',
    'Tapir grows from requests. Most new commands, components and fixes start as a message from someone who needed them in their own work.':
        'Tapir 因请求而成长。大多数新命令、运算器和修复，都始于某个人在自己的工作中需要它们时发来的一条消息。',

    // How to ask for a feature
    'How to ask for a <em>feature.</em>': '如何请求<em>新功能。</em>',
    'Four steps from "I need this" to a Monday release.': '从“我需要这个”到周一发布，只需四步。',
    'Say what you want to do': '说出你想做什么',
    'Post it on Discord or open an issue on GitHub. Describe the task in Archicad, not only the command: "number the doors along a line", "read the area of every zone". A Discord message that reads like a feature request becomes a GitHub issue by itself, labelled <code>enhancement</code>.':
        '在 Discord 上发帖，或在 GitHub 上创建 issue。描述你在 Archicad 中要完成的任务，而不只是命令：“沿一条线给门编号”“读取每个区域的面积”。读起来像功能请求的 Discord 消息会自动变成 GitHub issue，并带有 <code>enhancement</code> 标签。',
    'Asking for a part of the Archicad API?': '想要 Archicad API 的某个部分？',
    'The <a href="../api-coverage/">API coverage</a> page lists every function of the Archicad API and whether Tapir has a command for it. Use the <em>Archicad API request</em> issue form to ask for one that is missing.':
        '<a href="../api-coverage/">API 覆盖率</a>页面列出了 Archicad API 的每个函数，以及 Tapir 是否有对应的命令。缺少的函数可以用 <em>Archicad API request</em> issue 表单来请求。',
    'The issue bot picks it up': 'issue 机器人接手',
    'A clear, wanted request is built like a fix: the bot writes it and opens a pull request, and you are asked to try it. A large one is split into parts. When the design is open, the bot describes the choices and a maintainer decides.':
        '清晰且确实需要的请求会像修复一样被实现：机器人编写代码并创建 pull request，然后请你试用。大的请求会拆分成几部分。设计尚未确定时，机器人会列出可选方案，由维护者决定。',
    'It ships on a Monday': '周一发布',
    'Merged changes are released every Monday. The release notes list them, and the add-on offers to update itself.':
        '已合并的更改每周一发布。发布说明会列出它们，插件也会提示自动更新。',

    // What is being asked for now
    'What is being asked for now': '目前的请求',
    'Requests': '请求',
    'Open feature requests': '未完成的功能请求',
    'Every open issue labelled <code>enhancement</code>. Add a comment or a reaction to one you need too.':
        '所有带 <code>enhancement</code> 标签的未关闭 issue。如果你也需要某一项，请留言或点个表情回应。',
    'See the requests': '查看请求',
    'Plan': '计划',
    'Project board': '项目看板',
    'The maintainers\' board of planned and ongoing work.': '维护者的看板，列出计划中和进行中的工作。',
    'Open the board': '打开看板',
    'In progress': '进行中',
    'Open pull requests': '未合并的 pull request',
    'Changes being built, reviewed and tested right now.': '正在开发、审查和测试的更改。',
    'See the pull requests': '查看 pull request',
    'Shipped': '已发布',
    'Releases': '版本',
    'Every release, with its notes and downloads. The <span class="nowrap">command reference</span> shows the version each command came in.':
        '每个版本及其说明和下载。<span class="nowrap">命令参考</span>标明了每个命令是在哪个版本中加入的。',
    'See the releases': '查看版本',

    // Recently shipped
    'Recently shipped': '最近发布',
    'Already <em>here.</em>': '已经<em>实现。</em>',
    'Asked for on the old site\'s "What\'s cooking" list, and now part of Tapir.':
        '这些曾列在旧网站的“What\'s cooking”清单上，现在已是 Tapir 的一部分。',
    'Stairs, roofs and curtain walls': '楼梯、屋顶和幕墙',
    'Read and change them: <a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>, <a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a> and <a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>.':
        '读取和修改它们：<a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>、<a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a> 和 <a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>。',
    'Building materials and composites': '建筑材料和复合结构',
    'Create them from a script: <a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a> and <a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>.':
        '用脚本创建它们：<a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a> 和 <a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>。',
    'Dialogs from Python': '来自 Python 的对话框',
    'A script shows its own window in Archicad: <a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a> and <a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>.':
        '脚本在 Archicad 中显示自己的窗口：<a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a> 和 <a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>。',
    '2D detailing': '二维详图',
    'Lines, fills and text: <a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>, <a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a> and <a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>.':
        '线、填充和文本：<a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>、<a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a> 和 <a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>。',
    'Your own properties': '你自己的属性',
    'Add property definitions from a script: <a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>.':
        '用脚本添加属性定义：<a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>。',
    'Built-in scripts': '内置脚本',
    'Ready to run from the palette, from numbering to a project health check. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">See them on GitHub</a>.':
        '可直接从面板运行，从编号到项目健康检查。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">在 GitHub 上查看</a>。',

    // Ideas
    'Ideas': '想法',
    'Still <em>open.</em>': '仍待<em>实现。</em>',
    'Two ideas from the old list that nobody has asked for in an issue yet. Want one? Open the issue.':
        '旧清单上的两个想法，还没有人在 issue 中提出。想要其中一个？那就创建 issue 吧。',
    'Idea': '想法',
    'Clearer error messages in Python': '更清晰的 Python 错误信息',
    'Tracebacks and command errors that say what went wrong and how to fix it, so a script is easier to troubleshoot.':
        '让 traceback 和命令错误说明哪里出了问题以及如何修复，让脚本更容易排查。',
    'Ask for it': '请求它',
    'Tapir scripts in CI': '在 CI 中运行 Tapir 脚本',
    'Running Tapir scripts automatically, for example in a GitHub Actions pipeline.':
        '自动运行 Tapir 脚本，例如在 GitHub Actions 流水线中。',

    // Next step
    'Next step': '下一步',
    'Missing something in your work?': '工作中缺少什么？',
    'Tell us what you want to automate': '告诉我们你想自动化什么',
    'Open an issue, or describe it on Discord and the bot files it for you.':
        '创建一个 issue，或在 Discord 上描述，机器人会替你提交。',
    'Request a feature on GitHub': '在 GitHub 上请求功能',
    'Ask on Discord': '在 Discord 上提问'
});
