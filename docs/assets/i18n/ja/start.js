/* 日本語: the Get started page (start/index.html). See ../README.md.
   The menu, palette and button names of Archicad and Tapir stay English, as the add-on shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Automate Archicad with Python – get started | Tapir': 'Python で Archicad を自動化 – はじめる | Tapir',
    'Install Tapir, run a script from the Tapir palette in Archicad, add more scripts, write a first Python script and install the Grasshopper plugin.':
        'Tapir をインストールし、Archicad の Tapir パレットからスクリプトを実行し、スクリプトを追加し、最初の Python スクリプトを書き、Grasshopper プラグインをインストールします。',
    '<a href="../">Tapir</a> / Get started': '<a href="../">Tapir</a> / はじめる',
    'Get <em>started.</em>': 'さあ、<em>はじめよう。</em>',
    'From download to your first script in about ten minutes. Steps 1 to 3 need no programming.':
        'ダウンロードから最初のスクリプトまで約 10 分。ステップ 1〜3 にプログラミングは不要です。',
    '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Prefer watching? The <a href="../tutorials/">Tapir video tutorials</a> walk through installing Tapir and using it.':
        '<strong class="callout-label"><span aria-hidden="true">▶</span> 動画</strong> 動画で見たい方は、<a href="../tutorials/">Tapir の動画チュートリアル</a>（英語）で Tapir のインストールと使い方を順に紹介しています。',

    // 1. Install
    'Install Tapir': 'Tapir をインストール',
    '<strong>With the installer (recommended).</strong> It finds every Archicad version on the computer and installs the matching add-on into each, so Archicad loads it on its next start.':
        '<strong>インストーラーを使う（推奨）。</strong>コンピューター上のすべての Archicad バージョンを見つけ、それぞれに対応するアドオンをインストールします。Archicad は次回起動時にそれを読み込みます。',
    'Download <span class="os">WINDOWS</span>': 'ダウンロード <span class="os">WINDOWS</span>',
    'Download <span class="os">MACOS</span>': 'ダウンロード <span class="os">MACOS</span>',
    '<strong>Windows:</strong> the installer asks for administrator rights, because it copies the add-on into the Archicad folder under Program Files.':
        '<strong>Windows：</strong>インストーラーは Program Files 内の Archicad フォルダーにアドオンをコピーするため、管理者権限を求めます。',
    '<strong>macOS:</strong> unzip the download and open the app.':
        '<strong>macOS：</strong>ダウンロードしたファイルを解凍し、アプリを開きます。',
    '<strong>Prefer to install by hand?</strong> Download the <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> or <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> file for your Archicad version from the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">latest release</a>, then in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, browse to the file and click <em>OK</em>.':
        '<strong>手動でインストールしたい場合：</strong><a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">最新リリース</a>からお使いの Archicad バージョン用の <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> または <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> をダウンロードし、Archicad で <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em> を開いてファイルを選び、<em>OK</em> をクリックします。',
    'Tapir keeps itself up to date: it tells you when a new version is out and installs it for you.':
        'Tapir は自動で最新の状態を保ちます。新しいバージョンが出ると知らせ、インストールまで行います。',

    // 2. Run a script
    'Run your first script': '最初のスクリプトを実行',
    'Tapir adds a <em>Tapir</em> menu to Archicad. Choose <em>Tapir Palette</em> from it, pick a script in the palette\'s list and click <em>Run Script</em>. The built-in scripts are there from the start: try <em>ProjectHealthCheck</em>, which lists elements without an ID, zones with the same number, unclassified elements and more. It only reads the model.':
        'Tapir は Archicad に <em>Tapir</em> メニューを追加します。そこから <em>Tapir Palette</em> を選び、パレットの一覧でスクリプトを選んで <em>Run Script</em> をクリックします。組み込みスクリプトは最初から入っています。<em>ProjectHealthCheck</em> を試してみてください。ID のない要素、同じ番号のゾーン、未分類の要素などを一覧にします。モデルを読み取るだけです。',
    'Tapir runs the scripts with <a href="https://docs.astral.sh/uv/">uv</a>, which brings the Python and the packages each script needs. If uv is not installed yet, Tapir offers to install it the first time you run a script.':
        'Tapir はスクリプトを <a href="https://docs.astral.sh/uv/">uv</a> で実行します。uv は各スクリプトに必要な Python とパッケージを用意します。uv がまだインストールされていなければ、最初にスクリプトを実行するときに Tapir がインストールを提案します。',
    '<em>Manage Shortcuts</em> on the palette puts up to six scripts in the Tapir menu, as <em>Run Script Shortcut 1</em> to <em>6</em>.':
        'パレットの <em>Manage Shortcuts</em> で、最大 6 つのスクリプトを <em>Run Script Shortcut 1</em>〜<em>6</em> として Tapir メニューに置けます。',

    // 3. More scripts
    'Add more scripts': 'スクリプトを追加',
    'The <a href="../scripts/">Scripts</a> page collects scripts shared by the community, each one reviewed before it is listed. The Tapir palette is getting a window to find and install them in one click.':
        '<a href="../scripts/">スクリプト</a>ページには、コミュニティが共有したスクリプトが集まっています。どれも掲載前にレビューされています。Tapir パレットには、それらをワンクリックで探してインストールできるウィンドウが追加される予定です。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Until then</strong> If your palette has no such window yet, add a script by hand, as below.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> それまでは</strong> パレットにまだそのウィンドウがない場合は、以下のように手動でスクリプトを追加します。',
    'Download the scripts you want into a folder, then tell Tapir about that folder in its configuration file, <code>Documents/Tapir/config.json</code> in your user folder (Tapir writes it the first time it runs). Add the folder to its <code>localFolders</code> list, keeping the rest of the file as it is:':
        '使いたいスクリプトをフォルダーにダウンロードし、そのフォルダーを Tapir の設定ファイルに書き込みます。設定ファイルはユーザーフォルダー内の <code>Documents/Tapir/config.json</code> です（Tapir の初回起動時に作られます）。ファイルの他の部分はそのままにして、<code>localFolders</code> の一覧にフォルダーを追加します。',
    '<code>path</code> is the folder (on macOS for example <code>/Users/me/Documents/Tapir scripts</code>). In JSON, a Windows path\'s backslashes are doubled.':
        '<code>path</code> はフォルダーです（macOS なら例えば <code>/Users/me/Documents/Tapir scripts</code>）。JSON では、Windows のパスのバックスラッシュを二重にします。',
    '<code>displayName</code> is the name the palette shows for the folder\'s scripts; without it, the folder\'s name is used.':
        '<code>displayName</code> は、パレットがそのフォルダーのスクリプトに表示する名前です。省略するとフォルダー名が使われます。',
    'Optionally, <code>includePattern</code> and <code>excludePattern</code> are regular expressions that choose which files of the folder the palette lists.':
        '必要に応じて、正規表現の <code>includePattern</code> と <code>excludePattern</code> で、フォルダー内のどのファイルをパレットに表示するかを選べます。',
    'The palette lists the folder\'s scripts next to the built-in ones the next time it loads its scripts.':
        '次にパレットがスクリプトを読み込むとき、そのフォルダーのスクリプトが組み込みスクリプトと並んで表示されます。',

    // 4. Python
    'Write a script of your own': '自分のスクリプトを書く',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (with several Archicads open, the next ones use 19724, 19725 and so on). Tapir\'s commands are called through Archicad\'s <code>API.ExecuteAddOnCommand</code> command, in the <code>TapirCommand</code> namespace. Python\'s standard library is enough; there are no packages to install.':
        'プロジェクトを開いている間、Archicad は <code>http://127.0.0.1:19723</code> で JSON コマンドに応答します（複数の Archicad を開いている場合、2 つ目以降は 19724、19725 と順に使います）。Tapir のコマンドは Archicad の <code>API.ExecuteAddOnCommand</code> コマンドを通じて、<code>TapirCommand</code> 名前空間で呼び出します。Python の標準ライブラリだけで十分で、インストールするパッケージはありません。',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a> sends the same commands and finds the port by itself, if you prefer it. Put your script in a <a href="#marketplace">local folder</a> to run it from the Tapir palette.':
        'お好みで、Graphisoft の <a href="https://pypi.org/project/archicad/"><code>archicad</code> パッケージ</a>も使えます。同じコマンドを送り、ポートも自動で見つけます。スクリプトを<a href="#marketplace">ローカルフォルダー</a>に置けば、Tapir パレットから実行できます。',
    '<a href="../archicad-addon/">All commands, with schemas and examples</a>':
        '<a href="../archicad-addon/">すべてのコマンド（スキーマと例付き）</a>',

    // 5. Grasshopper
    'Use it from Grasshopper': 'Grasshopper から使う',
    'In Rhino, open the Package Manager (the <code>PackageManager</code> command, or <em>Tools &gt; Package Manager</em>), search for <span class="mono">tapir</span>, install it and restart Rhino. Its components are on the <em>Tapir</em> tab in Grasshopper and work with the Archicad that runs Tapir. The Walls, Doors, Windows and Railings components read and change Archicad elements with the names you know from Archicad\'s settings dialogs.':
        'Rhino で Package Manager（<code>PackageManager</code> コマンド、または <em>Tools &gt; Package Manager</em>）を開き、<span class="mono">tapir</span> を検索してインストールし、Rhino を再起動します。コンポーネントは Grasshopper の <em>Tapir</em> タブにあり、Tapir が動いている Archicad と連携します。Walls、Doors、Windows、Railings の各コンポーネントは、Archicad の設定ダイアログでおなじみの名前で Archicad の要素を読み取り、変更します。',
    '<a href="../grasshopper/">The Grasshopper guide</a>': '<a href="../grasshopper/">Grasshopper ガイド</a>',

    // Next step
    'Next step': '次のステップ',
    'Find a script': 'スクリプトを探す',
    'Scripts shared by architects, checked and adapted for any office.':
        '建築家が共有したスクリプトを、チェックしてどの事務所でも使えるようにしたものです。',
    'Browse scripts': 'スクリプトを見る',
    'The command reference': 'コマンドリファレンス',
    'Every Tapir command with its parameters, its response and a Python example.':
        'すべての Tapir コマンドを、パラメーター、レスポンス、Python の例とともに。',
    'Open the reference': 'リファレンスを開く',
    'Stuck?': '困ったら',
    'Ask on Discord': 'Discord で質問',
    'Architects, developers and the maintainers answer there. Or open an issue on GitHub.':
        '建築家、開発者、メンテナーが答えてくれます。GitHub で issue を作成することもできます。',
    'Join Discord': 'Discord に参加'
});
