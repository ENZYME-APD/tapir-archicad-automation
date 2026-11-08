/* 日本語: the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Archicad JSON API commands reference | Tapir': 'Archicad JSON API コマンドリファレンス | Tapir',
    'Every JSON command the Tapir add-on adds to Archicad, with its parameters, its response and a Python example, and how to call one from Python.':
        'Tapir アドオンが Archicad に追加するすべての JSON コマンドを、パラメーター、レスポンス、Python の例とともに掲載し、Python から呼び出す方法も説明します。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">ドキュメント</a> / コマンド',
    'Tapir <em>commands.</em>': 'Tapir <em>コマンド。</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'Tapir アドオンが Archicad に登録する JSON コマンドです。Graphisoft の公式コマンドと並んで、<code>TapirCommand</code> 名前空間にあります。各コマンドにパラメーター、レスポンス、例を示します。',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script. Looking for something no command does? <a href="../limits/">What the Archicad API can\'t do</a> lists what Graphisoft\'s API does not offer, with what to do instead.':
        'Tapir は初めてですか？<a href="../start/">はじめる</a>で、インストールと最初のスクリプトの実行方法を説明しています。どのコマンドにもできないことを探していますか？<a href="../limits/">Archicad API にできないこと</a>に、Graphisoft の API が提供していないものと代わりにできることをまとめています。',
    'Calling a <em>command.</em>': 'コマンドの<em>呼び出し。</em>',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (the next Archicads opened use 19724, 19725 and so on). A Tapir command is sent through Archicad\'s <code>API.ExecuteAddOnCommand</code> command.':
        'プロジェクトを開いている間、Archicad は <code>http://127.0.0.1:19723</code> で JSON コマンドに応答します（続けて開いた Archicad は 19724、19725 などを使います）。Tapir コマンドは Archicad の <code>API.ExecuteAddOnCommand</code> コマンドを通じて送ります。',
    'Python standard library': 'Python 標準ライブラリ',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>':
        'Graphisoft の <a href="https://pypi.org/project/archicad/"><code>archicad</code> パッケージ</a>',
    'The examples of the commands below use <code>aclib</code>, the small helper in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> that sends commands the same way as the standard-library script.':
        '以下のコマンドの例では、<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> にある小さなヘルパー <code>aclib</code> を使います。標準ライブラリのスクリプトと同じ方法でコマンドを送ります。',
    'All <em>commands.</em>': 'すべての<em>コマンド。</em>',

    // The two coverage pages
    'Overview': '概要',
    'By group': 'グループ別',
    'Checklist': 'チェックリスト',

    // The Archicad API coverage (api-coverage/)
    'Archicad API coverage checklist | Tapir': 'Archicad API カバレッジチェックリスト | Tapir',
    'Which parts of Graphisoft\'s Archicad API the Tapir add-on uses, item by item, kept up to date from every issue, pull request and push to the repository.':
        'Tapir アドオンが Graphisoft の Archicad API のどの部分を使っているかを項目ごとに示します。リポジトリへの issue、pull request、push のたびに更新されます。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">ドキュメント</a> / API カバレッジ',
    'Archicad API <em>coverage.</em>': 'Archicad API <em>カバレッジ。</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Tapir が Graphisoft の Archicad API の何をカバーしているかを、項目ごとに示します。',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        '<a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir アドオン</a>が Archicad API（<span id="archicad-range">サポートするすべての Archicad バージョン</span>：旧名称とコマンド ID を含む C 関数、および C++ API）のどの部分を使っているか。issue と main への push のたびに数分以内、pull request のたびに 1 時間以内に更新されます。<a href="../archicad-addon/">コマンドリファレンス</a>もご覧ください。',
    'By part of the API': 'API の部分別',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage of the Tapir commands | Tapir': 'Tapir コマンドの Grasshopper カバレッジ | Tapir',
    'Which Tapir commands for Archicad the components of the Tapir Grasshopper plugin call, and whether they cover each command\'s parameters or still miss some.':
        'Tapir Grasshopper プラグインのコンポーネントが Archicad 用のどの Tapir コマンドを呼び出し、各コマンドのパラメーターをカバーしているか、まだ足りないものがあるか。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">ドキュメント</a> / Grasshopper カバレッジ',
    'Grasshopper <em>coverage.</em>': 'Grasshopper <em>カバレッジ。</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Grasshopper コンポーネントがどの Tapir コマンドを呼び出し、どのパラメーターがまだ足りないか。',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Tapir Grasshopper プラグインのコンポーネントが <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir アドオン</a>のどの JSON コマンドを呼び出し、そのコンポーネントがコマンドのパラメーターをカバーしているか。issue と main への push のたびに数分以内、pull request のたびに 1 時間以内に更新されます。これはプラグインのコードの静的チェックです。コンポーネントがどのパラメーターを指定しているかを示すもので、それぞれが動作することを示すものではありません。<a href="../archicad-addon/">コマンドリファレンス</a>と <a href="../api-coverage/">Archicad API カバレッジ</a>もご覧ください。',
    'Parameters of the used commands': '使用中のコマンドのパラメーター'
});
