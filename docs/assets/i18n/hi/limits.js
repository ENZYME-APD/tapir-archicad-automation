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
    'Read the slab\'s outline with <a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> and create a static railing along the edge with <a href="../archicad-addon/#CreateRailings">CreateRailings</a>. When the slab changes, delete the railing and create it again.':
        '<a href="../archicad-addon/#GetDetailsOfElements">GetDetailsOfElements</a> से slab की outline पढ़ें और <a href="../archicad-addon/#CreateRailings">CreateRailings</a> से किनारे के साथ एक static railing बनाएँ। जब slab बदले, railing हटाकर फिर से बनाएँ।',

    // Navigator and views
    'Navigator and views': 'Navigator और views',
    'Add an item to a publisher set': 'किसी publisher set में item जोड़ना',
    'Add the items in Archicad\'s Organizer, then publish the set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad के Organizer में items जोड़ें, फिर <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> से set को publish करें।',
    'Change, rename or create a stored Model View Options combination': 'Model View Options का कोई saved combination बदलना, उसका नाम बदलना या नया बनाना',
    'Read the combinations with <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a>; change the options of the current window with <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a>.':
        'Combinations को <a href="../archicad-addon/#GetModelViewOptionDetails">GetModelViewOptionDetails</a> से पढ़ें; मौजूदा window के options <a href="../archicad-addon/#SetViewOptions">SetViewOptions</a> से बदलें।',

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

    // Documents and publishing
    'Documents and publishing': 'Documents और publishing',
    'Read the rows of an Interactive Schedule': 'किसी Interactive Schedule की rows पढ़ना',
    'Build the list yourself: find the elements with <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> and read their values with <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> or <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a>. The <a href="../scripts/">built-in scripts</a> Room Finishes Schedule and Material Take-Off build such lists.':
        'सूची ख़ुद बनाएँ: <a href="../archicad-addon/#GetElementsByType">GetElementsByType</a> से elements ढूँढें और उनकी values <a href="../archicad-addon/#GetPropertyValuesOfElements">GetPropertyValuesOfElements</a> या <a href="../archicad-addon/#GetQuantitiesOfElements">GetQuantitiesOfElements</a> से पढ़ें। <a href="../scripts/">Built-in scripts</a> Room Finishes Schedule और Material Take-Off ऐसी सूचियाँ बनाते हैं।',
    'Export an Interactive Schedule': 'किसी Interactive Schedule को export करना',
    'Put the schedule in a publisher set in Archicad once, then publish that set with <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a>.':
        'Archicad में schedule को एक बार किसी publisher set में डालें, फिर उस set को <a href="../archicad-addon/#PublishPublisherSet">PublishPublisherSet</a> से publish करें।',

    // Archicad's interface
    'Archicad\'s interface': 'Archicad का interface',
    'Read the text of the Session Report window': 'Session Report window का text पढ़ना',
    'Open the window for the user with <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> and windowType \'Report\' (from Archicad 26).':
        'User के लिए window को <a href="../archicad-addon/#ChangeWindow">ChangeWindow</a> और windowType \'Report\' से खोलें (Archicad 26 से)।',
    'Get the outcome of an Action Center check': 'Action Center check का नतीजा पाना',
    'Run the check with <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a>; Archicad shows the Action Center when it found an issue. For the hotlink check, read the state of each source with <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> (from Archicad 27).':
        'Check को <a href="../archicad-addon/#RunActionCenterChecks">RunActionCenterChecks</a> से चलाएँ; कोई समस्या मिलने पर Archicad Action Center दिखाता है। Hotlink check के लिए, हर source की स्थिति <a href="../archicad-addon/#GetHotlinkSourceStatus">GetHotlinkSourceStatus</a> से पढ़ें (Archicad 27 से)।',

    'Found something else the Archicad API cannot do? Say so in an issue, with the DevKit headers you checked, and it is added here. <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Open an issue</a>':
        'कुछ और मिला जो Archicad API नहीं कर सकता? जाँचे गए DevKit headers के साथ एक issue में बताएँ, और उसे यहाँ जोड़ा जाएगा। <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new/choose">Issue खोलें</a>'
});
