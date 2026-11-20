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
    'Not documented': 'ドキュメントなし',

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
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        '手すりのフロア表示の自動設定をオフにして、表示するフロアを指定できるようにする',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        'メッシュの Floor Plan Display、Show Projection、切断されない線と上方の線、または切断線の線種を設定する',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        '手すりの平面図シンボルを読み取る、または設定する：切断線、表示するシンボル構成要素、その2D属性、シンボルの上書き',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        '<a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> でスラブの輪郭を読み取り、<a href="../archicad-addon/#CreateRailings">CreateRailings</a> でそのエッジに沿って静的な手すりを作成してください。スラブが変わったら、手すりを削除して作成し直します。',
    'Anchor the sill or header of a window or door to the project zero or to a reference level':
        '窓やドアの下端または上端を、プロジェクトゼロまたは参照レベルに固定する',
    'Anchor it to a story with <code>verticalLink</code> <code>SillToStory</code> or <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> or <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, and work out the height from the story level (<a href="../archicad-addon/#GetStories">GetStories</a>).':
        '<a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> または <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a> の <code>verticalLink</code> に <code>SillToStory</code> または <code>HeaderToStory</code> を指定してフロアに固定し、高さはフロアのレベル（<a href="../archicad-addon/#GetStories">GetStories</a>）から計算してください。',
    'Read or set the joint type (Vertical Cut and the others) and the dx offset of a stair structure':
        '階段構造の接合タイプ（Vertical Cut など）と dx オフセットを読み取る、または設定する',
    'Change the baseline of an existing stair (move a flight end, a landing or a winder) and keep its GUID':
        '既存の階段の基準線を変更し（階段の端、踊り場、回り段を移動）、GUID はそのまま保つ',
    'Recreate the stair on the new baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, which keeps its settings, Element ID, classifications, categories, properties and solid element operations, and returns the old and the new GUID. Its labels are lost.':
        '<a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> で新しい基準線上に階段を作り直してください。設定、要素 ID、分類、カテゴリ、プロパティ、ソリッド要素操作は保たれ、古い GUID と新しい GUID が返されます。ラベルは失われます。',
    'Set a label’s optimised position, “use text pen as uniform pen” of a symbol label, or a perpendicular text direction':
        'ラベルの最適化された位置、シンボルラベルの「use text pen as uniform pen」、または直交する文字方向を設定する',
    'Set the text direction (parallel, horizontal, vertical or a custom angle) with textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> and <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. When the symbol label’s library part has a parameter for the other two, set it with <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; otherwise set them in the Label tool settings or a favorite, and create the labels with favoriteName.':
        '文字方向（平行、水平、垂直、任意の角度）は、<a href="../archicad-addon/#CreateLabels">CreateLabels</a> と <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a> の textWay で設定できます。シンボルラベルのライブラリ部品にほかの 2 つのためのパラメータがあれば、<a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a> で設定してください。なければラベルツールの設定かお気に入りで設定し、favoriteName を指定してラベルを作成してください。',
    'Create a radial dimension of an arc, a circle, a curved wall or a curved slab edge': '円弧、円、曲面壁、またはスラブの曲線エッジに半径寸法を作成する',
    'Place the radial dimension in Archicad and read it with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, which returns how Archicad stored it. Those readings are what a create command needs.':
        'Archicad で半径寸法を配置し、<a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> で読み取ってください。Archicad がどのように保存したかが返されます。作成コマンドに必要なのは、この読み取った値です。',

    // Design options
    'Design options': 'デザインオプション',
    'Read which design option, or the Main Model, is the default that new elements go to':
        '新しい要素が入る既定のデザインオプション（または Main Model）がどれかを読み取る',
    'Create a temporary element, read its design option with <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, then move it to the Main Model with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (a null GUID as <code>designOptionId</code>) and delete it with <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; an element in an option that is not shown cannot be deleted. Set the default with <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.':
        '一時的な要素を作成し、<a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a> でそのデザインオプションを読み取ってから、<a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> で Main Model に移動し（<code>designOptionId</code> に null GUID を指定）、<a href="../archicad-addon/#DeleteElements">DeleteElements</a> で削除してください。表示されていないオプションの要素は削除できません。既定は <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a> で設定できます。',
    'Read or change which design options the current window shows, as the eye icons of the Design Options palette do':
        'Design Options パレットの目のアイコンのように、現在のウィンドウに表示するデザインオプションを読み取る、または変更する',
    'Work with saved views: read and set the combination a View Map view shows with <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> and <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, and the options of a combination with <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.':
        '保存したビューを使ってください。View Map のビューが表示する組み合わせは <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> と <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a> で読み取り・設定でき、組み合わせのオプションは <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a> で設定できます。',
    'Duplicate a design option or a design option set, as the duplicate button of the Design Options palette does':
        'Design Options パレットの複製ボタンのように、デザインオプションまたはデザインオプションセットを複製する',
    'Create the set and its options with <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> and <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. For the elements, list them with <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copy them with <a href="../archicad-addon/#CopyElements">CopyElements</a> and move the copies to the new option with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.':
        'セットとそのオプションは <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> と <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a> で作成します。要素は <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a> で一覧にし、<a href="../archicad-addon/#CopyElements">CopyElements</a> でコピーして、<a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> でコピーを新しいオプションに移動してください。',
    'Lock or colour the passive elements of design options, as the Design Options palette does':
        'Design Options パレットのように、デザインオプションのパッシブ要素をロックする、または色付けする',

    // Navigator and views
    'Navigator and views': 'ナビゲータとビュー',
    'Add an item to a publisher set': 'パブリッシャーセット（publisher set）に項目を追加する',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad の Organizer で項目を追加し、<a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> でそのセットをパブリッシュします。',
    'Change, rename or create a stored Model View Options combination': '保存済みの Model View Options の組み合わせを変更、名前変更、または作成する',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        '組み合わせは <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> で読み取れます。現在のウィンドウのオプションは <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a> で変更できます。',
    'Read or set the floor plan cut plane or the structural analytical model settings saved in a view':
        'ビューに保存された平面図の切断面、または構造解析モデルの設定を読み取る、または設定する',
    'Set them in the view\'s View Settings in Archicad. The project\'s floor plan cut plane can be set with <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.':
        'Archicad でそのビューの View Settings から設定してください。プロジェクトの平面図の切断面は <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a> で設定できます。',
    'Tell a clone folder of the View Map from a normal folder': 'View Map のクローンフォルダを通常のフォルダと見分ける',
    'Give the clone folders a name or ID of their own and find them by it in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.':
        'クローンフォルダに独自の名前または ID を付け、<a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a> でそれを手がかりに見つけてください。',
    'Change the settings of an existing Layout Book subset (prefix, numbering style, start at, ...)':
        '既存の Layout Book のサブセットの設定（接頭辞、番号スタイル、開始番号、...）を変更する',
    'Read the subset with <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create a subset with the new settings with <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> and move the layouts into it with <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. The old, empty subset is deleted in Archicad.':
        '<a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a> でサブセットを読み取り、<a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> で新しい設定のサブセットを作成して、<a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a> でレイアウトをそこに移動してください。古い空のサブセットは Archicad で削除します。',

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
    'Delete one offset modifier of a profile': 'プロファイルのオフセット修正値を 1 つだけ削除する',
    'Rebuild the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>\' <code>replaceSkins</code> and <code>newSkins</code>, which drops every modifier, then add the ones to keep again with <code>newProfileModifiers</code> (Archicad 27 and later).':
        '<a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> の <code>replaceSkins</code> と <code>newSkins</code> でプロファイルを作り直し（すべての修正値が削除されます）、残したいものを <code>newProfileModifiers</code> で追加し直してください（Archicad 27 以降）。',
    'Import an IFC translator from a file into the project, or export one to a file':
        'IFC トランスレータをファイルからプロジェクトに読み込む、またはファイルに書き出す',
    'Keep the office translator in the project template, so new projects have it, and bring it into older projects once with Import… in the IFC Translators dialog. A script can check that a project has it with <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> and export with it by name with <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.':
        '事務所のトランスレータをプロジェクトテンプレートに入れておけば、新しいプロジェクトにはそれが入ります。古いプロジェクトには、IFC Translators ダイアログの Import… で一度だけ読み込んでください。スクリプトは <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> でプロジェクトにそれがあるかを確認し、<a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a> で名前を指定してそのトランスレータで書き出せます。',

    // Documents and publishing
    'Documents and publishing': '図面とパブリッシュ',
    'Read the rows of an Interactive Schedule': 'Interactive Schedule の行を読み取る',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        '一覧を自分で作ります。<a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> で要素を探し、<a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> または <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a> で値を読み取ります。<a href="../scripts/">組み込みスクリプト</a>の Room Finishes Schedule と Material Take-Off はこのような一覧を作ります。',
    'Export an Interactive Schedule': 'Interactive Schedule を書き出す',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad で一度だけ一覧表をパブリッシャーセットに入れておき、そのセットを <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> でパブリッシュします。',
    'Choose the output format (PDF, DWG, DXF …) of a publisher set per call': '呼び出しごとにパブリッシャーセットの出力形式（PDF、DWG、DXF …）を選ぶ',
    'Make one publisher set per format in Archicad’s Organizer once, then publish the one you need with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; its answer lists the files written (<code>writtenFiles</code>).':
        'Archicad の Organizer で形式ごとにパブリッシャーセットを一度作っておき、必要なものを <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> でパブリッシュしてください。その応答に、書き出されたファイルの一覧（<code>writtenFiles</code>）が含まれます。',
    'Create a revision Change without placing a Change Marker, or change the ID or description of an existing Change':
        'Change Marker を配置せずにリビジョンの Change を作成する、または既存の Change の ID や説明を変更する',
    'Create the Change together with its first marker with <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). To change an existing Change, use Archicad’s Change Manager.':
        '<a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a>（createChange）で、最初のマーカーと一緒に Change を作成してください。既存の Change を変更するには、Archicad の Change Manager を使います。',
    'Set the custom scheme fields of a revision Change': 'リビジョンの Change のカスタムスキームのフィールドを設定する',
    'Fill them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> reads them back.':
        'Archicad の Change Manager で入力してください。<a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> で読み戻せます。',
    'Add elements to a revision Change, or remove them from it': 'リビジョンの Change に要素を追加する、または Change から取り除く',
    'Add them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> reads which Changes an element belongs to.':
        'Archicad の Change Manager で追加してください。要素がどの Change に属しているかは <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> で読み取れます。',

    // Archicad's interface
    'Archicad\'s interface': 'Archicad のインターフェース',
    'Read the text of the Session Report window': 'Session Report ウィンドウのテキストを読み取る',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        '<a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> に windowType \'Report\' を指定して、ユーザーのためにウィンドウを開きます（Archicad 26 以降）。',
    'Get the outcome of an Action Center check': 'Action Center のチェック結果を取得する',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        '<a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> でチェックを実行します。問題が見つかると Archicad が Action Center を表示します。ホットリンクのチェックでは、<a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> で各ソースの状態を読み取れます（Archicad 27 以降）。',
    'Load a new add-on (.apx or .bundle) while Archicad runs, from an add-on or a script':
        'Archicad の実行中に、アドオンやスクリプトから新しいアドオン（.apx または .bundle）を読み込む',
    'Add it in Archicad\'s Add-On Manager (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). An add-on that is already loaded is replaced only after Archicad quits, as the Tapir Installer does when it updates Tapir.':
        'Archicad の Add-On Manager で追加してください（Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add）。すでに読み込まれているアドオンは、Archicad を終了した後でなければ置き換えられません。Tapir Installer も Tapir を更新するときにそうしています。',

    'Set the profile modifier values, or the stretched width and height, of a wall, beam or column that uses a profile':
        'プロファイルを使う壁、梁、柱のプロファイル修正値、またはストレッチされた幅と高さを設定する',
    'Read them with <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. To change them, change the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (a copy for these elements, or the profile itself), or set them in the settings of the element in Archicad.':
        '<a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a> で読み取れます。変更するには、<a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> でプロファイルを変更する（これらの要素用のコピー、またはプロファイル自体）か、Archicad で要素の設定から設定してください。',
    'Read or set how a grid element is shown in sections, elevations and interior elevations: its vertical extent and its markers at the top and bottom':
        '断面図、立面図、展開図でのグリッド要素の表示を読み取る、または設定する：鉛直方向の範囲と上端・下端のマーカー',
    'Select the grid elements in Archicad and set it for all of them at once in their settings dialog.':
        'Archicad でグリッド要素を選択し、その設定ダイアログですべてに一度に設定してください。',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'Archicad API にできないことをほかにも見つけましたか？確認した DevKit のヘッダーを添えて issue で知らせてください。ここに追加します。<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">issue を作成</a>'
});
