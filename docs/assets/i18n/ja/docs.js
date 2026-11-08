/* 日本語: the Docs page (docs/index.html). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad automation docs | Tapir': 'Archicad 自動化ドキュメント | Tapir',
    'Tapir\'s documentation: every JSON command, the Grasshopper guide, the Archicad API coverage and the Grasshopper coverage, and how a call to Tapir works.':
        'Tapir のドキュメント：すべての JSON コマンド、Grasshopper ガイド、Archicad API と Grasshopper のカバー状況、そして Tapir の呼び出しの仕組み。',
    '<a href="../">Tapir</a> / Docs': '<a href="../">Tapir</a> / ドキュメント',
    'The <em>reference.</em>': '<em>リファレンス。</em>',
    'Every command, what Tapir covers of the Archicad API, and how the Grasshopper components map to it.':
        'すべてのコマンド、Tapir が Archicad API のどこをカバーしているか、そして Grasshopper コンポーネントとの対応。',
    'The docs': 'ドキュメント',

    // The four docs
    'Tapir commands': 'Tapir コマンド',
    'Search by name, filter by the version a command came in, and open any command for its input and output schema and a Python example.':
        '名前で検索し、コマンドが追加されたバージョンで絞り込み、各コマンドの入力・出力スキーマと Python の例を表示できます。',
    'Open the reference': 'リファレンスを開く',
    'Grasshopper guide': 'Grasshopper ガイド',
    'The tool components, one per Archicad tool, with inputs named after the settings dialog. Install, a first definition, and what the buttons do.':
        'Archicad のツールごとに 1 つのツールコンポーネント。入力名は設定ダイアログに合わせてあります。インストール、最初の定義、各ボタンの働き。',
    'Read the guide': 'ガイドを読む',
    'Archicad API': 'Archicad API',
    '· <span data-live-value="coverage"></span> covered': '· <span data-live-value="coverage"></span> をカバー',
    'Every function of Graphisoft\'s Archicad API and whether Tapir uses it, is working on it, or skipped it, updated every hour.':
        'Graphisoft の Archicad API のすべての関数と、Tapir がそれを使っているか、対応中か、見送ったか。1 時間ごとに更新。',
    'See the coverage': 'カバー状況を見る',
    'Components': 'コンポーネント',
    'Which Tapir commands have a Grasshopper component, and which inputs each component still lacks.':
        'どの Tapir コマンドに Grasshopper コンポーネントがあり、各コンポーネントにどの入力がまだ足りないか。',

    // The API limits teaser (its title is in common.js)
    'What the Archicad API can\'t do, such as undo, the clipboard or the rows of a schedule, with what to do instead and the DevKit evidence.':
        '元に戻す、クリップボード、一覧表の行など、Archicad API にできないことを、代わりにできることと DevKit での根拠とともにまとめています。',
    'See the limits': '限界を見る',

    // How it works
    'How it works': '仕組み',
    'How it <em>works.</em>': '<em>仕組み。</em>',
    'Tapir is an Archicad add-on that turns native API functions into a stateless JSON server on port 19723. Anything that can send an HTTP request can drive Archicad.':
        'Tapir は、ネイティブの API 関数をポート 19723 上のステートレスな JSON サーバーにする Archicad アドオンです。HTTP リクエストを送れるものなら何でも Archicad を操作できます。',
    'Grasshopper, Python scripts and AI tools send JSON over HTTP to the Tapir Add-On, which calls Archicad\'s C++ API on the BIM model.':
        'Grasshopper、Python スクリプト、AI ツールが HTTP で JSON を Tapir アドオンに送り、アドオンが BIM モデルに対して Archicad の C++ API を呼び出します。',
    '<strong>Grasshopper</strong><span>The tapir components</span>': '<strong>Grasshopper</strong><span>tapir コンポーネント</span>',
    '<strong>Python scripts</strong><span>The palette, or your own</span>': '<strong>Python スクリプト</strong><span>パレットのもの、または自作</span>',
    '<strong>AI and MCP</strong><span>Assistants and MCP servers</span>': '<strong>AI と MCP</strong><span>アシスタントと MCP サーバー</span>',
    '↓ JSON over HTTP ↓': '↓ HTTP で JSON ↓',
    '<strong>Tapir Add-On</strong><span>A stateless JSON layer: one command per request</span>':
        '<strong>Tapir アドオン</strong><span>ステートレスな JSON 層：1 リクエストに 1 コマンド</span>',
    '<strong>Archicad</strong><span>The BIM model and its C++ API</span>': '<strong>Archicad</strong><span>BIM モデルとその C++ API</span>',

    // Anatomy of a call
    'Anatomy of a call': '呼び出しの構造',
    'Every Tapir command is sent the same way; only <code>commandName</code> and <code>addOnCommandParameters</code> change. <a href="../start/#python">Your first Python script</a>':
        'Tapir のコマンドはどれも同じ方法で送ります。変わるのは <code>commandName</code> と <code>addOnCommandParameters</code> だけです。<a href="../start/#python">最初の Python スクリプト</a>',

    // More guides
    'More guides': 'その他のガイド',
    'Install and run a script': 'インストールしてスクリプトを実行',
    'Install Tapir, run a script from the palette and write your first Python script.':
        'Tapir をインストールし、パレットからスクリプトを実行し、最初の Python スクリプトを書きます。',
    'Get started': 'はじめる',
    'AI': 'AI',
    'MCP servers and scripts an AI assistant writes for you.': 'MCP サーバーと、AI アシスタントが書いてくれるスクリプト。',
    'Use Tapir with AI': 'AI で Tapir を使う',
    'Learn by watching': '見て学ぶ',
    'Six videos, from installing Tapir to whole workflows.': 'Tapir のインストールからワークフロー全体まで、6 本の動画。',
    'Watch the tutorials': 'チュートリアルを見る'
});
