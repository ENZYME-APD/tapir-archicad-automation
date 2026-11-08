/* 中文 (简体): the About page (about/index.html). See ../README.md.
   The names of the people stay as they are. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'About Tapir – open-source Archicad add-on': '关于 Tapir – 开源 Archicad 插件',
    'Why the open-source Archicad add-on Tapir exists, how it grew from Enzyme\'s R&D and Tibor Lorántfy\'s JSON template, who builds it and how to support it.':
        '开源 Archicad 插件 Tapir 为何存在，它如何从 Enzyme 的研发和 Tibor Lorántfy 的 JSON 模板成长起来，由谁开发，以及如何支持它。',
    '<a href="../">Tapir</a> / About': '<a href="../">Tapir</a> / 关于',
    'About <em>Tapir.</em>': '关于 <em>Tapir。</em>',
    'An open-source project that makes Archicad scriptable for architects, not only for programmers. Started in 2022, released every week since.':
        '一个让建筑师（而不仅是程序员）也能用脚本驱动 Archicad 的开源项目。始于 2022 年，此后每周发布。',

    // Why
    'Why it <em>exists.</em>': '为何<em>存在。</em>',
    'Archicad\'s own JSON API covers only part of what its add-on API can do. Tapir adds the missing commands as an add-on, then puts them within reach of people who don\'t code: a palette of ready scripts, Grasshopper components, and AI assistants.':
        'Archicad 自带的 JSON API 只覆盖了其插件 API 能力的一部分。Tapir 以插件形式补上缺少的命令，再让不写代码的人也能用上它们：现成脚本的面板、Grasshopper 组件和 AI 助手。',

    // How it started
    'Revision history': '修订历史',
    'How it <em>started.</em>': '它是如何<em>开始的。</em>',
    'Started as R&amp;D at Enzyme, an architecture practice, then opened to everyone, in the spirit of ifc.js: practitioners shouldn\'t wait for a vendor to support their workflow.':
        '最初是建筑事务所 Enzyme 的研发项目，后来本着 ifc.js 的精神向所有人开放：从业者不应等待厂商来支持自己的工作流程。',
    'Revisions · Tapir': '修订记录 · Tapir',
    'Rev': '版次',
    'Date': '日期',
    'Description': '说明',
    'By': '贡献者',
    '5 July 2022': '2022 年 7 月 5 日',
    'Born in Enzyme\'s R&amp;D': '诞生于 Enzyme 的研发',
    'Archicad\'s JSON interface was a step forward, but Grasshopper users still needed C++ add-ons to reach native elements. The first commit, with the MIT licence.':
        'Archicad 的 JSON 接口是一大进步，但 Grasshopper 用户仍需要 C++ 插件才能访问原生构件。第一次提交，采用 MIT 许可证。',
    '7 July 2022': '2022 年 7 月 7 日',
    'A place to talk': '交流的地方',
    'The Discord server opens, where architects and developers say what they need and show what they made.':
        'Discord 服务器开放，建筑师和开发者在这里说出需求，展示自己的作品。',
    'The first members': '首批成员',
    'July 2024': '2024 年 7 月',
    'The template': '模板',
    'Tibor Lorántfy, whose archicad-additional-json-commands the add-on is built on, joins the project. Almost any function of the Archicad API can now become a JSON command.':
        'Tibor Lorántfy 加入项目，插件正是基于他的 archicad-additional-json-commands 构建。如今 Archicad API 的几乎所有函数都能成为 JSON 命令。',
    'Built by its community': '由社区共建',
    'A release every Monday. Reports on Discord become GitHub issues, bots draft the fixes and people test them in their own Archicad. <a href="../community/#activity">See the growth</a>.':
        '每周一发布。Discord 上的报告变成 GitHub issue，机器人起草修复，再由人们在自己的 Archicad 中测试。<a href="../community/#activity">查看增长</a>。',
    'Everyone who reports, tests and contributes': '所有报告、测试和贡献的人',

    // The people
    'The people': '人们',
    'The people <em>behind it.</em>': '背后的<em>人们。</em>',
    'ENZYME-APD runs the project and publishes Tapir under the MIT licence. Everyone below has written code for it, or tested it and given the feedback it is built on, in alphabetical order.':
        'ENZYME-APD 负责运营该项目，并以 MIT 许可证发布 Tapir。以下每个人都为它编写过代码，或测试过它并提供了它赖以构建的反馈，按字母顺序排列。',
    'Many more report bugs, test fixes in their own Archicad and share scripts; bots draft fixes, and people test and approve them. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/graphs/contributors">All contributors on GitHub</a>':
        '还有更多人报告问题、在自己的 Archicad 中测试修复并分享脚本；机器人起草修复，再由人来测试和批准。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/graphs/contributors">GitHub 上的全部贡献者</a>',

    // Principles
    'Principles': '原则',
    '<b>FREE</b><strong>No price, ever</strong><span>MIT licence for the add-on, the plugin and every shared script.</span>':
        '<b>免费</b><strong>永远免费</strong><span>Add-On、Grasshopper 插件和每个共享脚本都采用 MIT 许可证。</span>',
    '<b>OPEN</b><strong>Built in public</strong><span>Every issue, pull request and review is on GitHub.</span>':
        '<b>开放</b><strong>公开开发</strong><span>每个 issue、pull request 和审查都在 GitHub 上。</span>',
    '<b>SIGNED</b><strong>Safe to install</strong><span>The Windows installer is signed through SignPath; the macOS installer and add-ons are signed and notarized.</span>':
        '<b>已签名</b><strong>安全安装</strong><span>Windows 安装程序通过 SignPath 签名；macOS 安装程序和 Add-On 均已签名并经过公证。</span>',
    '<b>WEEKLY</b><strong>Small, steady releases</strong><span>A release every Monday when something changed.</span>':
        '<b>每周</b><strong>小步、稳定的发布</strong><span>只要有改动，每周一发布一次。</span>',

    // Support
    'Support Tapir': '支持 Tapir',
    'Support <em>Tapir.</em>': '支持 <em>Tapir。</em>',
    'Tapir is free and open source. This is how it stays up to date with every Archicad version and keeps growing.':
        'Tapir 免费且开源。以下方式让它跟上每个 Archicad 版本并不断成长。',
    'Everyone': '每个人',
    'Community crowdfunding': '社区众筹',
    'Small contributions from many users make a big difference: regular small pledges can fund maintainers who update Tapir for each new Archicad version.':
        '众多用户的小额支持能带来巨大改变：定期的小额捐助可以资助维护者为每个新的 Archicad 版本更新 Tapir。',
    'Practices and firms': '事务所与企业',
    'Corporate sponsoring': '企业赞助',
    'Offices and engineering firms that use Tapir in their own R&amp;D sponsor the commands their projects need, and those commands then serve everyone.':
        '在自身研发中使用 Tapir 的事务所和工程公司赞助其项目所需的命令，这些命令随后惠及所有人。',
    'Your office': '您的事务所',
    'Training and services': '培训与服务',
    'Training sessions, implementation workshops and custom development for offices moving to computational BIM workflows with Tapir.':
        '为借助 Tapir 转向计算化 BIM 工作流程的事务所提供培训课程、实施工作坊和定制开发。',
    'Talk to us: <strong class="nowrap">tapir@weareenzyme.com</strong>': '联系我们：<strong class="nowrap">tapir@weareenzyme.com</strong>',
    'Free code signing provided by SignPath.io, certificate by SignPath Foundation. Archicad is a trademark of Graphisoft SE; Tapir is not made by Graphisoft.':
        '免费代码签名由 SignPath.io 提供，证书由 SignPath Foundation 颁发。Archicad 是 Graphisoft SE 的商标；Tapir 并非由 Graphisoft 开发。'
});
