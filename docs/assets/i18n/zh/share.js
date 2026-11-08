/* 中文 (简体): the Share page (scripts/share/index.html). See ../README.md.
   Channel, label, file and code names, and the palette's Share command, stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Share an Archicad script | Tapir': '分享 Archicad 脚本 | Tapir',
    'Share an Archicad script you wrote: send it on Discord or as a pull request, and it is checked, adapted to work in any office and listed with your name.':
        '分享你写的 Archicad 脚本：通过 Discord 或 pull request 发送，它会经过检查、调整为可在任何事务所使用，并以你的名字上架。',
    '<a href="../../">Tapir</a> / <a href="../">Scripts</a> / Share': '<a href="../../">Tapir</a> / <a href="../">脚本</a> / 分享',
    'Share a script <em>from Archicad.</em>': '从 Archicad <em>分享脚本。</em>',
    'No GitHub account, no setup. Your name stays on it, and everyone who uses Tapir can install it.':
        '无需 GitHub 账号，无需任何设置。脚本上保留你的名字，所有 Tapir 用户都能安装它。',

    // What happens
    'What happens': '流程',
    '<b aria-hidden="true">1</b><strong>Send it</strong><span>Post the script in <code>#share-a-script</code> on Discord, with a few words on what it does. The Tapir palette is getting a Share button that does the same: you see the whole file before anything is sent. It arrives as a GitHub issue labelled <code>script-share</code>.</span>':
        '<b aria-hidden="true">1</b><strong>发送</strong><span>在 Discord 的 <code>#share-a-script</code> 中发布脚本，并简单说明它的作用。Tapir 面板即将加入一个 Share 按钮，作用相同：发送任何内容之前，你都能看到完整的文件。它会成为一个带 <code>script-share</code> 标签的 GitHub issue。</span>',
    '<b aria-hidden="true">2</b><strong>Adapted for any office</strong><span>Tapir\'s bot, an AI, opens a pull request: hard-coded layers, properties and paths become settings, risky code is removed, and it works in every Archicad language, in metric and imperial.</span>':
        '<b aria-hidden="true">2</b><strong>适配任何事务所</strong><span>Tapir 的机器人（一个 AI）会创建一个 pull request：写死的图层、属性和路径变成设置项，有风险的代码被移除，脚本可在任何语言版本的 Archicad 中、在公制和英制下使用。</span>',
    '<b aria-hidden="true">3</b><strong>You confirm, it\'s listed</strong><span>Try the adapted version and say it still does what you meant. Once the pull request is reviewed and merged, it appears on the <a href="../">Scripts</a> page with your name.</span>':
        '<b aria-hidden="true">3</b><strong>你确认后上架</strong><span>试用调整后的版本，确认它仍然实现你想要的功能。pull request 经过审查并合并后，脚本就会以你的名字出现在<a href="../">脚本</a>页面上。</span>',

    // Three ways
    'Three ways to send it': '三种发送方式',
    'Coming to the palette': '即将加入面板',
    'From the Tapir palette': '从 Tapir 面板',
    'Right-click your script and choose Share: add a few words about it and send. Until your palette has it, use Discord.':
        '右键单击你的脚本并选择 Share：写几句说明后发送。在你的面板具备此功能之前，请使用 Discord。',
    'Easiest today': '目前最简单',
    'On Discord': '通过 Discord',
    'Post the script in the <code>#share-a-script</code> channel, with a few words on what it does and which Archicad version you use.':
        '在 <code>#share-a-script</code> 频道发布脚本，简单说明它的作用以及你使用的 Archicad 版本。',
    'Join Discord': '加入 Discord',
    'If you use GitHub': '如果你使用 GitHub',
    'As a pull request': '通过 pull request',
    'Add a folder <code>marketplace/&lt;name&gt;/</code> with the script, a <code>listing.json</code> and a README, as the marketplace README describes.':
        '按照市场 README 的说明，添加一个文件夹 <code>marketplace/&lt;name&gt;/</code>，其中包含脚本、一个 <code>listing.json</code> 和一个 README。',
    'The marketplace folder': '市场文件夹',

    // The small print
    'The small print': '细则',
    'What is sent': '发送的内容',
    'Only the script': '只有脚本',
    'The file, your one-line description, the name to credit, and your Archicad and Tapir versions. Nothing from the open project.':
        '文件、你的一句话说明、署名，以及你的 Archicad 和 Tapir 版本。不包含打开的项目中的任何内容。',
    'Free for everyone': '对所有人免费',
    'Every shared script is free and open source. Unless you choose another open-source licence, yours is published under the <a class="inline" href="https://opensource.org/license/mit">MIT licence</a>: anyone may use, change and share it, as long as they keep your name and the licence with it. It comes without warranty. You stay its author, and its page credits you.':
        '每个分享的脚本都是免费、开源的。除非你选择其他开源许可证，你的脚本将以 <a class="inline" href="https://opensource.org/license/mit">MIT 许可证</a>发布：任何人都可以使用、修改和分享它，只要保留你的名字和许可证。它不附带任何担保。你仍是它的作者，它的页面会注明你。',
    'Only share a script you wrote, or one whose author agreed to share it.':
        '只分享你自己写的脚本，或作者同意分享的脚本。',

    // What the AI changes
    'What the AI changes': 'AI 会修改什么',
    'What the AI <em>changes.</em>': 'AI 会<em>修改什么。</em>',
    'It keeps what your script does. It changes how, so the script is safe and works in other offices, and lists every change in plain words in the pull request.':
        '它保留脚本的功能，只改变实现方式，让脚本安全并能在其他事务所使用，并在 pull request 中用通俗的语言列出每一处修改。',
    'Made safe': '确保安全',
    'Removed, or flagged for a maintainer': '移除，或标记给维护者',
    'connections to anything other than Archicad on your own computer;':
        '连接你本机 Archicad 以外的任何东西；',
    'running other programs or code built from text (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>);':
        '运行其他程序或由文本拼成的代码（<code>subprocess</code>、<code>os.system</code>、<code>eval</code>、<code>exec</code>）；',
    'installing packages while it runs (<code>pip install</code>);':
        '运行时安装软件包（<code>pip install</code>）；',
    'deleting or overwriting files, and writing anywhere but the project\'s folder and <code>Documents/Tapir</code>;':
        '删除或覆盖文件，以及写入项目文件夹和 <code>Documents/Tapir</code> 以外的位置；',
    'passwords, tokens and keys;': '密码、令牌和密钥；',
    'user names, <code>C:\\Users\\…</code> paths, and project, client and office names, in the code and its comments.':
        '代码及其注释中的用户名、<code>C:\\Users\\…</code> 路径，以及项目、客户和事务所名称。',
    'Made to fit other offices': '适配其他事务所',
    'Settings instead of your names': '用设置项代替你的名称',
    'Property names, layers, stories, paths and classification codes you typed in become settings at the top of the script, or fields of its window.':
        '你手动输入的属性名、图层、楼层、路径和分类编码，会变成脚本开头的设置项或其窗口中的字段。',
    'Built-in properties are found by their ID, so the script works in every language of Archicad.':
        '内置属性通过 ID 查找，因此脚本可在任何语言版本的 Archicad 中使用。',
    'Lengths and areas work in metric and imperial units; paths work on Windows and macOS.':
        '长度和面积支持公制和英制单位；路径在 Windows 和 macOS 上都能使用。',
    'Made safe to run': '运行安全',
    'No surprises': '没有意外',
    'It says what it is about to change, and does nothing, saying so, when nothing fits.':
        '它会说明将要修改的内容；没有符合条件的对象时，它什么也不做，并给出提示。',
    'It makes its changes in one Undo step where it can.':
        '在可能的情况下，它的修改只占一个撤销步骤。',
    'When a property or element type it needs is missing, it shows a clear message instead of crashing.':
        '缺少所需的属性或构件类型时，它会显示清楚的提示，而不是崩溃。',
    'Described': '附带说明',
    'A README in plain words': '一份通俗易懂的 README',
    'What it does, how to use it, its settings, what it changes, its limits, and <em>Original by</em> you.':
        '它的功能、用法、设置、修改的内容、局限，以及注明你的 <em>Original by</em>。',
    'Its category, the Archicad and Tapir versions it works with, and whether it changes the model, writes files or only reads.':
        '它的类别、适用的 Archicad 和 Tapir 版本，以及它是修改模型、写入文件还是只读取。',

    // Review
    'Review': '审查',
    'Before it is merged': '合并之前',
    'What the review checks': '审查检查什么',
    'Every pull request is reviewed before it is merged, like any other code in Tapir. The review checks that:':
        '和 Tapir 中的其他代码一样，每个 pull request 在合并前都会经过审查。审查会检查：',
    'nothing on the <em>Made safe</em> list is left in, unless a maintainer agreed to it;':
        '<em>确保安全</em>清单上的内容没有残留，除非维护者同意保留；',
    'the script still does what you wrote it for, and you confirmed that;':
        '脚本仍能实现你编写它的目的，并且你已确认；',
    'the folder has the files and fields the <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">marketplace format</a> asks for;':
        '文件夹包含<a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">市场格式</a>要求的文件和字段；',
    'the README is accurate, and the screenshots show no client, project or people.':
        'README 准确无误，截图中没有客户、项目或人物。',
    'Next step': '下一步',
    'Post your script': '发布你的脚本',
    'Share it in <code>#share-a-script</code> on Discord, and we take it from there.':
        '在 Discord 的 <code>#share-a-script</code> 中分享，剩下的交给我们。',
    'Browse scripts': '浏览脚本'
});
