/* 日本語: the Tapir and AI page (ai/index.html). See ../README.md.
   The two prompts are translated too (an AI tool understands them in any language), the Python
   example stays as it is. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Control Archicad with AI assistants (MCP) | Tapir': 'AI アシスタント（MCP）で Archicad を操作 | Tapir',
    'Control Archicad with Claude and other AI assistants through Tapir: the Tapir MCP servers, a prompt to start with, and how to let an AI write a script.':
        'Tapir を通じて Claude などの AI アシスタントで Archicad を操作する方法。Tapir 用の MCP サーバー、最初に試すプロンプト、スクリプトを AI に書かせる方法を紹介します。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / AI': '<a href="../">Tapir</a> / <a href="../docs/">ドキュメント</a> / AI',
    'Tapir and <em>AI.</em>': 'Tapir と <em>AI。</em>',
    'An AI assistant can read your Archicad project and change it through Tapir. You ask in plain words; it sends the commands to the Archicad running on your computer.':
        'AI アシスタントは Tapir を通じて Archicad プロジェクトを読み取り、変更できます。普段の言葉で頼むだけで、アシスタントがお使いのコンピュータで動いている Archicad にコマンドを送ります。',

    // Two ways
    'Two ways to <em>connect.</em>': '<em>つなぎ方は二通り。</em>',
    'Both talk to the same Tapir commands on port 19723, on your own computer.':
        'どちらも、お使いのコンピュータ上のポート 19723 で同じ Tapir コマンドを使います。',
    'While you chat': 'チャットしながら',
    'An MCP server': 'MCP サーバー',
    'The AI tool calls Tapir\'s commands itself, while you chat. Works with tools that support the Model Context Protocol (MCP), such as Claude Desktop.':
        'チャットの間に、AI ツールが自分で Tapir のコマンドを呼び出します。Claude Desktop など、Model Context Protocol (MCP) に対応したツールで使えます。',
    'Tapir MCP servers': 'Tapir 用 MCP サーバー',
    'Any AI tool': 'どの AI ツールでも',
    'A script the AI writes': 'AI が書くスクリプト',
    'Any AI tool can write a short Python script that sends Tapir commands. You read it, then run it from the Tapir palette or a terminal.':
        'どの AI ツールでも、Tapir コマンドを送る短い Python スクリプトを書けます。内容を読んでから、Tapir パレットかターミナルで実行します。',
    'How it works': '仕組み',

    // MCP servers
    'MCP servers': 'MCP サーバー',
    'Tapir MCP <em>servers.</em>': 'Tapir 用 MCP <em>サーバー。</em>',
    'These are community projects, not part of Tapir. Each has its own install steps in its README. Which one Tapir recommends is still being decided in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, so they are listed here without a recommendation.':
        'これらはコミュニティのプロジェクトで、Tapir の一部ではありません。インストール手順はそれぞれの README にあります。Tapir がどれを推奨するかは <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a> でまだ検討中のため、ここでは推奨なしで紹介しています。',
    'By SzamosiMate': 'SzamosiMate 作',
    'The MCP server for Tapir that the others started from. It is also published on PyPI as the <span class="mono">tapir-archicad-mcp</span> package.':
        'ほかのサーバーの出発点になった Tapir 用 MCP サーバーです。PyPI でも <span class="mono">tapir-archicad-mcp</span> パッケージとして公開されています。',
    'Its repository': 'リポジトリ',
    'The package on PyPI': 'PyPI のパッケージ',
    'By the community': 'コミュニティ作',
    'Its forks': 'そのフォーク',
    'Versions of it that others changed and published. Check when each was last updated.':
        'ほかの人が変更して公開したバージョンです。それぞれの最終更新日を確認してください。',
    'See the forks': 'フォークを見る',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Careful</strong> An MCP server can change your model. Try it on a copy of your project first, and read what the AI is about to do before you allow it.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> 注意</strong> MCP サーバーはモデルを変更できます。まずプロジェクトのコピーで試し、許可する前に AI が何をしようとしているかを読んでください。',

    // A prompt to start with
    'A prompt to start with': '最初に試すプロンプト',
    'Start by <em>reading.</em>': 'まずは<em>読み取りから。</em>',
    'Open a project in Archicad, with the Tapir Add-On loaded, and give your AI tool this prompt. It only reads, so it is safe to try on any project.':
        'Tapir アドオンを読み込んだ Archicad でプロジェクトを開き、AI ツールにこのプロンプトを渡します。読み取るだけなので、どのプロジェクトでも安心して試せます。',
    'When the answer looks right, ask for a change in the same plain words, and say exactly which elements it may change: "Set the property X to Y on the selected walls only."':
        '答えが正しそうなら、同じように普段の言葉で変更を頼み、変更してよい要素を正確に伝えます。例：「選択した壁だけ、プロパティ X を Y に設定して。」',
    'prompt': 'プロンプト',
    'read only': '読み取りのみ',
    'Use Tapir to look at the Archicad project I have open. Do not change anything; only read. 1. List the stories with their names and levels. 2. Count the walls, slabs, columns, doors and windows on each story. 3. Show the result as one table, one row per story. Tell me which Tapir commands you used.':
        'Tapir を使って、私が開いている Archicad プロジェクトを調べてください。\n何も変更せず、読み取るだけにしてください。\n1. フロアを名前とレベル付きで一覧にしてください。\n2. 各フロアの壁、スラブ、柱、ドア、窓の数を数えてください。\n3. 結果をフロアごとに一行の表一つにまとめてください。\n使った Tapir コマンドも教えてください。',

    // Check the names first
    'Check the names first': 'まず名前を確認する',
    'Check names, <em>don\'t guess.</em>': '名前は<em>推測せず、確認する。</em>',
    'Your words rarely match the names in the project: "fire rating" may be a property called <span class="mono">FireRating</span> in one template and <span class="mono">Feuerwiderstand</span> in another, and "level 2" may be the story "02 First Floor". An AI that guesses the name still gets an answer, only a wrong one: no values found, so "no doors have a fire rating".':
        'ふだんの言葉とプロジェクト内の名前が一致することはまれです。「耐火等級」は、あるテンプレートでは <span class="mono">FireRating</span>、別のテンプレートでは <span class="mono">Feuerwiderstand</span> という名前のプロパティかもしれませんし、「2 階」は「02 First Floor」というフロアかもしれません。名前を推測する AI も答えは返しますが、それは間違った答えです。値が見つからないため、「耐火等級を持つドアはありません」となってしまいます。',
    '<b aria-hidden="true">1</b><strong>Look the words up first</strong><span>Before a command whose parameters come from your words (a story, property, classification, layer, building material, composite, surface, zone category or element type), the AI sends those words to <a href="../archicad-addon/#ResolveNames">ResolveNames</a>, with the kind of each when it knows it. It only reads.</span>':
        '<b aria-hidden="true">1</b><strong>まず言葉を調べる</strong><span>パラメータがユーザーの言葉に基づくコマンド（フロア、プロパティ、分類、レイヤー、ビルディングマテリアル、複合構造、表面材、ゾーンカテゴリ、要素タイプ）の前に、AI はその言葉を <a href="../archicad-addon/#ResolveNames">ResolveNames</a> に送ります。種類がわかっていれば、それぞれの種類も一緒に送ります。このコマンドは読み取るだけです。</span>',
    '<b aria-hidden="true">2</b><strong>Use what it returns</strong><span><span class="mono">found</span>: the item named exactly so, with the ids the other commands take (story index, attribute id, elementType, propertyId, classification ids). <span class="mono">closeMatches</span>: only similar names, best first; the AI asks you which one you meant instead of picking one itself.</span>':
        '<b aria-hidden="true">2</b><strong>返された結果を使う</strong><span><span class="mono">found</span>：名前が完全に一致する項目と、ほかのコマンドが受け取る ID（フロアのインデックス、属性 ID、elementType、propertyId、分類の ID）。<span class="mono">closeMatches</span>：似た名前だけで、近いものから順に並びます。AI は自分で選ばず、どれのことかをユーザーに尋ねます。</span>',
    '<b aria-hidden="true">3</b><strong>Say "not in this project"</strong><span><span class="mono">notFound</span>: nothing in the project is close, so the project has no such item. The AI says "this project has no X" instead of guessing a name or reporting an empty result as the answer.</span>':
        '<b aria-hidden="true">3</b><strong>「このプロジェクトにはない」と伝える</strong><span><span class="mono">notFound</span>：プロジェクト内に近いものがなく、そのような項目はプロジェクトにありません。AI は名前を推測したり、空の結果をそのまま答えにしたりせず、「このプロジェクトには X はありません」と伝えます。</span>',
    'Add these lines to your prompt, or to the instructions of your MCP server, so the AI checks the names every time. They work the same when the AI writes a script: the script sends <span class="mono">ResolveNames</span> first and stops with a message on <span class="mono">notFound</span>.':
        'この数行をプロンプトや MCP サーバーの指示に加えておけば、AI は毎回名前を確認します。AI がスクリプトを書くときも同じです。スクリプトはまず <span class="mono">ResolveNames</span> を送り、<span class="mono">notFound</span> ならメッセージを出して止まります。',
    'Before a change, ask for a preview. The commands that delete or replace, such as <a href="../archicad-addon/#DeleteElements">DeleteElements</a>, <a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a> and <a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a>, take <span class="mono">"dryRun": true</span>: nothing changes, and the response lists what would. The command reference says which commands have it.':
        '変更の前には、プレビューを求めましょう。<a href="../archicad-addon/#DeleteElements">DeleteElements</a>、<a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a>、<a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a> など、削除や置き換えを行うコマンドは <span class="mono">"dryRun": true</span> を受け付けます。何も変更されず、変更される内容が応答に一覧で返されます。どのコマンドで使えるかはコマンドリファレンスに書かれています。',
    'add to any request': 'どの依頼にも追加',
    'Before you use a name taken from my words (a story, property, classification, layer, building material, composite, surface, zone category or element type), check it with the Tapir command ResolveNames. - found: use the ids it returns. - closeMatches: ask me which one I mean. - notFound: tell me this project has no such item. Never guess a name. Before a command that deletes or replaces, run it with "dryRun": true if it has that input, and show me what it would change.':
        '私の言葉から取った名前（フロア、プロパティ、分類、レイヤー、\nビルディングマテリアル、複合構造、表面材、ゾーンカテゴリ、\n要素タイプ）を使う前に、Tapir コマンド ResolveNames で\n確認してください。\n- found：返された ID を使ってください。\n- closeMatches：どれのことか私に尋ねてください。\n- notFound：このプロジェクトにはそれがないと伝えてください。\n  名前を推測しないでください。\n削除や置き換えを行うコマンドは、"dryRun": true の入力が\nあればまずそれを付けて実行し、何が変わるかを見せてください。',

    // When the AI writes a script
    'When the AI writes a script': 'AI がスクリプトを書く場合',
    'Describe it, <em>run it.</em>': '説明して、<em>実行する。</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'MCP サーバーがなければ、作業を説明し、AI にコマンド一覧を渡して、AI が書いたスクリプトを実行します。',
    '<b aria-hidden="true">1</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b aria-hidden="true">1</b><strong>したいことを説明する</strong><span>例：「2 階の複合構造の外壁をすべて抽出し、その正味体積を計算して。」</span>',
    '<b aria-hidden="true">2</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b aria-hidden="true">2</b><strong>コマンド一覧を渡す</strong><span>AI に<a href="../archicad-addon/">コマンドリファレンス</a>と下の動作する例を渡します。Python の標準ライブラリだけを使い、ポート 19723 を使うよう頼みます。</span>',
    '<b aria-hidden="true">3</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b aria-hidden="true">3</b><strong>読んでから実行する</strong><span>Tapir パレットやターミナルで実行する前に、スクリプトを読みます。モデルを変更するスクリプトは、まずプロジェクトのコピーで試してください。</span>',
    'for Claude or any AI tool': 'Claude などの AI ツール向け',
    'Write a Python script that uses only the standard library and sends Tapir commands to Archicad on http://127.0.0.1:19723, like the example I attach. Use the Tapir command reference I attach. Extract all composite exterior walls on level 2 and calculate their net volume. Only read the model; change nothing.':
        '標準ライブラリだけを使う Python スクリプトを書いてください。\n添付の例のように、http://127.0.0.1:19723 の\nArchicad に Tapir コマンドを送るものです。\n添付の Tapir コマンドリファレンスを使ってください。\n\n2 階の複合構造の外壁をすべて抽出し、\nその正味体積を計算してください。\n\nモデルは読み取るだけで、何も変更しないでください。',

    // Next step
    'Next step': '次のステップ',
    'Install Tapir': 'Tapir をインストールする',
    'Install Tapir, then give your AI tool the prompt above.': 'Tapir をインストールして、上のプロンプトを AI ツールに渡しましょう。',
    'Get started': 'はじめる',
    'The command reference': 'コマンドリファレンス',
    'The list to give the AI: every command with its schema.': 'AI に渡す一覧。すべてのコマンドとそのスキーマ。',
    'Open the reference': 'リファレンスを開く',
    'Show what you built': '作ったものを見せる',
    'Ask about MCP servers and share your prompts.': 'MCP サーバーについて質問したり、プロンプトを共有したりしましょう。',
    'Join Discord': 'Discord に参加する'
});
