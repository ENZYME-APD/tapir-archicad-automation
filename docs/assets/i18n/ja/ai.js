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

    // When the AI writes a script
    'When the AI writes a script': 'AI がスクリプトを書く場合',
    'Describe it, <em>run it.</em>': '説明して、<em>実行する。</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'MCP サーバーがなければ、作業を説明し、AI にコマンド一覧を渡して、AI が書いたスクリプトを実行します。',
    '<b>01</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b>01</b><strong>したいことを説明する</strong><span>例：「2 階の複合構造の外壁をすべて抽出し、その正味体積を計算して。」</span>',
    '<b>02</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b>02</b><strong>コマンド一覧を渡す</strong><span>AI に<a href="../archicad-addon/">コマンドリファレンス</a>と下の動作する例を渡します。Python の標準ライブラリだけを使い、ポート 19723 を使うよう頼みます。</span>',
    '<b>03</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b>03</b><strong>読んでから実行する</strong><span>Tapir パレットやターミナルで実行する前に、スクリプトを読みます。モデルを変更するスクリプトは、まずプロジェクトのコピーで試してください。</span>',
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
