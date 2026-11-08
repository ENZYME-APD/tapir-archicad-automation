/* 日本語: the Scripts page (scripts/index.html), with the texts scripts.js shows from it. See ../README.md.
   The scripts' own names, summaries and READMEs stay as their authors wrote them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Free Archicad Python scripts | Tapir': '無料の Archicad Python スクリプト | Tapir',
    'Free Python scripts for the Tapir palette in Archicad, shared by architects, each one reviewed and adapted to work in any office before it is listed.':
        '建築家が共有した、Archicad の Tapir パレット用の無料 Python スクリプト。掲載前にそれぞれ審査され、どの事務所でも動くように調整されています。',
    '<a href="../">Tapir</a> / Scripts': '<a href="../">Tapir</a> / スクリプト',
    'The script <em>schedule.</em>': 'スクリプト<em>一覧。</em>',
    'Scripts shared by architects. Each one is checked for safety and adapted to work in any office and any Archicad language before it is listed.':
        '建築家が共有したスクリプトです。掲載前にそれぞれ安全性を確認し、どの事務所でも、どの言語版の Archicad でも動くように調整しています。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Opening soon</strong> The scripts move to this page, and the list appears here as soon as the first ones are in. Until then, the scripts shared so far are on the <a href="https://tapir-marketplace.vercel.app/">current marketplace</a>, and you can <a href="share/">share your own</a>.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> まもなく公開</strong> スクリプトはこのページに移ります。最初のスクリプトが揃いしだい、ここに一覧が表示されます。それまでは、これまでに共有されたスクリプトは<a href="https://tapir-marketplace.vercel.app/">現在のマーケットプレイス</a>にあります。<a href="share/">自分のスクリプトを共有</a>することもできます。',

    // The built-in scripts, shown until the marketplace has listings
    'Built in to <em>Tapir.</em>': 'Tapir に<em>組み込み済み。</em>',
    'These are five of the scripts that come with the Tapir palette: open it in Archicad, pick one and press Run Script. Each is a short Python file you can read and change.':
        'Tapir パレットに付属するスクリプトのうちの 5 つです。Archicad でパレットを開き、1 つ選んで Run Script を押します。どれも読んで変更できる短い Python ファイルです。',
    'Built-in script': '組み込みスクリプト',
    'Number elements along a line': '線に沿って要素に番号を付ける',
    'Select a polyline and the doors, zones or objects to number. Each one gets its Element ID in the order the polyline passes it.':
        'ポリラインと、番号を付けるドア、ゾーン、オブジェクトを選択します。ポリラインが通過する順に、それぞれに Element ID が付きます。',
    'Colour the model by a property': 'プロパティでモデルを色分け',
    'Pick a property; every element gets a colour for its value, with a legend of the values and how many elements have each. Number values can be grouped into ranges.':
        'プロパティを選ぶと、各要素がその値に応じた色になり、値とそれぞれの要素数を示す凡例が付きます。数値は範囲ごとにまとめられます。',
    'Check a project before it goes out': '提出前にプロジェクトをチェック',
    'Lists elements without an ID, zones with the same number, unclassified and duplicate elements, empty stories and unused layers. Export the list to Excel or turn it into Issues.':
        'ID のない要素、同じ番号のゾーン、未分類や重複した要素、空のフロア、使われていないレイヤーを一覧にします。一覧は Excel に書き出すか、Issues にできます。',
    'Publish on a schedule': 'スケジュールで発行',
    'Publishes the chosen Publisher Sets now or on a schedule, every few hours or daily at a set time. It can receive the Teamwork changes and update the drawings first, and logs every run.':
        '選んだ Publisher Sets を今すぐ、または数時間ごとや毎日決まった時刻にスケジュールで発行します。先に Teamwork の変更を受信して図面を更新でき、毎回の実行を記録します。',
    'Tidy up the View Map': 'View Map を整理',
    'Finds the views that are neither placed on a layout nor published, and moves the ones you choose into a separate folder that keeps their folder structure.':
        'レイアウトに配置されておらず発行もされていないビューを見つけ、選んだものをフォルダ構成を保ったまま別のフォルダに移動します。',

    // The schedule
    'Browse scripts': 'スクリプトを見る',
    'Search scripts': 'スクリプトを検索',
    'Search: name, task or author': '検索：名前、用途、作者',
    'Category': 'カテゴリ',
    'All': 'すべて',
    'Archicad version': 'Archicad のバージョン',
    'Any Archicad': 'すべての Archicad',
    'Filters': '絞り込み',
    'Active filters': '適用中の絞り込み',
    'Remove filter: {name}': '絞り込みを解除: {name}',
    '1 script': '1 件のスクリプト',
    '{count} scripts': '{count} 件のスクリプト',
    'No script matches. Try fewer words or another filter.': '該当するスクリプトはありません。語句を減らすか、別の絞り込みをお試しください。',
    'Marketplace · sorted by category': 'マーケットプレイス · カテゴリ順',
    'Script': 'スクリプト',
    'Effect': '影響',
    'Author': '作者',
    'Changes the model': 'モデルを変更',
    'Writes files': 'ファイルを書き出し',
    'Read only': '読み取りのみ',
    'Not stated': '記載なし',

    // The categories
    'Quality assurance': '品質チェック',
    'Quantities and scheduling': '数量と一覧表',
    'Documentation and layouts': '図面とレイアウト',
    'Zones and areas': 'ゾーンと面積',
    'Element data and properties': '要素データとプロパティ',
    'Modeling and geometry': 'モデリングと形状',
    'Classifications and attributes': '分類と属性',
    'Navigator and views': 'ナビゲータとビュー',
    'Libraries and objects': 'ライブラリとオブジェクト',
    'Import and export': '読み込みと書き出し',
    'Batch automation': '一括処理',
    'Utilities': 'ユーティリティ',

    // Install and share
    'Install and share': 'インストールと共有',
    'Install': 'インストール',
    'Install a script': 'スクリプトをインストール',
    'Open a script here and follow its <em>Install in Tapir</em> steps. In short: download it into a folder and add that folder to the Tapir palette. The website is for browsing; scripts run in Archicad.':
        'ここでスクリプトを開き、<em>Tapir にインストール</em>の手順に従ってください。要するに、フォルダにダウンロードし、そのフォルダを Tapir パレットに追加します。このサイトは閲覧用で、スクリプトは Archicad で実行します。',
    'Step by step in Get started': '「はじめる」で手順を見る',
    'Wrote a script that saves you time? Share it, and we help you make it ready for other offices. No GitHub account needed.':
        '時間を節約できるスクリプトを書きましたか？共有していただければ、他の事務所でも使えるように手伝います。GitHub アカウントは不要です。',
    'How to share a script': 'スクリプトの共有方法',
    'Join Discord': 'Discord に参加',

    // Not found
    '<a href="../">Tapir</a> / <a href="./">Scripts</a> / Not found': '<a href="../">Tapir</a> / <a href="./">スクリプト</a> / 見つかりません',
    'Script not <em>found.</em>': 'スクリプトが<em>見つかりません。</em>',
    'There is no script with this name on the list, or the list could not be read just now.':
        'この名前のスクリプトは一覧にないか、今は一覧を読み込めませんでした。',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Not found</strong> Try again later, or see <a href="./">all scripts</a>.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> 見つかりません</strong> 後でもう一度お試しいただくか、<a href="./">すべてのスクリプト</a>をご覧ください。',

    // One script
    '<a href="../">Tapir</a> / <a href="./">Scripts</a> / <span id="detail-crumb"></span>': '<a href="../">Tapir</a> / <a href="./">スクリプト</a> / <span id="detail-crumb"></span>',
    'Screenshot {number} of {name}': '{name} のスクリーンショット {number}',
    'What it does': 'できること',
    'More screenshots': 'その他のスクリーンショット',
    'Versions': 'バージョン',
    'Code': 'コード',
    'About this script': 'このスクリプトについて',
    'Version': 'バージョン',
    '<span id="detail-tapir"></span> or later': '<span id="detail-tapir"></span> 以降',
    'Version <span class="version-number"></span>': 'バージョン <span class="version-number"></span>',
    'Archicad <span class="version-archicad"></span> · Tapir <span class="version-tapir"></span> or later':
        'Archicad <span class="version-archicad"></span> · Tapir <span class="version-tapir"></span> 以降',
    'Install in Tapir': 'Tapir にインストール',
    'Check that Archicad and Tapir are new enough: see <em>Archicad</em> and <em>Tapir</em> above.':
        'Archicad と Tapir が十分に新しいか確認してください。上の <em>Archicad</em> と <em>Tapir</em> を参照してください。',
    'When your Tapir palette lists scripts to install, find <strong class="detail-name-ref"></strong> there and install it.':
        'Tapir パレットにインストールできるスクリプトの一覧が表示されるようになったら、そこで <strong class="detail-name-ref"></strong> を探してインストールしてください。',
    'Until then, download the script into a folder and add that folder to the palette, as <a href="../start/#marketplace">Get started</a> shows.':
        'それまでは、<a href="../start/#marketplace">はじめる</a>にあるように、スクリプトをフォルダにダウンロードし、そのフォルダをパレットに追加してください。',
    'Open the project, select what the script asks for, and run it from the palette.':
        'プロジェクトを開き、スクリプトが求めるものを選択して、パレットから実行します。',
    'This script changes the model: read what it changes under <em>What it does</em> before you run it, and try it on a copy first.':
        'このスクリプトはモデルを変更します。実行する前に<em>できること</em>で変更内容を読み、まずコピーで試してください。',
    'This script writes files, for example a report or an export, and changes nothing in the model.':
        'このスクリプトはレポートや書き出しなどのファイルを書き出し、モデルは何も変更しません。',
    'This script only reads: it changes nothing in the model and writes no files.':
        'このスクリプトは読み取りのみです。モデルは何も変更せず、ファイルも書き出しません。',
    'Download the script': 'スクリプトをダウンロード',
    'View on GitHub': 'GitHub で見る',
    'Report a problem': '問題を報告',

    // Next step
    'Next step': '次のステップ',
    'All scripts': 'すべてのスクリプト',
    'Back to the schedule of every script.': 'すべてのスクリプトの一覧に戻ります。',
    'Browse scripts': 'スクリプトを見る',
    'Made one of your own?': '自分でも作りましたか？',
    'Share it, and any office can install it from the palette.': '共有すれば、どの事務所でもパレットからインストールできます。',
    'How sharing works': '共有の流れ'
});
