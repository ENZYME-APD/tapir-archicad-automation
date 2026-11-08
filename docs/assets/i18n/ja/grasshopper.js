/* 日本語: the Grasshopper page (grasshopper/index.html). See ../README.md.
   Component, input and button names stay English, as Grasshopper shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad Grasshopper live connection | Tapir': 'Archicad と Grasshopper のライブ接続 | Tapir',
    'Connect Grasshopper live to a running Archicad with the Tapir plugin: read, change and create Archicad elements from Rhino geometry, without writing code.':
        'Tapir プラグインで Grasshopper を起動中の Archicad にライブ接続し、コードを書かずに、Rhino のジオメトリから Archicad の要素を読み取り、変更し、作成します。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper':
        '<a href="../">Tapir</a> / <a href="../docs/">ドキュメント</a> / Grasshopper',
    'Grasshopper, <em>visually.</em>': 'Grasshopper で、<em>ビジュアルに。</em>',
    'The Tapir plugin connects Grasshopper to a running Archicad: read elements and their settings, change them, and create new ones from Rhino geometry, without writing code.':
        'Tapir プラグインは Grasshopper を起動中の Archicad につなぎます。要素とその設定を読み取り、変更し、Rhino のジオメトリから新しい要素を作成できます。コードを書く必要はありません。',

    // What the components do
    'What the components <em>do.</em>': 'コンポーネントで<em>できること。</em>',
    'Each component sends a Tapir command to Archicad and gives you the answer as Grasshopper data. You find them on the <strong>Tapir</strong> tab, grouped as in Archicad.':
        '各コンポーネントは Tapir コマンドを Archicad に送り、その応答を Grasshopper のデータとして返します。コンポーネントは <strong>Tapir</strong> タブにあり、Archicad と同じようにグループ分けされています。',
    '<b>READ</b><strong>Elements and their data</strong><span>Elements by type, story or selection; their properties, classifications, settings and geometry.</span>':
        '<b>読み取り</b><strong>要素とそのデータ</strong><span>タイプ、フロア、選択による要素の取得と、そのプロパティ、分類、設定、ジオメトリ。</span>',
    '<b>CHANGE</b><strong>What you pass in</strong><span>Property values, classifications, layers, IDs and tool settings of the elements you pass in.</span>':
        '<b>変更</b><strong>渡したもの</strong><span>渡した要素のプロパティ値、分類、レイヤー、ID、ツール設定。</span>',
    '<b>CREATE</b><strong>From Rhino geometry</strong><span>Walls, doors, windows, railings, level dimensions and more, from points and curves.</span>':
        '<b>作成</b><strong>Rhino のジオメトリから</strong><span>点や曲線から、壁、ドア、窓、手すり、レベル寸法など。</span>',
    '<b>PROJECT</b><strong>The rest of the file</strong><span>Stories, attributes, navigator views, layouts, issues, favorites and libraries.</span>':
        '<b>プロジェクト</b><strong>ファイルのその他の部分</strong><span>フロア、属性、ナビゲータのビュー、レイアウト、課題、お気に入り、ライブラリ。</span>',
    'Which Tapir commands have a component, and which settings each covers, is on the <a href="../gh-coverage/">Grasshopper coverage</a> page.':
        'どの Tapir コマンドにコンポーネントがあり、それぞれがどの設定に対応しているかは、<a href="../gh-coverage/">Grasshopper カバレッジ</a>のページにあります。',

    // Tool components
    'Tool components': 'ツールコンポーネント',
    'One per Archicad <em>tool.</em>': 'Archicad の<em>ツールごとに一つ。</em>',
    'A tool component works like the Archicad tool of the same name. One JSON input and one JSON output per panel of its settings dialog, keyed by the labels you see there, such as <code>Wall Height</code>.':
        'ツールコンポーネントは、同じ名前の Archicad ツールと同じように動作します。設定ダイアログのパネルごとに JSON の入力と出力が一つずつあり、キーはそこに表示される英語のラベル（<code>Wall Height</code> など）です。',
    'Placed by <code>StartPoint</code> and <code>EndPoint</code>.': '<code>StartPoint</code> と <code>EndPoint</code> で配置します。',
    'Placed in an <code>OwnerWall</code> at a <code>CenterOffset</code>.': '<code>OwnerWall</code> の <code>CenterOffset</code> の位置に配置します。',
    'Placed along a <code>ReferenceLine</code>.': '<code>ReferenceLine</code> に沿って配置します。',
    'Placed at a <code>Position</code>, optionally on a parent element.': '<code>Position</code> に配置します。親要素の上にも配置できます。',
    '<b>REFRESH</b><strong>Reads</strong><span>Reads the elements in <code>ElementGuid</code> and fills every output.</span>':
        '<b>REFRESH</b><strong>読み取る</strong><span><code>ElementGuid</code> の要素を読み取り、すべての出力を埋めます。</span>',
    '<b>EXECUTE</b><strong>Writes</strong><span>Writes the given settings to those elements. Only the keys you give are written.</span>':
        '<b>EXECUTE</b><strong>書き込む</strong><span>指定した設定をそれらの要素に書き込みます。書き込まれるのは指定したキーだけです。</span>',
    '<b>CREATE</b><strong>Makes new elements</strong><span>From the geometry inputs, starting from the tool\'s defaults or from a <code>FavoriteName</code>.</span>':
        '<b>CREATE</b><strong>新しい要素を作る</strong><span>ジオメトリの入力から、ツールの既定値または <code>FavoriteName</code> を基に作成します。</span>',
    'Nothing is written to Archicad until you press Execute or Create. A JSON output read by Refresh can be fed straight back into the same input. Archicad has no command to change railings or level dimensions after they are placed, so for those Execute only writes the ID, layer and draw order.':
        'Execute か Create を押すまで、Archicad には何も書き込まれません。Refresh で読み取った JSON 出力は、そのまま同じ入力に戻せます。Archicad には配置後の手すりやレベル寸法を変更するコマンドがないため、これらについて Execute が書き込むのは ID、レイヤー、表示順序だけです。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Note</strong> Walls remembers the walls its Create made, in the <code>.gh</code> file. The next Create updates them instead of making new ones. Turn this off, or forget them, in the component\'s menu.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> 注意</strong> Walls は、自身の Create で作成した壁を <code>.gh</code> ファイルに記憶します。次の Create では新しく作らずにそれらを更新します。この動作はコンポーネントのメニューでオフにしたり、記憶を消したりできます。',

    // Install
    'Install': 'インストール',
    'Install the Tapir Add-On in Archicad': 'Archicad に Tapir アドオンをインストールする',
    'The components talk to Archicad through the Add-On. <a href="../start/#install">Get started</a> shows how.':
        'コンポーネントはアドオンを通じて Archicad とやり取りします。方法は<a href="../start/#install">はじめる</a>を参照してください。',
    'Install the <span class="mono">tapir</span> package in Rhino': 'Rhino に <span class="mono">tapir</span> パッケージをインストールする',
    'In Rhino 7 or 8, open <em>Tools &gt; Package Manager</em> (or type <code>PackageManager</code>), search for <strong>tapir</strong> and click Install. Restart Rhino.':
        'Rhino 7 または 8 で <em>Tools &gt; Package Manager</em> を開き（または <code>PackageManager</code> と入力し）、<strong>tapir</strong> を検索して Install をクリックします。その後 Rhino を再起動します。',
    'Open Archicad, then Grasshopper': 'Archicad を開いてから Grasshopper を開く',
    'With a project open in Archicad, start Grasshopper. The <strong>Tapir</strong> tab appears in its toolbar. With more than one Archicad open, give the port of the one you want to the <strong>ConnectArchicad</strong> component (<em>Tapir &gt; General</em>); the first Archicad uses 19723.':
        'Archicad でプロジェクトを開いた状態で Grasshopper を起動すると、ツールバーに <strong>Tapir</strong> タブが表示されます。複数の Archicad を開いている場合は、使いたい Archicad のポートを <strong>ConnectArchicad</strong> コンポーネント（<em>Tapir &gt; General</em>）に渡します。最初の Archicad は 19723 を使います。',

    // A first definition
    'A first definition': '最初の定義',
    'One <em>wall.</em>': '壁を<em>一枚。</em>',
    'Build this in an empty Grasshopper canvas, with a project open in Archicad:':
        'Archicad でプロジェクトを開いた状態で、空の Grasshopper キャンバスに次のものを組み立てます。',
    'Place two <strong>Point</strong> parameters and set them to 0,0,0 and 5,0,0. Archicad reads the numbers as metres.':
        '<strong>Point</strong> パラメータを二つ置き、0,0,0 と 5,0,0 に設定します。Archicad は数値をメートルとして読み取ります。',
    'Place the <strong>Walls</strong> component from <em>Tapir &gt; Design Tools</em>. Connect the points to <code>StartPoint</code> and <code>EndPoint</code>.':
        '<em>Tapir &gt; Design Tools</em> から <strong>Walls</strong> コンポーネントを置き、点を <code>StartPoint</code> と <code>EndPoint</code> につなぎます。',
    'Place a <strong>Panel</strong> with the text on the right and connect it to <code>GeometryAndPositioning_JSON</code>.':
        '右のテキストを入れた <strong>Panel</strong> を置き、<code>GeometryAndPositioning_JSON</code> につなぎます。',
    'Press <strong>Create</strong>. A 5 m wall appears in Archicad, 3 m high and 30 cm thick.':
        '<strong>Create</strong> を押します。Archicad に長さ 5 m、高さ 3 m、厚さ 30 cm の壁が現れます。',
    'Move a point and press Create again: the same wall moves. Change the panel and press Create: the same wall changes.':
        '点を動かしてもう一度 Create を押すと、同じ壁が移動します。パネルを変えて Create を押すと、同じ壁が変わります。',
    'Press <strong>Refresh</strong> to see every setting of the wall in the JSON outputs. Copy a key from there to set it the next time.':
        '<strong>Refresh</strong> を押すと、壁のすべての設定が JSON 出力に表示されます。そこからキーをコピーすれば、次回その設定を指定できます。',

    // Next step
    'Next step': '次のステップ',
    'Install the Add-On': 'アドオンをインストールする',
    'Install Tapir and the <span class="mono">tapir</span> package, then try the wall above.':
        'Tapir と <span class="mono">tapir</span> パッケージをインストールして、上の壁を試してみましょう。',
    'Get started': 'はじめる',
    'Coverage': 'カバレッジ',
    'Which commands have a component, and which inputs each still lacks.':
        'どのコマンドにコンポーネントがあり、それぞれにまだどの入力が足りないか。',
    'See the coverage': 'カバレッジを見る',
    'Videos': '動画',
    'Watch it work': '動くところを見る',
    'The Archicad and Grasshopper tutorials.': 'Archicad と Grasshopper のチュートリアル。',
    'Watch the tutorials': 'チュートリアルを見る'
});
