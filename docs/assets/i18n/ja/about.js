/* 日本語: the About page (about/index.html). See ../README.md.
   The names of the people stay as they are. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'About Tapir – open-source Archicad add-on': 'Tapir について – オープンソースの Archicad アドオン',
    'Why the open-source Archicad add-on Tapir exists, how it grew from Enzyme\'s R&D and Tibor Lorántfy\'s JSON template, who builds it and how to support it.':
        'オープンソースの Archicad アドオン Tapir が生まれた理由、Enzyme の研究開発と Tibor Lorántfy の JSON テンプレートから育った経緯、開発している人々、そして支援の方法。',
    '<a href="../">Tapir</a> / About': '<a href="../">Tapir</a> / Tapir について',
    'About <em>Tapir.</em>': '<em>Tapir</em> について。',
    'An open-source project that makes Archicad scriptable for architects, not only for programmers. Started in 2022, released every week since.':
        'プログラマーだけでなく建築家も Archicad をスクリプトで操作できるようにするオープンソースプロジェクトです。2022 年に始まり、以来毎週リリースしています。',

    // Why
    'Why it <em>exists.</em>': '<em>存在する理由。</em>',
    'Archicad\'s own JSON API covers only part of what its add-on API can do. Tapir adds the missing commands as an add-on, then puts them within reach of people who don\'t code: a palette of ready scripts, Grasshopper components, and AI assistants.':
        'Archicad 自身の JSON API は、アドオン API でできることの一部しかカバーしていません。Tapir は足りないコマンドをアドオンとして追加し、コードを書かない人にも届けます。すぐに使えるスクリプトのパレット、Grasshopper コンポーネント、そして AI アシスタントです。',

    // How it started
    'Revision history': '改訂履歴',
    'How it <em>started.</em>': 'どのように<em>始まったか。</em>',
    'Started as R&amp;D at Enzyme, an architecture practice, then opened to everyone, in the spirit of ifc.js: practitioners shouldn\'t wait for a vendor to support their workflow.':
        '建築設計事務所 Enzyme の研究開発として始まり、その後 ifc.js の精神ですべての人に開かれました。実務者は、ベンダーが自分のワークフローに対応するのを待つべきではありません。',
    'Revisions · Tapir': '改訂履歴 · Tapir',
    'Rev': '改訂',
    'Date': '日付',
    'Description': '内容',
    'By': '担当',
    '5 July 2022': '2022 年 7 月 5 日',
    'Born in Enzyme\'s R&amp;D': 'Enzyme の研究開発から誕生',
    'Archicad\'s JSON interface was a step forward, but Grasshopper users still needed C++ add-ons to reach native elements. The first commit, with the MIT licence.':
        'Archicad の JSON インターフェースは一歩前進でしたが、Grasshopper ユーザーがネイティブ要素に届くには、まだ C++ のアドオンが必要でした。MIT ライセンスでの最初のコミット。',
    '7 July 2022': '2022 年 7 月 7 日',
    'A place to talk': '話し合う場所',
    'The Discord server opens, where architects and developers say what they need and show what they made.':
        'Discord サーバーが開設。建築家と開発者が、必要なものを伝え、作ったものを見せ合う場です。',
    'The first members': '最初のメンバー',
    'July 2024': '2024 年 7 月',
    'The template': 'テンプレート',
    'Tibor Lorántfy, whose archicad-additional-json-commands the add-on is built on, joins the project. Almost any function of the Archicad API can now become a JSON command.':
        'アドオンの土台である archicad-additional-json-commands の作者、Tibor Lorántfy がプロジェクトに参加。Archicad API のほぼすべての関数を JSON コマンドにできるようになりました。',
    'Built by its community': 'コミュニティによる開発',
    'A release every Monday. Reports on Discord become GitHub issues, bots draft the fixes and people test them in their own Archicad. <a href="../community/#activity">See the growth</a>.':
        '毎週月曜日にリリース。Discord での報告は GitHub の Issue になり、ボットが修正案を作り、人が自分の Archicad でテストします。<a href="../community/#activity">成長を見る</a>。',
    'Everyone who reports, tests and contributes': '報告し、テストし、貢献するすべての人',

    // The people
    'The people': '人々',
    'The people <em>behind it.</em>': 'Tapir を<em>支える人々。</em>',
    'ENZYME-APD runs the project and publishes Tapir under the MIT licence. Everyone below has written code for it, or tested it and given the feedback it is built on, in alphabetical order.':
        'ENZYME-APD がプロジェクトを運営し、Tapir を MIT ライセンスで公開しています。以下の皆さんは、コードを書いたり、テストして Tapir の土台となるフィードバックをくれたりした方々です（アルファベット順）。',
    'Many more report bugs, test fixes in their own Archicad and share scripts; bots draft fixes, and people test and approve them. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/graphs/contributors">All contributors on GitHub</a>':
        'ほかにも多くの人がバグを報告し、自分の Archicad で修正をテストし、スクリプトを共有しています。ボットが修正案を作り、人がそれをテストして承認します。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/graphs/contributors">GitHub のすべてのコントリビューター</a>',

    // Principles
    'Principles': '原則',
    '<b>FREE</b><strong>No price, ever</strong><span>MIT licence for the add-on, the plugin and every shared script.</span>':
        '<b>無料</b><strong>ずっと無料</strong><span>アドオン、プラグイン、共有されるすべてのスクリプトに MIT ライセンス。</span>',
    '<b>OPEN</b><strong>Built in public</strong><span>Every issue, pull request and review is on GitHub.</span>':
        '<b>オープン</b><strong>公開の場で開発</strong><span>すべての issue、pull request、レビューは GitHub にあります。</span>',
    '<b>SIGNED</b><strong>Safe to install</strong><span>The Windows installer is signed through SignPath; the macOS installer and add-ons are signed and notarized.</span>':
        '<b>署名済み</b><strong>安心してインストール</strong><span>Windows インストーラーは SignPath で署名され、macOS インストーラーとアドオンは署名と公証を受けています。</span>',
    '<b>WEEKLY</b><strong>Small, steady releases</strong><span>A release every Monday when something changed.</span>':
        '<b>毎週</b><strong>小さく着実なリリース</strong><span>変更があれば毎週月曜日にリリースします。</span>',

    // Support
    'Support Tapir': 'Tapir を支援する',
    'Support <em>Tapir.</em>': '<em>Tapir</em> を支援する。',
    'Tapir is free and open source. This is how it stays up to date with every Archicad version and keeps growing.':
        'Tapir は無料のオープンソースです。こうした支援によって、Archicad の各バージョンに追従し、成長を続けています。',
    'Everyone': 'すべての人',
    'Community crowdfunding': 'コミュニティのクラウドファンディング',
    'Small contributions from many users make a big difference: regular small pledges can fund maintainers who update Tapir for each new Archicad version.':
        '多くのユーザーからの小さな支援が大きな違いを生みます。定期的な少額の支援で、Archicad の新バージョンごとに Tapir を更新するメンテナーを支えられます。',
    'Practices and firms': '設計事務所と企業',
    'Corporate sponsoring': '企業スポンサー',
    'Offices and engineering firms that use Tapir in their own R&amp;D sponsor the commands their projects need, and those commands then serve everyone.':
        '自社の研究開発で Tapir を使う設計事務所やエンジニアリング会社が、プロジェクトに必要なコマンドのスポンサーになり、そのコマンドはみんなの役に立ちます。',
    'Your office': 'あなたの事務所',
    'Training and services': 'トレーニングとサービス',
    'Training sessions, implementation workshops and custom development for offices moving to computational BIM workflows with Tapir.':
        'Tapir でコンピュテーショナル BIM のワークフローに移行する事務所向けに、トレーニング、導入ワークショップ、カスタム開発を提供します。',
    'Talk to us: <strong class="nowrap">tapir@weareenzyme.com</strong>': 'お問い合わせ：<strong class="nowrap">tapir@weareenzyme.com</strong>',
    'Free code signing provided by SignPath.io, certificate by SignPath Foundation. Archicad is a trademark of Graphisoft SE; Tapir is not made by Graphisoft.':
        '無料のコード署名は SignPath.io が提供し、証明書は SignPath Foundation によるものです。Archicad は Graphisoft SE の商標です。Tapir は Graphisoft 製ではありません。'
});
