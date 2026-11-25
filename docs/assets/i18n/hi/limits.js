/* हिन्दी: the API limits page (limits/index.html). The Evidence column (headers and function names)
   stays English. See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'What the Archicad API can\'t do | Tapir': 'Archicad API क्या नहीं कर सकता | Tapir',
    'What people ask for that Graphisoft\'s Archicad API does not offer, such as undo or the clipboard, each with what to do instead and the DevKit evidence.':
        'लोग जो माँगते हैं पर Graphisoft का Archicad API नहीं देता, जैसे undo या clipboard, हर एक के साथ उसकी जगह क्या करें और DevKit का सबूत।',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API limits': '<a href="../">Tapir</a> / <a href="../docs/">दस्तावेज़</a> / API की सीमाएँ',
    'What the Archicad API <em>can\'t do.</em>': 'Archicad API <em>क्या नहीं कर सकता।</em>',
    'Tapir uses the Archicad API that Graphisoft publishes for add-ons, and covers all of it that can be a JSON command. What that API does not offer, Tapir cannot offer either. This page lists what people ask for that is not in it, with what to do instead.':
        'Tapir वह Archicad API इस्तेमाल करता है जो Graphisoft add-ons के लिए प्रकाशित करता है, और उसका हर वह हिस्सा cover करता है जो JSON command बन सकता है। जो वह API नहीं देता, वह Tapir भी नहीं दे सकता। यह पेज बताता है कि लोग क्या माँगते हैं जो उसमें नहीं है, और उसकी जगह क्या करें।',

    // The list
    'The <em>list.</em>': '<em>सूची।</em>',
    'It is also what Tapir would like the Archicad API to offer, and the list Tapir sends to Graphisoft. Every row was checked in the headers of the Archicad API DevKits of Archicad 25 to 30, the versions Tapir is built for; the Evidence column names the headers and functions.':
        'यह वह भी है जो Tapir चाहता है कि Archicad API दे, और वह सूची जो Tapir Graphisoft को भेजता है। हर row को Archicad 25 से 30 तक, जिन versions के लिए Tapir build होता है, उनके Archicad API DevKits के headers में जाँचा गया है; सबूत वाला column उन headers और functions के नाम बताता है।',
    'What people want': 'लोग क्या चाहते हैं',
    'Status': 'स्थिति',
    'What to do instead': 'उसकी जगह क्या करें',
    'Evidence': 'सबूत',
    'Issue': 'Issue',
    'Not possible': 'संभव नहीं',
    'Partly possible': 'आंशिक रूप से संभव',
    'Workaround': 'Workaround',
    'Nothing yet': 'अभी कुछ नहीं',
    'Not documented': 'Documentation में नहीं',

    // Elements
    'Elements': 'Elements',
    'Explode an element into its parts, as Edit &gt; Reshape &gt; Explode into Current View does':
        'किसी element को उसके हिस्सों में तोड़ना, जैसे Edit &gt; Reshape &gt; Explode into Current View करता है',
    'A camera with a name of its own, or an axonometric camera': 'अपने नाम वाला camera, या axonometric camera',
    'Name the Camera Set the camera is in. For an axonometric view, set the 3D window with <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a>.':
        'जिस Camera Set में camera है, उसका नाम रखें। Axonometric view के लिए <a href="../archicad-addon/#Set3DProjectionSettings">Set3DProjectionSettings</a> से 3D window सेट करें।',
    'Remove one direction of a trim between two roofs or shells that trim each other':
        'एक-दूसरे को trim करने वाली दो roofs या shells के बीच trim की केवल एक दिशा हटाना',
    'Remove the trim with <a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a>, which removes both directions, then trim the other direction again with <a href="../archicad-addon/#TrimElements">TrimElements</a>.':
        '<a href="../archicad-addon/#RemoveElementTrims">RemoveElementTrims</a> से trim हटाएँ, जो दोनों दिशाएँ हटाता है, फिर दूसरी दिशा को <a href="../archicad-addon/#TrimElements">TrimElements</a> से दोबारा trim करें।',
    'Read which slab or stair edge a railing is linked to, or link a railing to one':
        'पढ़ना कि railing किस slab या stair के किनारे से जुड़ी है, या railing को किसी किनारे से जोड़ना',
    'Turn off the automatic story visibility of a railing, so the stories it is shown on can be set':
        'रेलिंग की स्वचालित स्टोरी दृश्यता बंद करना, ताकि वे स्टोरी तय की जा सकें जिन पर वह दिखती है',
    'Set the Floor Plan Display, Show Projection, uncut and overhead lines or the cut line type of a mesh':
        'मेश का Floor Plan Display, Show Projection, बिना कटी और ऊपर की रेखाएँ या कट रेखा का प्रकार तय करना',
    'Read or set the floor plan symbol of a railing: its break mark, which symbol components it shows, their 2D attributes and the symbol overrides':
        'रेलिंग का floor plan symbol पढ़ना या सेट करना: उसका break mark, वह कौन-से symbol components दिखाती है, उनके 2D attributes और symbol overrides',
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        '<a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> से slab की outline पढ़ें और <a href="../archicad-addon/#CreateRailings">CreateRailings</a> से किनारे के साथ एक static railing बनाएँ। जब slab बदले, railing हटाकर फिर से बनाएँ।',
    'Move a window or door into another wall': 'किसी window या door को दूसरी wall में ले जाना',
    'Delete it and create it in the other wall with <a href="../archicad-addon/#CreateWindows">CreateWindows</a> or <a href="../archicad-addon/#CreateDoors">CreateDoors</a>; a favorite (<code>favoriteName</code>) gives it the same settings.':
        'उसे हटाएँ और <a href="../archicad-addon/#CreateWindows">CreateWindows</a> या <a href="../archicad-addon/#CreateDoors">CreateDoors</a> से दूसरी wall में फिर से बनाएँ; एक favorite (<code>favoriteName</code>) उसे वही settings देता है।',
    'Move a skylight onto another roof or shell': 'किसी skylight को दूसरी roof या shell पर ले जाना',
    'Delete it and create it on the other roof or shell with <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a>.':
        'उसे हटाएँ और <a href="../archicad-addon/#CreateSkylights">CreateSkylights</a> से दूसरी roof या shell पर फिर से बनाएँ।',
    'Turn an automatic zone into a manual one': 'किसी automatic zone को manual zone में बदलना',
    'Delete the zone and create it again as a manual zone, from its polygon, with <a href="../archicad-addon/#CreateZones">CreateZones</a>.':
        'Zone को हटाएँ और <a href="../archicad-addon/#CreateZones">CreateZones</a> से उसके polygon से manual zone के रूप में फिर से बनाएँ।',
    'Change the height of a slanted curtain wall': 'किसी slanted curtain wall की height बदलना',
    'Create it again with <a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a>, with its <code>angle</code> and the new <code>height</code>.':
        '<a href="../archicad-addon/#CreateCurtainWalls">CreateCurtainWalls</a> से उसे उसके <code>angle</code> और नई <code>height</code> के साथ फिर से बनाएँ।',
    'Move the lines or the arc of an angle dimension': 'किसी angle dimension की lines या arc को खिसकाना',
    'Delete it and create it again with <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a>.':
        'उसे हटाएँ और <a href="../archicad-addon/#CreateAngleDimensions">CreateAngleDimensions</a> से फिर से बनाएँ।',
    'Set the position or the offset of a stair\'s walking line': 'किसी stair की walking line की position या offset सेट करना',
    'Anchor the sill or header of a window or door to the project zero or to a reference level':
        'Window या door के sill या header को project zero या किसी reference level से anchor करना',
    'Anchor it to a story with <code>verticalLink</code> <code>SillToStory</code> or <code>HeaderToStory</code> in <a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> or <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a>, and work out the height from the story level (<a href="../archicad-addon/#GetStories">GetStories</a>).':
        '<a href="../archicad-addon/#ModifyWindows">ModifyWindows</a> या <a href="../archicad-addon/#ModifyDoors">ModifyDoors</a> में <code>verticalLink</code> <code>SillToStory</code> या <code>HeaderToStory</code> से इसे किसी story से anchor करें, और ऊँचाई story level (<a href="../archicad-addon/#GetStories">GetStories</a>) से निकालें।',
    'Read or set the joint type (Vertical Cut and the others) and the dx offset of a stair structure':
        'Stair structure का joint type (Vertical Cut और बाकी) और dx offset पढ़ना या सेट करना',
    'Change the baseline of an existing stair (move a flight end, a landing or a winder) and keep its GUID':
        'किसी मौजूदा stair की baseline बदलना (flight का सिरा, landing या winder खिसकाना) और उसका GUID बनाए रखना',
    'Recreate the stair on the new baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a>, which keeps its settings, Element ID, classifications, categories, properties and solid element operations, and returns the old and the new GUID. Its labels are lost.':
        '<a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> से stair को नई baseline पर दोबारा बनाएँ, जो उसकी settings, Element ID, classifications, categories, properties और solid element operations बनाए रखता है, और पुराना और नया GUID लौटाता है। उसके labels खो जाते हैं।',
    'Set the left and right boundaries or the walking line of a stair (a custom outline, or a walking line off the middle of the flight)':
        'किसी stair की बाईं और दाईं boundaries या walking line तय करना (अपनी outline, या flight के बीच से हटी walking line)',
    'Archicad lays them out from the baseline and the flight width: change the baseline with <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> or the flight width with <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a>. The walking line stays in the middle of the flight. <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> reads them.':
        'Archicad इन्हें baseline और flight width से बनाता है: baseline <a href="../archicad-addon/#RecreateStairs">RecreateStairs</a> से या flight width <a href="../archicad-addon/#ModifyStairs">ModifyStairs</a> से बदलें। walking line flight के बीच में ही रहती है। <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> इन्हें पढ़ता है।',
    'Set a label’s optimised position, “use text pen as uniform pen” of a symbol label, or a perpendicular text direction':
        'किसी label की optimised position, symbol label का “use text pen as uniform pen”, या perpendicular text direction सेट करना',
    'Set the text direction (parallel, horizontal, vertical or a custom angle) with textWay in <a href="../archicad-addon/#CreateLabels">CreateLabels</a> and <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a>. When the symbol label’s library part has a parameter for the other two, set it with <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a>; otherwise set them in the Label tool settings or a favorite, and create the labels with favoriteName.':
        'Text direction (parallel, horizontal, vertical या custom angle) <a href="../archicad-addon/#CreateLabels">CreateLabels</a> और <a href="../archicad-addon/#ModifyLabels">ModifyLabels</a> में textWay से सेट करें। अगर symbol label के library part में बाकी दोनों के लिए parameter है, तो उसे <a href="../archicad-addon/#SetGDLParametersOfElements">SetGDLParametersOfElements</a> से सेट करें; नहीं तो उन्हें Label tool settings या किसी favorite में सेट करें, और labels को favoriteName से बनाएँ।',
    'Create a radial dimension of an arc, a circle, a curved wall or a curved slab edge':
        'किसी arc, circle, curved wall या curved slab edge का radial dimension बनाना',
    'Place the radial dimension in Archicad and read it with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a>, which returns how Archicad stored it. Those readings are what a create command needs.':
        'Radial dimension को Archicad में रखें और <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> से पढ़ें, जो बताता है कि Archicad ने उसे कैसे store किया। Create command को यही readings चाहिए।',

    // Design options
    'Design options': 'Design options',
    'Read which design option, or the Main Model, is the default that new elements go to':
        'पढ़ना कि नए elements डिफ़ॉल्ट रूप से किस design option में, या Main Model में, जाते हैं',
    'Create a temporary element, read its design option with <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a>, then move it to the Main Model with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> (a null GUID as <code>designOptionId</code>) and delete it with <a href="../archicad-addon/#DeleteElements">DeleteElements</a>; an element in an option that is not shown cannot be deleted. Set the default with <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a>.':
        'एक अस्थायी element बनाएँ, <a href="../archicad-addon/#GetDesignOptionForElements">GetDesignOptionForElements</a> से उसका design option पढ़ें, फिर <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> से उसे Main Model में ले जाएँ (<code>designOptionId</code> के रूप में null GUID) और <a href="../archicad-addon/#DeleteElements">DeleteElements</a> से delete करें; जो option नहीं दिख रहा, उसके element को delete नहीं किया जा सकता। डिफ़ॉल्ट <a href="../archicad-addon/#SetDefaultDesignOption">SetDefaultDesignOption</a> से सेट करें।',
    'Read or change which design options the current window shows, as the eye icons of the Design Options palette do':
        'पढ़ना या बदलना कि current window कौन-से design options दिखाती है, जैसे Design Options palette के eye icons करते हैं',
    'Work with saved views: read and set the combination a View Map view shows with <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> and <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a>, and the options of a combination with <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a>.':
        'Saved views के साथ काम करें: View Map view जो combination दिखाता है, उसे <a href="../archicad-addon/#GetDesignOptionCombinationOfViews">GetDesignOptionCombinationOfViews</a> और <a href="../archicad-addon/#SetDesignOptionCombinationOfViews">SetDesignOptionCombinationOfViews</a> से पढ़ें और सेट करें, और किसी combination के options <a href="../archicad-addon/#SetActiveDesignOptionsInCombinations">SetActiveDesignOptionsInCombinations</a> से।',
    'Duplicate a design option or a design option set, as the duplicate button of the Design Options palette does':
        'किसी design option या design option set को duplicate करना, जैसे Design Options palette का duplicate button करता है',
    'Create the set and its options with <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> and <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a>. For the elements, list them with <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a>, copy them with <a href="../archicad-addon/#CopyElements">CopyElements</a> and move the copies to the new option with <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a>.':
        'Set और उसके options <a href="../archicad-addon/#CreateDesignOptionSets">CreateDesignOptionSets</a> और <a href="../archicad-addon/#CreateDesignOptions">CreateDesignOptions</a> से बनाएँ। Elements के लिए, उन्हें <a href="../archicad-addon/#GetElementsOfDesignOptions">GetElementsOfDesignOptions</a> से list करें, <a href="../archicad-addon/#CopyElements">CopyElements</a> से copy करें और copies को <a href="../archicad-addon/#MoveElementsToDesignOptions">MoveElementsToDesignOptions</a> से नए option में ले जाएँ।',
    'Lock or colour the passive elements of design options, as the Design Options palette does':
        'Design options के passive elements को lock करना या रंगना, जैसे Design Options palette करता है',

    // Navigator and views
    'Navigator and views': 'Navigator और views',
    'Add an item to a publisher set': 'किसी publisher set में item जोड़ना',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad के Organizer में items जोड़ें, फिर <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> से set को publish करें।',
    'Rename a publisher set': 'किसी publisher set का नाम बदलना',
    'Rename the set in Archicad\'s Organizer. <a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> reads the names, and <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> publishes a set by its name.':
        'Set का नाम Archicad के Organizer में बदलें। <a href="../archicad-addon/#GetPublisherSets">GetPublisherSets</a> नाम पढ़ता है, और <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> किसी set को उसके नाम से publish करता है।',
    'Change, rename or create a stored Model View Options combination': 'Model View Options का कोई saved combination बदलना, उसका नाम बदलना या नया बनाना',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Combinations को <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> से पढ़ें; मौजूदा window के options <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a> से बदलें।',
    'Make a 3D style the current one': 'किसी 3D style को current style बनाना',
    'Give a view the 3D style with <a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) and open that view with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, from Archicad 27).':
        '<a href="../archicad-addon/#SetViewSettings">SetViewSettings</a> (<code>d3styleName</code>) से किसी view को वह 3D style दें और <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> (<code>navigatorItemId</code>, Archicad 27 से) से वह view खोलें।',
    'Read or set the floor plan cut plane or the structural analytical model settings saved in a view':
        'किसी view में saved floor plan cut plane या structural analytical model settings पढ़ना या सेट करना',
    'Set them in the view\'s View Settings in Archicad. The project\'s floor plan cut plane can be set with <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a>.':
        'इन्हें Archicad में view की View Settings में सेट करें। Project का floor plan cut plane <a href="../archicad-addon/#SetProjectPreferences">SetProjectPreferences</a> से सेट किया जा सकता है।',
    'Tell a clone folder of the View Map from a normal folder': 'View Map के clone folder को सामान्य folder से अलग पहचानना',
    'Give the clone folders a name or ID of their own and find them by it in <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a>.':
        'Clone folders को उनका अपना नाम या ID दें और <a href="../archicad-addon/#GetNavigatorItemTree">GetNavigatorItemTree</a> में उसी से उन्हें ढूँढें।',
    'Change the settings of an existing Layout Book subset (prefix, numbering style, start at, ...)':
        'किसी मौजूदा Layout Book subset की settings बदलना (prefix, numbering style, start at, ...)',
    'Read the subset with <a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a>, create a subset with the new settings with <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> and move the layouts into it with <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a>. The old, empty subset is deleted in Archicad.':
        '<a href="../archicad-addon/#GetLayoutSubsetSettings">GetLayoutSubsetSettings</a> से subset पढ़ें, <a href="../archicad-addon/#CreateLayoutSubset">CreateLayoutSubset</a> से नई settings वाला subset बनाएँ और <a href="../archicad-addon/#MoveNavigatorItem">MoveNavigatorItem</a> से layouts उसमें ले जाएँ। पुराना, खाली subset Archicad में delete किया जाता है।',

    // Project and files
    'Project and files': 'Project और files',
    'Undo or redo, as Edit &gt; Undo does': 'Undo या redo, जैसे Edit &gt; Undo करता है',
    'Most Tapir commands change the model in an undo step, so the user can undo them in Archicad; the command reference says which cannot be undone. Save the project with <a href="../archicad-addon/#SaveProject">SaveProject</a> before a script that may need to be taken back.':
        'ज़्यादातर Tapir commands model को एक undo step में बदलते हैं, इसलिए user उन्हें Archicad में undo कर सकता है; command reference बताता है कि किन्हें undo नहीं किया जा सकता। जिस script को शायद वापस लेना पड़े, उससे पहले <a href="../archicad-addon/#SaveProject">SaveProject</a> से project save करें।',
    'Copy and paste through the clipboard': 'Clipboard से copy और paste',
    'In a project, copy 2D elements to another view with <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a>. Between projects, save the elements as a module with <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a>, place it in the other project with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, and, to keep plain elements, <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'एक project के अंदर, 2D elements को <a href="../archicad-addon/#CopyElementsToDatabase">CopyElementsToDatabase</a> से दूसरे view में copy करें। Projects के बीच, elements को <a href="../archicad-addon/#SaveAsModuleFile">SaveAsModuleFile</a> से module के रूप में save करें, उसे <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> और <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> से दूसरे project में रखें, और साधारण elements रखने के लिए <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a> चलाएँ।',
    'Merge a file into the project, as File &gt; Interoperability &gt; Merge does': 'किसी file को project में merge करना, जैसे File &gt; Interoperability &gt; Merge करता है',
    'Place the file as a hotlink with <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> and <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a>, then <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a>.':
        'File को <a href="../archicad-addon/#CreateHotlinkNodes">CreateHotlinkNodes</a> और <a href="../archicad-addon/#CreateHotlinkInstances">CreateHotlinkInstances</a> से hotlink के रूप में रखें, फिर <a href="../archicad-addon/#BreakHotlinkNodes">BreakHotlinkNodes</a> चलाएँ।',
    'Delete one offset modifier of a profile': 'Profile का एक offset modifier delete करना',
    'Rebuild the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a>\' <code>replaceSkins</code> and <code>newSkins</code>, which drops every modifier, then add the ones to keep again with <code>newProfileModifiers</code> (Archicad 27 and later).':
        '<a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> के <code>replaceSkins</code> और <code>newSkins</code> से profile दोबारा बनाएँ, जो हर modifier हटा देता है, फिर जिन्हें रखना है उन्हें <code>newProfileModifiers</code> से फिर जोड़ें (Archicad 27 और बाद में)।',
    'Import an IFC translator from a file into the project, or export one to a file':
        'किसी file से IFC translator को project में import करना, या उसे file में export करना',
    'Keep the office translator in the project template, so new projects have it, and bring it into older projects once with Import… in the IFC Translators dialog. A script can check that a project has it with <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> and export with it by name with <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a>.':
        'Office का translator project template में रखें, ताकि नए projects में वह हो, और पुराने projects में उसे एक बार IFC Translators dialog के Import… से लाएँ। Script <a href="../archicad-addon/#GetIFCExportTranslators">GetIFCExportTranslators</a> से जाँच सकती है कि project में वह है, और <a href="../archicad-addon/#IFCFileOperation">IFCFileOperation</a> से उसके नाम से उसी के साथ export कर सकती है।',

    // Documents and publishing
    'Documents and publishing': 'Documents और publishing',
    'Read the rows of an Interactive Schedule': 'किसी Interactive Schedule की rows पढ़ना',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'सूची ख़ुद बनाएँ: <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> से elements ढूँढें और उनकी values <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> या <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a> से पढ़ें। <a href="../scripts/">Built-in scripts</a> Room Finishes Schedule और Material Take-Off ऐसी सूचियाँ बनाते हैं।',
    'Export an Interactive Schedule': 'किसी Interactive Schedule को export करना',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad में schedule को एक बार किसी publisher set में डालें, फिर उस set को <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> से publish करें।',
    'Choose the output format (PDF, DWG, DXF …) of a publisher set per call':
        'हर call पर publisher set का output format (PDF, DWG, DXF …) चुनना',
    'Make one publisher set per format in Archicad’s Organizer once, then publish the one you need with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>; its answer lists the files written (<code>writtenFiles</code>).':
        'Archicad के Organizer में एक बार हर format के लिए एक publisher set बनाएँ, फिर जिसकी ज़रूरत हो उसे <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> से publish करें; उसका जवाब लिखी गई files की सूची देता है (<code>writtenFiles</code>)।',
    'Create a revision Change without placing a Change Marker, or change the ID or description of an existing Change':
        'Change Marker रखे बिना revision Change बनाना, या किसी मौजूदा Change का ID या description बदलना',
    'Create the Change together with its first marker with <a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange). To change an existing Change, use Archicad’s Change Manager.':
        '<a href="../archicad-addon/#CreateChangeMarkers">CreateChangeMarkers</a> (createChange) से Change को उसके पहले marker के साथ बनाएँ। किसी मौजूदा Change को बदलने के लिए Archicad का Change Manager इस्तेमाल करें।',
    'Set the custom scheme fields of a revision Change': 'Revision Change के custom scheme fields सेट करना',
    'Fill them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> reads them back.':
        'इन्हें Archicad के Change Manager में भरें; <a href="../archicad-addon/#GetRevisionChanges">GetRevisionChanges</a> इन्हें वापस पढ़ता है।',
    'Add elements to a revision Change, or remove them from it': 'Revision Change में elements जोड़ना, या उसमें से हटाना',
    'Add them in Archicad’s Change Manager; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> reads which Changes an element belongs to.':
        'इन्हें Archicad के Change Manager में जोड़ें; <a href="../archicad-addon/#GetRevisionChangesOfElements">GetRevisionChangesOfElements</a> पढ़ता है कि कोई element किन Changes से जुड़ा है।',

    // Library parts and GDL
    'Library parts and GDL': 'Library parts और GDL',
    'Run a 2D GDL script given as text, as <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> does with a 3D script':
        'Text के रूप में दी गई 2D GDL script चलाना, जैसे <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> 3D script के साथ करता है',
    'Save the script in a library part with <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (or <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>), then draw its 2D symbol with <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a>.':
        'Script को <a href="../archicad-addon/#CreateLibraryParts">CreateLibraryParts</a> (या <a href="../archicad-addon/#SetLibraryPartScripts">SetLibraryPartScripts</a>) से एक library part में save करें, फिर <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> से उसका 2D symbol बनाएँ।',
    'Get the error message and the line number when a GDL script fails':
        'GDL script fail होने पर error message और line number पाना',
    'Check whether the script runs with <a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) or <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D). To see the error, open the library part in Archicad’s GDL editor with <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> (from Archicad 27).':
        '<a href="../archicad-addon/#InterpretGDLScript3D">InterpretGDLScript3D</a> (3D) या <a href="../archicad-addon/#GetLibraryPart2DPrimitives">GetLibraryPart2DPrimitives</a> (2D) से जाँचें कि script चलती है या नहीं। Error देखने के लिए <a href="../archicad-addon/#OpenLibraryPartInEditor">OpenLibraryPartInEditor</a> से library part को Archicad के GDL editor में खोलें (Archicad 27 से)।',

    // Archicad's interface
    'Archicad\'s interface': 'Archicad का interface',
    'Read the text of the Session Report window': 'Session Report window का text पढ़ना',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'User के लिए window को <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> और windowType \'Report\' से खोलें (Archicad 26 से)।',
    'Get the outcome of an Action Center check': 'Action Center check का नतीजा पाना',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Check को <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> से चलाएँ; कोई समस्या मिलने पर Archicad Action Center दिखाता है। Hotlink check के लिए, हर source की स्थिति <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> से पढ़ें (Archicad 27 से)।',
    'Load a new add-on (.apx or .bundle) while Archicad runs, from an add-on or a script':
        'Archicad चलते समय, किसी add-on या script से नया add-on (.apx या .bundle) load करना',
    'Add it in Archicad\'s Add-On Manager (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add). An add-on that is already loaded is replaced only after Archicad quits, as the Tapir Installer does when it updates Tapir.':
        'इसे Archicad के Add-On Manager में जोड़ें (Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add)। पहले से load हुआ add-on, Archicad बंद होने के बाद ही बदला जाता है, जैसा Tapir Installer Tapir को update करते समय करता है।',

    'Set the profile modifier values, or the stretched width and height, of a wall, beam or column that uses a profile':
        'Profile इस्तेमाल करने वाली wall, beam या column के profile modifier values, या stretched width और height set करना',
    'Read them with <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a>. To change them, change the profile with <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> (a copy for these elements, or the profile itself), or set them in the settings of the element in Archicad.':
        'इन्हें <a href="../archicad-addon/#GetProfileValuesOfElements">GetProfileValuesOfElements</a> से पढ़ें। बदलने के लिए profile को <a href="../archicad-addon/#CreateProfiles">CreateProfiles</a> से बदलें (इन elements के लिए एक copy, या profile खुद), या Archicad में element की settings में set करें।',
    'Read or set how a grid element is shown in sections, elevations and interior elevations: its vertical extent and its markers at the top and bottom':
        'Sections, elevations और interior elevations में grid element कैसे दिखता है, यह पढ़ना या set करना: उसका vertical extent और ऊपर व नीचे के markers',
    'Select the grid elements in Archicad and set it for all of them at once in their settings dialog.':
        'Archicad में grid elements select करें और उनके settings dialog में सभी के लिए एक साथ set करें।',
    'Read the settings of an IFC translator, such as its type mapping and property mapping, for example to check them as the IFC conformity check of the Missing BIM Data report does':
        'किसी IFC ट्रांसलेटर की सेटिंग्स पढ़ना, जैसे उसकी टाइप मैपिंग और प्रॉपर्टी मैपिंग, उदाहरण के लिए उन्हें वैसे जाँचने के लिए जैसे Missing BIM Data रिपोर्ट की IFC अनुरूपता जाँच करती है',
    'Check what the translator gives the elements instead: their IFC type with <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a>, and from Archicad 28 the properties and classification references the preview translator maps to them with <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> and <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a>. <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> shows whether the properties a mapping needs exist in the project.':
        'इसके बजाय जाँचें कि ट्रांसलेटर एलिमेंट्स को क्या देता है: <a href="../archicad-addon/#GetIFCTypeOfElements">GetIFCTypeOfElements</a> से उनका IFC टाइप, और Archicad 28 से <a href="../archicad-addon/#GetIFCPropertiesOfElements">GetIFCPropertiesOfElements</a> और <a href="../archicad-addon/#GetIFCClassificationReferencesOfElements">GetIFCClassificationReferencesOfElements</a> से वे प्रॉपर्टीज़ और क्लासिफ़िकेशन रेफ़रेंस जो प्रीव्यू ट्रांसलेटर उन्हें मैप करता है। किसी मैपिंग को जिन प्रॉपर्टीज़ की ज़रूरत है वे प्रोजेक्ट में हैं या नहीं, यह <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> दिखाता है।',
    'Find the graphic override rules, Interactive Schedules and SAF translators that refer to a property or classification that no longer exists, as the Missing BIM Data report does':
        'वे ग्राफ़िक ओवरराइड नियम, इंटरैक्टिव शेड्यूल और SAF ट्रांसलेटर ढूँढना जो किसी ऐसी प्रॉपर्टी या क्लासिफ़िकेशन का हवाला देते हैं जो अब मौजूद नहीं है, जैसे Missing BIM Data रिपोर्ट करती है',
    'For graphic override rules (from Archicad 27): read the criterion of each rule with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) and compare the properties and classifications it tests with <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> and <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a>. For Interactive Schedules and SAF translators: nothing yet.':
        'ग्राफ़िक ओवरराइड नियमों के लिए (Archicad 27 से): <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> (includeCriterion) से हर नियम की शर्त पढ़ें और वह जिन प्रॉपर्टीज़ और क्लासिफ़िकेशन को जाँचती है उनकी तुलना <a href="../archicad-addon/#GetAllProperties">GetAllProperties</a> और <a href="../archicad-addon/#GetAllClassificationSystems">GetAllClassificationSystems</a> से करें। इंटरैक्टिव शेड्यूल और SAF ट्रांसलेटर के लिए: अभी कुछ नहीं।',
    'Export and import, all at once, the settings that the dialogs of Archicad save as XML (properties, classifications, the Layout Book, graphic overrides, Model View Options, IFC and DWG translators), to keep several projects in sync':
        'वे सेटिंग्स जिन्हें Archicad के डायलॉग XML के रूप में सहेजते हैं (प्रॉपर्टीज़, क्लासिफ़िकेशन, लेआउट बुक, ग्राफ़िक ओवरराइड, मॉडल व्यू ऑप्शंस, IFC और DWG ट्रांसलेटर), एक साथ एक्सपोर्ट और इंपोर्ट करना, ताकि कई प्रोजेक्ट एक जैसे रहें',
    'Copy what the API can write, one kind at a time, in one script run against each project: properties and classifications from their XML files with <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> and <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a>; attributes from another project with <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a>; graphic override rules with <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> and <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a>; Project Info with <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> and <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a>; Favorites with <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> and <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a>. The Layout Book, Model View Options and the translators: nothing yet.':
        'API जो लिख सकता है उसे एक-एक प्रकार करके, हर प्रोजेक्ट पर चलने वाली एक स्क्रिप्ट में कॉपी करें: प्रॉपर्टीज़ और क्लासिफ़िकेशन उनकी XML फ़ाइलों से <a href="../archicad-addon/#ImportPropertiesXml">ImportPropertiesXml</a> और <a href="../archicad-addon/#ImportClassificationsXml">ImportClassificationsXml</a> से; एट्रिब्यूट्स किसी दूसरे प्रोजेक्ट से <a href="../archicad-addon/#ImportAttributesFromProject">ImportAttributesFromProject</a> से; ग्राफ़िक ओवरराइड नियम <a href="../archicad-addon/#GetGraphicOverrideRules">GetGraphicOverrideRules</a> और <a href="../archicad-addon/#CreateGraphicOverrideRules">CreateGraphicOverrideRules</a> से; प्रोजेक्ट इन्फ़ो <a href="../archicad-addon/#GetProjectInfoFields">GetProjectInfoFields</a> और <a href="../archicad-addon/#SetProjectInfoField">SetProjectInfoField</a> से; फ़ेवरिट्स <a href="../archicad-addon/#ExportFavorites">ExportFavorites</a> और <a href="../archicad-addon/#ImportFavorites">ImportFavorites</a> से। लेआउट बुक, मॉडल व्यू ऑप्शंस और ट्रांसलेटर: अभी कुछ नहीं।',


    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'कुछ और मिला जो Archicad API नहीं कर सकता? जाँचे गए DevKit headers के साथ एक issue में बताएँ, और उसे यहाँ जोड़ा जाएगा। <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Issue खोलें</a>'
});
