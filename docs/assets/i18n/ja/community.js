/* 日本語: the Community page (community/index.html, with the texts community/community.js reads
   from it). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Community – Archicad automation with Tapir': 'コミュニティ – Tapir による Archicad 自動化',
    'Ask on Discord, report a bug, test a fix in your Archicad: how Tapir\'s community and its bots turn a report into a release, and how the project grew.':
        'Discord で質問し、バグを報告し、お使いの Archicad で修正をテストする。Tapir のコミュニティとボットが報告をリリースにつなげる仕組みと、プロジェクトの成長の歩みです。',
    '<a href="../">Tapir</a> / Community': '<a href="../">Tapir</a> / コミュニティ',
    'Ask. Report. <em>Test.</em>': '質問する。報告する。<em>テストする。</em>',
    'Architects, developers and the maintainers in one place. Bugs and ideas become fixes that someone with your Archicad version tests before they ship.':
        '建築家、開発者、メンテナーが一か所に集まっています。バグやアイデアは修正になり、リリース前にあなたと同じ Archicad バージョンを使う人がテストします。',

    // Where to find us
    'Where to find us': '私たちの居場所',
    'Ask anything': '何でも質問',
    'Architects, developers and the maintainers, in one server. Show what you made and talk with the people who build Tapir.':
        '建築家、開発者、メンテナーがひとつのサーバーに集まっています。作ったものを見せ、Tapir を開発している人たちと話しましょう。',
    'Join Discord': 'Discord に参加',
    'Learn the API': 'API を学ぶ',
    'A video series from the first command to full workflows.': '最初のコマンドから本格的なワークフローまでを扱う動画シリーズです。',
    'Watch the tutorials': 'チュートリアルを見る',
    'AI assistants': 'AI アシスタント',
    'Talk to your model': 'モデルと対話する',
    'MCP servers let Claude and other assistants read and change a project through Tapir.':
        'MCP サーバーを使うと、Claude などのアシスタントが Tapir を通じてプロジェクトを読み取り、変更できます。',

    // How the bots work
    'From report to release': '報告からリリースまで',
    'How the bots <em>work.</em>': 'ボットの<em>仕組み。</em>',
    'Most fixes are written by bots and checked by people. The maintainers set the rules and can stop any change; you decide whether a fix works in your Archicad.':
        'ほとんどの修正はボットが書き、人が確認します。メンテナーがルールを決め、どの変更でも止めることができます。修正があなたの Archicad で動くかどうかを決めるのはあなたです。',
    '<b>01</b><strong>Post it</strong><span>Describe the bug or idea on Discord or GitHub. A GitHub account is not needed on Discord.</span>':
        '<b>01</b><strong>投稿する</strong><span>バグやアイデアを Discord か GitHub に書きます。Discord なら GitHub アカウントは不要です。</span>',
    '<b>02</b><strong>An issue</strong><span>Every half hour, Discord posts that read like a bug report or a feature request become GitHub issues, and the bot replies with the link.</span>':
        '<b>02</b><strong>Issue になる</strong><span>30 分ごとに、バグ報告や機能リクエストらしい Discord の投稿が GitHub の Issue になり、ボットがそのリンクを返信します。</span>',
    '<b>03</b><strong>A draft fix</strong><span>The issue bot answers, asks for what it is missing, or writes the fix and opens a pull request. It is built for every Archicad version and reviewed.</span>':
        '<b>03</b><strong>修正案</strong><span>Issue ボットが回答するか、足りない情報を尋ねるか、修正を書いてプルリクエストを作成します。プルリクエストはすべての Archicad バージョン向けにビルドされ、レビューされます。</span>',
    '<b>04</b><strong>You test</strong><span>A comment on the pull request links the built add-on and asks the person who reported it to try it.</span>':
        '<b>04</b><strong>あなたがテスト</strong><span>プルリクエストのコメントにビルドされたアドオンへのリンクが載り、報告した人に試してもらうよう依頼します。</span>',
    '<b>05</b><strong>Released</strong><span>Green and reviewed, it is merged three days later unless a maintainer stops it, and ships in Monday\'s <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">release</a>.</span>':
        '<b>05</b><strong>リリース</strong><span>ビルドが通りレビューが済むと、メンテナーが止めない限り 3 日後にマージされ、月曜日の<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">リリース</a>に含まれます。</span>',

    // Report and test
    'Report and test': '報告とテスト',
    'Report': '報告',
    'How to report a bug': 'バグの報告方法',
    'Post it on Discord or open an issue on GitHub; both reach the same place. A report that can be fixed in one go says:':
        'Discord に投稿するか、GitHub で Issue を作成してください。どちらも同じ場所に届きます。一度で修正できる報告には次のことが書かれています。',
    'your Archicad version and build, and its language (for example 29.0.0 (3100) INT);':
        'Archicad のバージョンとビルド、その言語版（例: 29.0.0 (3100) INT）',
    'your Tapir version (the <em>About Tapir</em> dialog shows it);':
        'Tapir のバージョン（<em>About Tapir</em> ダイアログに表示されます）',
    'what you did: the script or Grasshopper definition, or the command and the JSON you sent;':
        '行ったこと: スクリプトや Grasshopper 定義、または送信したコマンドと JSON',
    'what you expected and what happened instead, with the error message;':
        '期待した結果と実際に起きたこと、およびエラーメッセージ',
    'whether the project is a Teamwork project, and which libraries are loaded, when it matters.':
        '関係する場合は、プロジェクトが Teamwork プロジェクトかどうかと、読み込まれているライブラリ',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Public</strong> Issues and Discord channels are public. Leave out client names, project names and file paths you don\'t want to share.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> 公開</strong> Issue と Discord のチャンネルは公開されています。共有したくない施主名、プロジェクト名、ファイルパスは書かないでください。',
    'Open an issue on GitHub': 'GitHub で Issue を作成',
    'Post on Discord': 'Discord に投稿',
    'Test': 'テスト',
    'How to test a fix': '修正のテスト方法',
    'A fix is only known to work once someone has run it in Archicad. The bots can build Tapir but cannot run Archicad, so your test is what matters most.':
        '修正が動くと分かるのは、誰かが Archicad で実行してからです。ボットは Tapir をビルドできても Archicad を実行できないため、あなたのテストが何より大切です。',
    'Open the pull request linked from your issue. A comment on it lists the downloads, one per Archicad version and platform. Downloading needs a GitHub login.':
        'Issue からリンクされているプルリクエストを開きます。そのコメントに、Archicad のバージョンとプラットフォームごとのダウンロードが並んでいます。ダウンロードには GitHub へのログインが必要です。',
    'Unzip the download for your Archicad version. On macOS it holds another zip with the <code>.bundle</code>.':
        'お使いの Archicad バージョン用のダウンロードを解凍します。macOS では、中にさらに <code>.bundle</code> を含む zip があります。',
    'In Archicad, open <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, remove or disable the installed Tapir, then <em>Add</em> the <code>.apx</code> (Windows) or <code>.bundle</code> (macOS) and click OK.':
        'Archicad で <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em> を開き、インストール済みの Tapir を削除または無効にしてから、<em>Add</em> で <code>.apx</code>（Windows）または <code>.bundle</code>（macOS）を追加し、OK をクリックします。',
    'Do what went wrong before, in a copy of your project.':
        'プロジェクトのコピーで、以前うまくいかなかった操作をもう一度行います。',
    'Say on the pull request whether it works and fixes your case. Afterwards, put your installed Tapir back the same way.':
        '動作したか、問題が解決したかをプルリクエストに書いてください。その後、同じ方法で元の Tapir に戻します。',
    'Testing often? The <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> tests open pull requests on your own Archicad while it is closed, leaves your Tapir in place and posts the result. It runs on Windows and needs Python and a GitHub account.':
        '頻繁にテストしますか？ <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> は、Archicad を閉じている間にオープンなプルリクエストをあなたの Archicad でテストし、インストール済みの Tapir はそのままにして、結果を投稿します。Windows で動作し、Python と GitHub アカウントが必要です。',

    // Momentum (community.js fills in the numbers)
    '{sign}{diff} in {days} days': '{days} 日間で {sign}{diff}',
    'Momentum': '勢い',
    'Commits': 'コミット',
    'Discord messages': 'Discord メッセージ',
    'Discord members <span class="change"></span>': 'Discord メンバー <span class="change"></span>',
    'People taking part, last 12 months': '参加している人（過去 12 か月）',
    'Top contributors': '主な貢献者',
    'Counted: commits (co-authors included), issues they reported that a merged fix closed, and pull requests they tested in Archicad (passed or failed).':
        '数えているもの：コミット（共同作成者を含む）、本人が報告し、マージされた修正で閉じられた Issue、本人が Archicad でテストしたプルリクエスト（成功・失敗を問わず）。',
    '{count} commit': '{count} コミット',
    '{count} commits': '{count} コミット',
    '{count} report': '{count} 件の報告',
    '{count} reports': '{count} 件の報告',
    '{count} test': '{count} 件のテスト',
    '{count} tests': '{count} 件のテスト',

    // Growth
    'Commits, issues and pull requests, and Discord messages per month, stacked, with the number of Discord members as a line, {first} to {last}. The table below lists every number.':
        '月ごとのコミット、Issue とプルリクエスト、Discord メッセージの積み上げグラフと、Discord メンバー数の折れ線（{first}〜{last}）。下の表にすべての数値があります。',
    'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec': '1月 2月 3月 4月 5月 6月 7月 8月 9月 10月 11月 12月',
    '{month} {year}': '{year}年{month}',
    'Growth': '成長',
    'Growth since <em>2022.</em>': '<em>2022 年</em>からの成長。',
    'Everything that happens around Tapir each month, from the first commit at Enzyme to today. Only numbers are collected, never the text of a message.':
        'Enzyme での最初のコミットから今日まで、Tapir の周りで毎月起きていることのすべてです。集めるのは数値だけで、メッセージの本文は決して集めません。',
    '<i class="swatch layer-commits"></i>Commits': '<i class="swatch layer-commits"></i>コミット',
    '<i class="swatch layer-github"></i>Issues and pull requests': '<i class="swatch layer-github"></i>Issue とプルリクエスト',
    '<i class="swatch layer-discordMessages"></i>Discord messages': '<i class="swatch layer-discordMessages"></i>Discord メッセージ',
    '<i class="swatch swatch-line"></i>Discord members, right scale': '<i class="swatch swatch-line"></i>Discord メンバー（右目盛り）',
    'Per month, stacked, on a square-root scale so the early years stay visible. Commits by people, without merges and bots; Discord messages include threads and forum posts. Discord members are the people in the server at the end of each month, counted by when they joined; people who later left are not included.':
        '月ごとの積み上げで、初期の年も見えるように平方根の目盛りを使っています。コミットは人によるもので、マージとボットを除きます。Discord メッセージにはスレッドとフォーラムの投稿を含みます。Discord メンバーは各月末にサーバーにいた人数で、参加日で数えています。後で退出した人は含みません。',
    'Show the numbers as a table': '数値を表で表示',
    'Month': '月',
    'Issues opened': '作成された Issue',
    'Pull requests merged': 'マージされたプルリクエスト',
    'Discord members': 'Discord メンバー',

    // Help build it
    'Help build it': '開発に参加する',
    'Made a script your office uses? Share it, and any office can install it from the palette.':
        '事務所で使っているスクリプトを作りましたか？共有すれば、どの事務所でもパレットからインストールできます。',
    'How to share': '共有の方法',
    'Ask for a feature': '機能をリクエスト',
    'How features are requested, and what is being asked for now.': '機能のリクエスト方法と、いまリクエストされているもの。',
    'See the roadmap': 'ロードマップを見る',
    'Code': 'コード',
    'Write a command': 'コマンドを書く',
    'The add-on is C++ on the Archicad API. The development guide shows how to add a command.':
        'アドオンは Archicad API 上の C++ で書かれています。開発ガイドでコマンドの追加方法を説明しています。',
    'Read the guide': 'ガイドを読む',
    'Get involved': '参加しよう',
    'We\'re building a motivated, diverse community around Archicad automation.':
        '私たちは Archicad の自動化を軸に、意欲的で多様なコミュニティを築いています。',
    'Say hello on Discord, and tell us what you automate in Archicad.':
        'Discord であいさつして、Archicad で何を自動化しているか教えてください。',
    'Join the Discord': 'Discord に参加'
});
