/* 日本語: the API limits page (limits/index.html). The Evidence column (headers and function names)
   stays English. See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'What the Archicad API can\'t do | Tapir': 'Archicad API にできないこと | Tapir',
    'What people ask for that Graphisoft\'s Archicad API does not offer, such as undo or the clipboard, each with what to do instead and the DevKit evidence.':
        '求められているのに Graphisoft の Archicad API が提供していないもの（元に戻す、クリップボードなど）を、代わりにできることと DevKit での根拠とともにまとめています。',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API limits': '<a href="../">Tapir</a> / <a href="../docs/">ドキュメント</a> / API の限界',
    'What the Archicad API <em>can\'t do.</em>': 'Archicad API に<em>できないこと。</em>',
    'Tapir uses the Archicad API that Graphisoft publishes for add-ons, and covers all of it that can be a JSON command. What that API does not offer, Tapir cannot offer either. This page lists what people ask for that is not in it, with what to do instead.':
        'Tapir は Graphisoft がアドオン向けに公開している Archicad API を使い、そのうち JSON コマンドにできるものはすべてカバーしています。その API にないものは、Tapir でも提供できません。このページでは、求められているのに API にないものと、代わりにできることをまとめています。',

    // The list
    'The <em>list.</em>': '<em>一覧。</em>',
    'It is also what Tapir would like the Archicad API to offer, and the list Tapir sends to Graphisoft. Every row was checked in the headers of the Archicad API DevKits of Archicad 25 to 30, the versions Tapir is built for; the Evidence column names the headers and functions.':
        'これは Tapir が Archicad API に望むものでもあり、Tapir が Graphisoft に送る一覧でもあります。各行は、Tapir がビルドされている Archicad 25〜30 の Archicad API DevKit のヘッダーで確認しました。「根拠」列にそのヘッダーと関数を記しています。',
    'What people want': '求められていること',
    'Status': '状況',
    'What to do instead': '代わりにできること',
    'Evidence': '根拠',
    'Issue': 'Issue',
    'Not possible': '不可能',
    'Partly possible': '一部可能',
    'Workaround': '回避策あり',
    'Nothing yet': 'まだなし',

    // Elements
    'Elements': '要素',
    'Explode an element into its parts, as Edit &gt; Reshape &gt; Explode into Current View does':
        'Edit &gt; Reshape &gt; Explode into Current View のように要素を部品に分解する',
    'A camera with a name of its own, or an axonometric camera': '独自の名前を持つカメラ、またはアクソメのカメラ',
    'Name the Camera Set the camera is in. For an axonometric view, set the 3D window with <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.':
        'カメラが入っている Camera Set に名前を付けてください。アクソメのビューには、<a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a> で 3D ウィンドウを設定します。',
    'Remove one direction of a trim between two roofs or shells that trim each other':
        '互いにトリムし合う 2 つの屋根またはシェルの間のトリムを、片方向だけ削除する',
    'Remove the trim with <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, which removes both directions, then trim the other direction again with <a href="../archicad-addon/#TrimElements">TrimElements</a>.':
        '<a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a> でトリムを削除し（両方向が削除されます）、もう一方の方向を <a href="../archicad-addon/#TrimElements">TrimElements</a> で再びトリムしてください。',
    'Read which slab or stair edge a railing is linked to, or link a railing to one':
        '手すりがどのスラブまたは階段のエッジに関連付けられているかを読み取る、または手すりをエッジに関連付ける',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        '<a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> でスラブの輪郭を読み取り、<a href="../archicad-addon/#CreateRailings">CreateRailings</a> でそのエッジに沿って静的な手すりを作成してください。スラブが変わったら、手すりを削除して作成し直します。',

    // Navigator and views
    'Navigator and views': 'ナビゲータとビュー',
    'Add an item to a publisher set': 'パブリッシャーセット（publisher set）に項目を追加する',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad の Organizer で項目を追加し、<a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> でそのセットをパブリッシュします。',
    'Change, rename or create a stored Model View Options combination': '保存済みの Model View Options の組み合わせを変更、名前変更、または作成する',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        '組み合わせは <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> で読み取れます。現在のウィンドウのオプションは <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a> で変更できます。',

    // Project and files
    'Project and files': 'プロジェクトとファイル',
    'Undo or redo, as Edit &gt; Undo does': 'Edit &gt; Undo のように元に戻す、またはやり直す',
    'Most Tapir commands change the model in an undo step, so the user can undo them in Archicad; the command reference says which cannot be undone. Save the project with <a href="../archicad-addon/#SaveProject">SaveProject</a> before a script that may need to be taken back.':
        'ほとんどの Tapir コマンドはモデルを 1 つの取り消し単位で変更するので、ユーザーは Archicad で元に戻せます。元に戻せないコマンドはコマンドリファレンスに書かれています。取り消す必要があるかもしれないスクリプトの前に、<a href="../archicad-addon/#SaveProject">SaveProject</a> でプロジェクトを保存してください。',
    'Copy and paste through the clipboard': 'クリップボードを使ったコピーと貼り付け',
    'In a project, copy 2D elements to another view with <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Between projects, save the elements as a module with <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, place it in the other project with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, and, to keep plain elements, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'プロジェクト内では、<a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a> で 2D 要素を別のビューにコピーします。プロジェクト間では、<a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a> で要素をモジュールとして保存し、<a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> と <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> で別のプロジェクトに配置し、通常の要素として残すには <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a> を使います。',
    'Merge a file into the project, as File &gt; Interoperability &gt; Merge does': 'File &gt; Interoperability &gt; Merge のようにファイルをプロジェクトに結合する',
    'Place the file as a hotlink with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, then <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        '<a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> と <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> でファイルをホットリンクとして配置し、続けて <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a> を使います。',

    // Documents and publishing
    'Documents and publishing': '図面とパブリッシュ',
    'Read the rows of an Interactive Schedule': 'Interactive Schedule の行を読み取る',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        '一覧を自分で作ります。<a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> で要素を探し、<a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> または <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a> で値を読み取ります。<a href="../scripts/">組み込みスクリプト</a>の Room Finishes Schedule と Material Take-Off はこのような一覧を作ります。',
    'Export an Interactive Schedule': 'Interactive Schedule を書き出す',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad で一度だけ一覧表をパブリッシャーセットに入れておき、そのセットを <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> でパブリッシュします。',

    // Archicad's interface
    'Archicad\'s interface': 'Archicad のインターフェース',
    'Read the text of the Session Report window': 'Session Report ウィンドウのテキストを読み取る',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        '<a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> に windowType \'Report\' を指定して、ユーザーのためにウィンドウを開きます（Archicad 26 以降）。',
    'Get the outcome of an Action Center check': 'Action Center のチェック結果を取得する',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        '<a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> でチェックを実行します。問題が見つかると Archicad が Action Center を表示します。ホットリンクのチェックでは、<a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> で各ソースの状態を読み取れます（Archicad 27 以降）。',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Archicad API にできないことをほかにも見つけましたか？確認した DevKit のヘッダーを添えて issue で知らせてください。ここに追加します。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">issue を作成</a>'
});
