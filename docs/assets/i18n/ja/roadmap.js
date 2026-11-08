/* 日本語: the Roadmap page (roadmap/index.html). See ../README.md.
   Command names, the label enhancement and the names of the issue form and of the old list stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Tapir roadmap – Archicad automation': 'Tapir ロードマップ – Archicad 自動化',
    'How to ask for a feature in Tapir, the Archicad add-on, what people are asking for now, what shipped from the old list and which ideas are still open.':
        'Archicad アドオン Tapir に機能をリクエストする方法、いまリクエストされているもの、旧リストからリリースされたもの、まだ実現していないアイデア。',
    '<a href="../">Tapir</a> / <a href="../community/">Community</a> / Roadmap':
        '<a href="../">Tapir</a> / <a href="../community/">コミュニティ</a> / ロードマップ',
    'What\'s <em>next.</em>': 'これから<em>来るもの。</em>',
    'Tapir grows from requests. Most new commands, components and fixes start as a message from someone who needed them in their own work.':
        'Tapir はリクエストから育ちます。新しいコマンド、コンポーネント、修正のほとんどは、自分の仕事でそれを必要とした誰かのメッセージから始まります。',

    // How to ask for a feature
    'How to ask for a <em>feature.</em>': '<em>機能</em>をリクエストするには',
    'Four steps from "I need this" to a Monday release.': '「これが必要」から月曜日のリリースまで、4 つのステップ。',
    'Say what you want to do': 'やりたいことを伝える',
    'Post it on Discord or open an issue on GitHub. Describe the task in Archicad, not only the command: "number the doors along a line", "read the area of every zone". A Discord message that reads like a feature request becomes a GitHub issue by itself, labelled <code>enhancement</code>.':
        'Discord に投稿するか、GitHub で issue を作成してください。コマンドだけでなく、Archicad での作業を説明してください。たとえば「線に沿ってドアに番号を付ける」「すべてのゾーンの面積を読み取る」。機能リクエストらしい Discord のメッセージは、自動的に <code>enhancement</code> ラベル付きの GitHub issue になります。',
    'Asking for a part of the Archicad API?': 'Archicad API の一部が必要ですか？',
    'The <a href="../api-coverage/">API coverage</a> page lists every function of the Archicad API and whether Tapir has a command for it. Use the <em>Archicad API request</em> issue form to ask for one that is missing.':
        '<a href="../api-coverage/">API カバレッジ</a>のページには、Archicad API のすべての関数と、Tapir にそのコマンドがあるかどうかが載っています。足りないものは <em>Archicad API request</em> の issue フォームでリクエストしてください。',
    'The issue bot picks it up': 'issue ボットが引き受ける',
    'A clear, wanted request is built like a fix: the bot writes it and opens a pull request, and you are asked to try it. A large one is split into parts. When the design is open, the bot describes the choices and a maintainer decides.':
        '明確で求められているリクエストは、修正と同じように作られます。ボットが実装して pull request を作成し、あなたに試してもらいます。大きなリクエストはいくつかの部分に分けられます。設計が決まっていない場合は、ボットが選択肢を説明し、メンテナーが決めます。',
    'It ships on a Monday': '月曜日にリリース',
    'Merged changes are released every Monday. The release notes list them, and the add-on offers to update itself.':
        'マージされた変更は毎週月曜日にリリースされます。リリースノートに一覧が載り、アドオンは自分自身の更新を提案します。',

    // What is being asked for now
    'What is being asked for now': 'いまリクエストされているもの',
    'Requests': 'リクエスト',
    'Open feature requests': '未解決の機能リクエスト',
    'Every open issue labelled <code>enhancement</code>. Add a comment or a reaction to one you need too.':
        '<code>enhancement</code> ラベルの付いた未解決の issue すべて。あなたも必要なものには、コメントかリアクションを付けてください。',
    'See the requests': 'リクエストを見る',
    'Plan': '計画',
    'Project board': 'プロジェクトボード',
    'The maintainers\' board of planned and ongoing work.': '予定中・進行中の作業をまとめたメンテナーのボード。',
    'Open the board': 'ボードを開く',
    'In progress': '進行中',
    'Open pull requests': 'オープンな pull request',
    'Changes being built, reviewed and tested right now.': 'いま開発、レビュー、テストされている変更。',
    'See the pull requests': 'pull request を見る',
    'Shipped': 'リリース済み',
    'Releases': 'リリース',
    'Every release, with its notes and downloads. The <span class="nowrap">command reference</span> shows the version each command came in.':
        'すべてのリリースと、そのノートとダウンロード。<span class="nowrap">コマンドリファレンス</span>には、各コマンドが追加されたバージョンが載っています。',
    'See the releases': 'リリースを見る',

    // Recently shipped
    'Recently shipped': '最近のリリース',
    'Already <em>here.</em>': 'もう<em>あります。</em>',
    'Asked for on the old site\'s "What\'s cooking" list, and now part of Tapir.':
        '旧サイトの「What\'s cooking」リストでリクエストされ、いまは Tapir に入っているもの。',
    'Stairs, roofs and curtain walls': '階段、屋根、カーテンウォール',
    'Read and change them: <a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>, <a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a> and <a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>.':
        '読み取りと変更：<a href="../archicad-addon/#ModifyStairs"><code>ModifyStairs</code></a>、<a href="../archicad-addon/#ModifyRoofs"><code>ModifyRoofs</code></a>、<a href="../archicad-addon/#ModifyCurtainWalls"><code>ModifyCurtainWalls</code></a>。',
    'Building materials and composites': 'ビルディングマテリアルと複合構造',
    'Create them from a script: <a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a> and <a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>.':
        'スクリプトから作成：<a href="../archicad-addon/#CreateBuildingMaterials"><code>CreateBuildingMaterials</code></a>、<a href="../archicad-addon/#CreateComposites"><code>CreateComposites</code></a>。',
    'Dialogs from Python': 'Python からダイアログ',
    'A script shows its own window in Archicad: <a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a> and <a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>.':
        'スクリプトが Archicad に独自のウィンドウを表示します：<a href="../archicad-addon/#ShowScriptUI"><code>ShowScriptUI</code></a>、<a href="../archicad-addon/#GetScriptUIResult"><code>GetScriptUIResult</code></a>。',
    '2D detailing': '2D 詳細図',
    'Lines, fills and text: <a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>, <a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a> and <a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>.':
        '線、塗りつぶし、テキスト：<a href="../archicad-addon/#CreateLineElements"><code>CreateLineElements</code></a>、<a href="../archicad-addon/#CreateHatches"><code>CreateHatches</code></a>、<a href="../archicad-addon/#CreateTexts"><code>CreateTexts</code></a>。',
    'Your own properties': '独自のプロパティ',
    'Add property definitions from a script: <a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>.':
        'スクリプトからプロパティ定義を追加：<a href="../archicad-addon/#CreatePropertyDefinitions"><code>CreatePropertyDefinitions</code></a>。',
    'Built-in scripts': '組み込みスクリプト',
    'Ready to run from the palette, from numbering to a project health check. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">See them on GitHub</a>.':
        'パレットからすぐに実行できます。番号付けからプロジェクトの健全性チェックまで。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts">GitHub で見る</a>。',

    // Ideas
    'Ideas': 'アイデア',
    'Still <em>open.</em>': 'まだ<em>未着手。</em>',
    'Two ideas from the old list that nobody has asked for in an issue yet. Want one? Open the issue.':
        '旧リストにあった、まだ誰も issue でリクエストしていない 2 つのアイデア。欲しいものがあれば、issue を作成してください。',
    'Idea': 'アイデア',
    'Clearer error messages in Python': 'Python でのわかりやすいエラーメッセージ',
    'Tracebacks and command errors that say what went wrong and how to fix it, so a script is easier to troubleshoot.':
        '何が起きたのか、どう直せばよいのかを伝えるトレースバックとコマンドエラー。スクリプトのトラブルシューティングが楽になります。',
    'Ask for it': 'リクエストする',
    'Tapir scripts in CI': 'CI での Tapir スクリプト',
    'Running Tapir scripts automatically, for example in a GitHub Actions pipeline.':
        'Tapir スクリプトを自動で実行すること。たとえば GitHub Actions のパイプラインで。',

    // Next step
    'Next step': '次のステップ',
    'Missing something in your work?': '仕事で足りないものはありますか？',
    'Tell us what you want to automate': '自動化したいことを教えてください',
    'Open an issue, or describe it on Discord and the bot files it for you.':
        'issue を作成するか、Discord で説明してください。ボットが代わりに issue を作成します。',
    'Request a feature on GitHub': 'GitHub で機能をリクエスト',
    'Ask on Discord': 'Discord で質問'
});
