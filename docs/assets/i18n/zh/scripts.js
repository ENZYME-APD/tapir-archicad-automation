/* 中文 (简体): the Scripts page (scripts/index.html), with the texts scripts.js shows from it. See ../README.md.
   The scripts' own names, summaries and READMEs stay as their authors wrote them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Free Archicad Python scripts | Tapir': '免费的 Archicad Python 脚本 | Tapir',
    'Free Python scripts for the Tapir palette in Archicad, shared by architects, each one reviewed and adapted to work in any office before it is listed.':
        '用于 Archicad Tapir 面板的免费 Python 脚本，由建筑师分享，每个脚本在上架前都经过审查，并调整为可在任何事务所使用。',
    '<a href="../">Tapir</a> / Scripts': '<a href="../">Tapir</a> / 脚本',
    'The script <em>schedule.</em>': '脚本<em>清单。</em>',
    'Scripts shared by architects. Each one is checked for safety and adapted to work in any office and any Archicad language before it is listed.':
        '建筑师分享的脚本。每个脚本在上架前都经过安全检查，并调整为可在任何事务所、任何语言版本的 Archicad 中使用。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Opening soon</strong> The scripts move to this page, and the list appears here as soon as the first ones are in. Until then, the scripts shared so far are on the <a href="https://tapir-marketplace.vercel.app/">current marketplace</a>, and you can <a href="share/">share your own</a>.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> 即将开放</strong> 脚本将迁移到此页面，第一批脚本上架后，列表就会显示在这里。在此之前，已分享的脚本在<a href="https://tapir-marketplace.vercel.app/">现有的市场</a>上，你也可以<a href="share/">分享自己的脚本</a>。',

    // The built-in scripts, shown until the marketplace has listings
    'Built in to <em>Tapir.</em>': 'Tapir <em>内置脚本。</em>',
    'These are five of the scripts that come with the Tapir palette: open it in Archicad, pick one and press Run Script. Each is a short Python file you can read and change.':
        '以下是 Tapir 面板自带的五个脚本：在 Archicad 中打开面板，选择一个并点击 Run Script。每个都是可以阅读和修改的简短 Python 文件。',
    'Built-in script': '内置脚本',
    'Number elements along a line': '沿一条线为构件编号',
    'Select a polyline and the doors, zones or objects to number. Each one gets its Element ID in the order the polyline passes it.':
        '选择一条多段线以及要编号的门、区域或对象。每个构件按多段线经过的顺序获得 Element ID。',
    'Colour the model by a property': '按属性为模型着色',
    'Pick a property; every element gets a colour for its value, with a legend of the values and how many elements have each. Number values can be grouped into ranges.':
        '选择一个属性，每个构件按其值获得一种颜色，并附有图例列出各个值及其构件数量。数值可以分组为区间。',
    'Check a project before it goes out': '在交付前检查项目',
    'Lists elements without an ID, zones with the same number, unclassified and duplicate elements, empty stories and unused layers. Export the list to Excel or turn it into Issues.':
        '列出没有 ID 的构件、编号相同的区域、未分类和重复的构件、空楼层以及未使用的图层。可将列表导出到 Excel 或转为 Issues。',
    'Publish on a schedule': '按计划发布',
    'Publishes the chosen Publisher Sets now or on a schedule, every few hours or daily at a set time. It can receive the Teamwork changes and update the drawings first, and logs every run.':
        '立即或按计划发布所选的 Publisher Sets，每隔几小时或每天在固定时间发布。它可以先接收 Teamwork 更改并更新图纸，并记录每次运行。',
    'Tidy up the View Map': '整理 View Map',
    'Finds the views that are neither placed on a layout nor published, and moves the ones you choose into a separate folder that keeps their folder structure.':
        '找出既未放置在布图上也未发布的视图，并把你选择的视图移到一个单独的文件夹中，保留其文件夹结构。',

    // The schedule
    'Browse scripts': '浏览脚本',
    'Search scripts': '搜索脚本',
    'Search: name, task or author': '搜索：名称、任务或作者',
    'Category': '类别',
    'All': '全部',
    'Archicad version': 'Archicad 版本',
    'Any Archicad': '任意 Archicad',
    'Filters': '筛选',
    'Active filters': '已应用的筛选',
    'Remove filter: {name}': '移除筛选：{name}',
    '1 script': '1 个脚本',
    '{count} scripts': '{count} 个脚本',
    'No script matches. Try fewer words or another filter.': '没有匹配的脚本。请减少关键词或换一个筛选条件。',
    'Marketplace · sorted by category': '市场 · 按类别排序',
    'Script': '脚本',
    'Effect': '作用',
    'Author': '作者',
    'Changes the model': '修改模型',
    'Writes files': '写入文件',
    'Read only': '只读',
    'Not stated': '未说明',

    // The categories
    'Quality assurance': '质量检查',
    'Quantities and scheduling': '工程量与清单',
    'Documentation and layouts': '文档与布图',
    'Zones and areas': '区域与面积',
    'Element data and properties': '构件数据与属性',
    'Modeling and geometry': '建模与几何',
    'Classifications and attributes': '分类与属性设置',
    'Navigator and views': '导航器与视图',
    'Libraries and objects': '库与对象',
    'Import and export': '导入与导出',
    'Batch automation': '批量自动化',
    'Utilities': '实用工具',

    // Install and share
    'Install and share': '安装与分享',
    'Install': '安装',
    'Install a script': '安装脚本',
    'Open a script here and follow its <em>Install in Tapir</em> steps. In short: download it into a folder and add that folder to the Tapir palette. The website is for browsing; scripts run in Archicad.':
        '在这里打开一个脚本，按照其<em>在 Tapir 中安装</em>的步骤操作。简而言之：把它下载到一个文件夹，再把该文件夹添加到 Tapir 面板。网站用于浏览，脚本在 Archicad 中运行。',
    'Step by step in Get started': '在“快速开始”中逐步了解',
    'Wrote a script that saves you time? Share it, and we help you make it ready for other offices. No GitHub account needed.':
        '写了一个能帮你省时间的脚本？分享出来，我们帮你让它适用于其他事务所。无需 GitHub 账号。',
    'How to share a script': '如何分享脚本',
    'Join Discord': '加入 Discord',

    // Not found
    '<a href="../">Tapir</a> / <a href="./">Scripts</a> / Not found': '<a href="../">Tapir</a> / <a href="./">脚本</a> / 未找到',
    'Script not <em>found.</em>': '未找到<em>脚本。</em>',
    'There is no script with this name on the list, or the list could not be read just now.':
        '列表中没有这个名称的脚本，或者暂时无法读取列表。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Not found</strong> Try again later, or see <a href="./">all scripts</a>.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> 未找到</strong> 请稍后再试，或查看<a href="./">全部脚本</a>。',

    // One script
    '<a href="../">Tapir</a> / <a href="./">Scripts</a> / <span id="detail-crumb"></span>': '<a href="../">Tapir</a> / <a href="./">脚本</a> / <span id="detail-crumb"></span>',
    'Screenshot {number} of {name}': '{name} 的截图 {number}',
    'What it does': '功能说明',
    'More screenshots': '更多截图',
    'Versions': '版本',
    'Code': '代码',
    'About this script': '关于此脚本',
    'Version': '版本',
    '<span id="detail-tapir"></span> or later': '<span id="detail-tapir"></span> 或更高版本',
    'Version <span class="version-number"></span>': '版本 <span class="version-number"></span>',
    'Archicad <span class="version-archicad"></span> · Tapir <span class="version-tapir"></span> or later':
        'Archicad <span class="version-archicad"></span> · Tapir <span class="version-tapir"></span> 或更高版本',
    'Install in Tapir': '在 Tapir 中安装',
    'Check that Archicad and Tapir are new enough: see <em>Archicad</em> and <em>Tapir</em> above.':
        '确认 Archicad 和 Tapir 的版本足够新：见上方的 <em>Archicad</em> 和 <em>Tapir</em>。',
    'When your Tapir palette lists scripts to install, find <strong class="detail-name-ref"></strong> there and install it.':
        '当你的 Tapir 面板列出可安装的脚本时，在其中找到 <strong class="detail-name-ref"></strong> 并安装。',
    'Until then, download the script into a folder and add that folder to the palette, as <a href="../start/#marketplace">Get started</a> shows.':
        '在此之前，把脚本下载到一个文件夹，再把该文件夹添加到面板，具体见<a href="../start/#marketplace">快速开始</a>。',
    'Open the project, select what the script asks for, and run it from the palette.':
        '打开项目，选中脚本所需的内容，然后从面板运行它。',
    'This script changes the model: read what it changes under <em>What it does</em> before you run it, and try it on a copy first.':
        '此脚本会修改模型：运行前请在<em>功能说明</em>中阅读它修改的内容，并先在副本上试用。',
    'This script writes files, for example a report or an export, and changes nothing in the model.':
        '此脚本会写入文件，例如报告或导出文件，不会修改模型。',
    'This script only reads: it changes nothing in the model and writes no files.':
        '此脚本只读取：不修改模型，也不写入文件。',
    'Download the script': '下载脚本',
    'View on GitHub': '在 GitHub 上查看',
    'Report a problem': '报告问题',

    // Next step
    'Next step': '下一步',
    'All scripts': '全部脚本',
    'Back to the schedule of every script.': '返回所有脚本的清单。',
    'Browse scripts': '浏览脚本',
    'Made one of your own?': '自己写了一个？',
    'Share it, and any office can install it from the palette.': '分享出来，任何事务所都能从面板安装它。',
    'How sharing works': '分享的流程'
});
