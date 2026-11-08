/* 日本語: the Share page (scripts/share/index.html). See ../README.md.
   Channel, label, file and code names, and the palette's Share command, stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Share an Archicad script | Tapir': 'Archicad スクリプトを共有 | Tapir',
    'Share an Archicad script you wrote: send it on Discord or as a pull request, and it is checked, adapted to work in any office and listed with your name.':
        '自分で書いた Archicad スクリプトを共有しましょう。Discord かプルリクエストで送ると、確認され、どの事務所でも動くように調整され、あなたの名前で掲載されます。',
    '<a href="../../">Tapir</a> / <a href="../">Scripts</a> / Share': '<a href="../../">Tapir</a> / <a href="../">スクリプト</a> / 共有',
    'Share a script <em>from Archicad.</em>': 'Archicad から<em>スクリプトを共有。</em>',
    'No GitHub account, no setup. Your name stays on it, and everyone who uses Tapir can install it.':
        'GitHub アカウントも設定も不要です。あなたの名前が残り、Tapir を使う誰もがインストールできます。',

    // What happens
    'What happens': '流れ',
    '<b aria-hidden="true">1</b><strong>Send it</strong><span>Post the script in <code>#share-a-script</code> on Discord, with a few words on what it does. The Tapir palette is getting a Share button that does the same: you see the whole file before anything is sent. It arrives as a GitHub issue labelled <code>script-share</code>.</span>':
        '<b aria-hidden="true">1</b><strong>送る</strong><span>Discord の <code>#share-a-script</code> に、何をするかを一言添えてスクリプトを投稿します。Tapir パレットにも同じことをする Share ボタンが加わる予定で、送信前にファイル全体を確認できます。投稿は <code>script-share</code> ラベル付きの GitHub issue になります。</span>',
    '<b aria-hidden="true">2</b><strong>Adapted for any office</strong><span>Tapir\'s bot, an AI, opens a pull request: hard-coded layers, properties and paths become settings, risky code is removed, and it works in every Archicad language, in metric and imperial.</span>':
        '<b aria-hidden="true">2</b><strong>どの事務所でも使えるように</strong><span>Tapir のボット（AI）がプルリクエストを作成します。直接書き込まれたレイヤー、プロパティ、パスは設定になり、危険なコードは取り除かれ、どの言語版の Archicad でも、メートル法でもヤード・ポンド法でも動くようになります。</span>',
    '<b aria-hidden="true">3</b><strong>You confirm, it\'s listed</strong><span>Try the adapted version and say it still does what you meant. Once the pull request is reviewed and merged, it appears on the <a href="../">Scripts</a> page with your name.</span>':
        '<b aria-hidden="true">3</b><strong>確認したら掲載</strong><span>調整されたバージョンを試し、意図どおりに動くかを伝えてください。プルリクエストがレビューされマージされると、あなたの名前で<a href="../">スクリプト</a>ページに掲載されます。</span>',

    // Three ways
    'Three ways to send it': '3 つの送り方',
    'Coming to the palette': 'パレットに近日搭載',
    'From the Tapir palette': 'Tapir パレットから',
    'Right-click your script and choose Share: add a few words about it and send. Until your palette has it, use Discord.':
        'スクリプトを右クリックして Share を選び、一言添えて送信します。パレットにこの機能が入るまでは Discord をお使いください。',
    'Easiest today': '今いちばん簡単',
    'On Discord': 'Discord で',
    'Post the script in the <code>#share-a-script</code> channel, with a few words on what it does and which Archicad version you use.':
        '<code>#share-a-script</code> チャンネルに、何をするか、どの Archicad バージョンを使っているかを一言添えてスクリプトを投稿します。',
    'Join Discord': 'Discord に参加',
    'If you use GitHub': 'GitHub を使うなら',
    'As a pull request': 'プルリクエストで',
    'Add a folder <code>marketplace/&lt;name&gt;/</code> with the script, a <code>listing.json</code> and a README, as the marketplace README describes.':
        'マーケットプレイスの README に従って、スクリプト、<code>listing.json</code>、README を入れた <code>marketplace/&lt;name&gt;/</code> フォルダを追加します。',
    'The marketplace folder': 'マーケットプレイスのフォルダ',

    // The small print
    'The small print': '注意事項',
    'What is sent': '送られるもの',
    'Only the script': 'スクリプトだけ',
    'The file, your one-line description, the name to credit, and your Archicad and Tapir versions. Nothing from the open project.':
        'ファイル、一行の説明、クレジットする名前、Archicad と Tapir のバージョンです。開いているプロジェクトの内容は送られません。',
    'Free for everyone': '誰でも無料',
    'Every shared script is free and open source. Unless you choose another open-source licence, yours is published under the <a class="inline" href="https://opensource.org/license/mit">MIT licence</a>: anyone may use, change and share it, as long as they keep your name and the licence with it. It comes without warranty. You stay its author, and its page credits you.':
        '共有されたスクリプトはすべて無料のオープンソースです。別のオープンソースライセンスを選ばない限り、あなたのスクリプトは <a class="inline" href="https://opensource.org/license/mit">MIT ライセンス</a>で公開されます。あなたの名前とライセンスを残す限り、誰でも使用、変更、共有できます。保証はありません。作者はあなたのままで、スクリプトのページにあなたの名前が載ります。',
    'Only share a script you wrote, or one whose author agreed to share it.':
        '共有するのは、自分で書いたスクリプトか、作者が共有に同意したスクリプトだけにしてください。',

    // What the AI changes
    'What the AI changes': 'AI が変えること',
    'What the AI <em>changes.</em>': 'AI が<em>変えること。</em>',
    'It keeps what your script does. It changes how, so the script is safe and works in other offices, and lists every change in plain words in the pull request.':
        'スクリプトがすることはそのままに、やり方を変えて、安全で他の事務所でも動くようにします。変更点はすべて、プルリクエストにわかりやすい言葉で列挙します。',
    'Made safe': '安全に',
    'Removed, or flagged for a maintainer': '削除、またはメンテナーに報告',
    'connections to anything other than Archicad on your own computer;':
        '自分のコンピューター上の Archicad 以外への接続',
    'running other programs or code built from text (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>);':
        '他のプログラムや、テキストから組み立てたコードの実行（<code>subprocess</code>、<code>os.system</code>、<code>eval</code>、<code>exec</code>）',
    'installing packages while it runs (<code>pip install</code>);':
        '実行中のパッケージのインストール（<code>pip install</code>）',
    'deleting or overwriting files, and writing anywhere but the project\'s folder and <code>Documents/Tapir</code>;':
        'ファイルの削除や上書き、プロジェクトのフォルダと <code>Documents/Tapir</code> 以外への書き込み',
    'passwords, tokens and keys;': 'パスワード、トークン、キー',
    'user names, <code>C:\\Users\\…</code> paths, and project, client and office names, in the code and its comments.':
        'コードとコメント中のユーザー名、<code>C:\\Users\\…</code> のパス、プロジェクト名、クライアント名、事務所名',
    'Made to fit other offices': '他の事務所に合わせて',
    'Settings instead of your names': '固有の名前の代わりに設定を',
    'Property names, layers, stories, paths and classification codes you typed in become settings at the top of the script, or fields of its window.':
        '直接入力されたプロパティ名、レイヤー、フロア、パス、分類コードは、スクリプト冒頭の設定か、ウィンドウの入力欄になります。',
    'Built-in properties are found by their ID, so the script works in every language of Archicad.':
        '組み込みのプロパティは ID で探すので、どの言語版の Archicad でも動きます。',
    'Lengths and areas work in metric and imperial units; paths work on Windows and macOS.':
        '長さと面積はメートル法でもヤード・ポンド法でも扱え、パスは Windows でも macOS でも動きます。',
    'Made safe to run': '安心して実行',
    'No surprises': '想定外の動きなし',
    'It says what it is about to change, and does nothing, saying so, when nothing fits.':
        'これから何を変更するかを伝え、該当するものがなければ、そう伝えて何もしません。',
    'It makes its changes in one Undo step where it can.':
        '可能な限り、変更を 1 回の取り消しで戻せるようにします。',
    'When a property or element type it needs is missing, it shows a clear message instead of crashing.':
        '必要なプロパティや要素タイプがないときは、異常終了せずにわかりやすいメッセージを表示します。',
    'Described': '説明付き',
    'A README in plain words': 'わかりやすい README',
    'What it does, how to use it, its settings, what it changes, its limits, and <em>Original by</em> you.':
        '何をするか、使い方、設定、変更する内容、制限、そしてあなたの名前を記した <em>Original by</em>。',
    'Its category, the Archicad and Tapir versions it works with, and whether it changes the model, writes files or only reads.':
        'カテゴリ、対応する Archicad と Tapir のバージョン、そしてモデルを変更するのか、ファイルを書き出すのか、読み取りのみなのか。',

    // Review
    'Review': 'レビュー',
    'Before it is merged': 'マージの前に',
    'What the review checks': 'レビューで確認すること',
    'Every pull request is reviewed before it is merged, like any other code in Tapir. The review checks that:':
        'Tapir の他のコードと同じく、すべてのプルリクエストはマージ前にレビューされます。レビューでは次のことを確認します。',
    'nothing on the <em>Made safe</em> list is left in, unless a maintainer agreed to it;':
        '<em>安全に</em>の一覧にあるものが、メンテナーの同意なしに残っていないこと',
    'the script still does what you wrote it for, and you confirmed that;':
        'スクリプトが書かれた目的どおりに動き、あなたがそれを確認したこと',
    'the folder has the files and fields the <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">marketplace format</a> asks for;':
        'フォルダに<a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">マーケットプレイスの形式</a>が求めるファイルと項目があること',
    'the README is accurate, and the screenshots show no client, project or people.':
        'README が正確で、スクリーンショットにクライアント、プロジェクト、人物が写っていないこと',
    'Next step': '次のステップ',
    'Post your script': 'スクリプトを投稿',
    'Share it in <code>#share-a-script</code> on Discord, and we take it from there.':
        'Discord の <code>#share-a-script</code> で共有してください。あとはこちらで進めます。',
    'Browse scripts': 'スクリプトを見る'
});
