#include "ScriptInfo.hpp"
#include "File.hpp"
#include "MigrationHelper.hpp"

#include <vector>

namespace {

constexpr USize  MaxReadSize       = 256 * 1024;   // the docstring and the header are at the top of the file
constexpr size_t MaxTitleLength    = 80;
constexpr size_t MaxSummaryLength  = 600;

bool IsBlank (char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

std::string Trim (const std::string& s)
{
    size_t begin = 0;
    while (begin < s.size () && IsBlank (s[begin])) {
        ++begin;
    }
    size_t end = s.size ();
    while (end > begin && IsBlank (s[end - 1])) {
        --end;
    }
    return s.substr (begin, end - begin);
}

std::string TrimRight (const std::string& s)
{
    size_t end = s.size ();
    while (end > 0 && IsBlank (s[end - 1])) {
        --end;
    }
    return s.substr (0, end);
}

std::vector<std::string> SplitLines (const std::string& text)
{
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= text.size ()) {
        size_t end = text.find ('\n', start);
        if (end == std::string::npos) {
            end = text.size ();
        }
        std::string line = text.substr (start, end - start);
        if (!line.empty () && line.back () == '\r') {
            line.pop_back ();
        }
        lines.push_back (line);
        start = end + 1;
    }
    return lines;
}

// Python reads source files as UTF-8, so a file that is not valid UTF-8 does not run; its text is not shown either.
bool IsValidUTF8 (const std::string& s)
{
    size_t i = 0;
    while (i < s.size ()) {
        const unsigned char c = static_cast<unsigned char> (s[i]);
        size_t seqLen = 0;
        unsigned char secondMin = 0x80;
        unsigned char secondMax = 0xBF;
        if (c < 0x80) {
            seqLen = 1;
        } else if (c >= 0xC2 && c <= 0xDF) {
            seqLen = 2;
        } else if (c >= 0xE0 && c <= 0xEF) {
            seqLen = 3;
            if (c == 0xE0) {
                secondMin = 0xA0;
            } else if (c == 0xED) {
                secondMax = 0x9F;
            }
        } else if (c >= 0xF0 && c <= 0xF4) {
            seqLen = 4;
            if (c == 0xF0) {
                secondMin = 0x90;
            } else if (c == 0xF4) {
                secondMax = 0x8F;
            }
        }
        if (seqLen == 0 || i + seqLen > s.size ()) {
            return false;
        }
        for (size_t j = 1; j < seqLen; ++j) {
            const unsigned char next = static_cast<unsigned char> (s[i + j]);
            if (j == 1 ? (next < secondMin || next > secondMax) : ((next & 0xC0) != 0x80)) {
                return false;
            }
        }
        i += seqLen;
    }
    return true;
}

// Cuts s to at most maxLength bytes without splitting a UTF-8 sequence, adding an ellipsis when cut.
std::string Shorten (const std::string& s, size_t maxLength)
{
    if (s.size () <= maxLength) {
        return s;
    }
    size_t end = maxLength;
    while (end > 0 && (static_cast<unsigned char> (s[end]) & 0xC0) == 0x80) {
        --end;
    }
    return TrimRight (s.substr (0, end)) + "\xE2\x80\xA6";
}

GS::UniString ToUniString (const std::string& s)
{
    return GS::UniString (s.c_str (), static_cast<GS::USize> (s.size ()), CC_UTF8);
}

bool IsRestComment (const std::string& s, size_t from)
{
    const std::string rest = from < s.size () ? Trim (s.substr (from)) : std::string ();
    return rest.empty () || rest[0] == '#';
}

// A TOML basic ("...") or literal ('...') string on one line, optionally followed by a comment.
bool ParseTomlString (const std::string& value, std::string& result)
{
    if (value.empty () || (value[0] != '"' && value[0] != '\'')) {
        return false;
    }
    const char quote = value[0];
    if (value.compare (0, 3, std::string (3, quote)) == 0) {
        return false;   // multi-line strings are not needed for these keys
    }

    std::string out;
    size_t i = 1;
    for (; i < value.size () && value[i] != quote; ++i) {
        if (value[i] == '\\' && quote == '"') {
            if (++i >= value.size ()) {
                return false;
            }
            switch (value[i]) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                default:   return false;
            }
            continue;
        }
        out += value[i];
    }
    if (i >= value.size () || !IsRestComment (value, i + 1)) {
        return false;
    }
    result = out;
    return true;
}

// A TOML array of non-negative integers, e.g. [28, 29, 30], optionally followed by a comment.
bool ParseTomlIntegerArray (const std::string& value, GS::Array<Int32>& result)
{
    if (value.empty () || value[0] != '[') {
        return false;
    }
    const size_t close = value.find (']');
    if (close == std::string::npos || !IsRestComment (value, close + 1)) {
        return false;
    }

    GS::Array<Int32> out;
    const std::string inner = value.substr (1, close - 1);
    size_t start = 0;
    while (start <= inner.size ()) {
        size_t comma = inner.find (',', start);
        if (comma == std::string::npos) {
            comma = inner.size ();
        }
        const std::string item = Trim (inner.substr (start, comma - start));
        if (item.empty ()) {
            if (comma != inner.size ()) {
                return false;   // only the last item may be empty: "[]" or a trailing comma
            }
        } else {
            if (item.size () > 9) {
                return false;
            }
            Int32 number = 0;
            for (char c : item) {
                if (c < '0' || c > '9') {
                    return false;
                }
                number = number * 10 + (c - '0');
            }
            out.Push (number);
        }
        start = comma + 1;
    }
    result = out;
    return true;
}

// The `# /// tapir` block as in PEP 723: an opening line, lines that are "#" or start with "# ", and a closing "# ///".
bool ParseHeader (const std::vector<std::string>& lines, ScriptInfo& info)
{
    for (size_t i = 0; i < lines.size (); ++i) {
        if (TrimRight (lines[i]) != "# /// tapir") {
            continue;
        }

        std::vector<std::string> content;
        bool closed = false;
        for (size_t j = i + 1; j < lines.size (); ++j) {
            const std::string line = TrimRight (lines[j]);
            if (line == "# ///") {
                closed = true;
                break;
            }
            if (line == "#") {
                continue;
            }
            if (line.compare (0, 2, "# ") != 0) {
                break;
            }
            content.push_back (line.substr (2));
        }
        if (!closed) {
            return false;
        }

        for (size_t k = 0; k < content.size (); ++k) {
            const std::string line = Trim (content[k]);
            if (line.empty () || line[0] == '#') {
                continue;
            }
            const size_t eq = line.find ('=');
            if (eq == std::string::npos) {
                continue;
            }
            const std::string key = Trim (line.substr (0, eq));
            const std::string value = Trim (line.substr (eq + 1));

            std::string text;
            if (key == "category") {
                if (ParseTomlString (value, text)) {
                    info.category = ToUniString (Trim (text));
                }
            } else if (key == "archicad-versions") {
                // TOML lets an array span lines. Its items are integers here, so a '#' always starts a comment, and
                // the lines it spans have no '='.
                std::string array = value.substr (0, value.find ('#'));
                for (size_t next = k + 1; array.find (']') == std::string::npos && next < content.size () && content[next].find ('=') == std::string::npos; ++next) {
                    array += ' ' + content[next].substr (0, content[next].find ('#'));
                }
                ParseTomlIntegerArray (Trim (array), info.archicadVersions);
            } else if (key == "min-tapir-version") {
                if (ParseTomlString (value, text)) {
                    info.minTapirVersion = ToUniString (Trim (text));
                }
            } else if (key == "changes") {
                if (ParseTomlString (value, text) && (text == "model" || text == "files" || text == "read-only")) {
                    info.changes = ToUniString (text);
                }
            }
        }
        info.hasHeader = true;
        return true;
    }
    return false;
}

// The module docstring: the first statement of the file when it is a string (comments, blank lines and the metadata
// blocks before it are skipped). In a string without the r prefix a backslash at the end of a line joins the next
// line, and \\, \" and \' are unescaped; an escaped quote never ends the string.
bool FindDocstring (const std::vector<std::string>& lines, std::string& docstring)
{
    std::string rest;
    size_t i = 0;
    for (; i < lines.size (); ++i) {
        const std::string line = Trim (lines[i]);
        if (line.empty () || line[0] == '#') {
            continue;
        }
        rest = line;
        break;
    }
    if (i >= lines.size ()) {
        return false;
    }

    size_t q = 0;
    bool isRaw = false;
    while (q < rest.size () && q < 2 && (rest[q] == 'r' || rest[q] == 'R' || rest[q] == 'u' || rest[q] == 'U')) {
        isRaw = isRaw || rest[q] == 'r' || rest[q] == 'R';
        ++q;
    }
    if (q >= rest.size () || (rest[q] != '"' && rest[q] != '\'')) {
        return false;
    }
    const bool isTripleQuoted = rest.compare (q, 3, std::string (3, rest[q])) == 0;
    const std::string delimiter = isTripleQuoted ? rest.substr (q, 3) : rest.substr (q, 1);

    std::string out;
    std::string line = rest.substr (q + delimiter.size ());
    for (size_t j = i; ; ) {
        bool joinsNextLine = false;
        for (size_t k = 0; k < line.size (); ++k) {
            if (line.compare (k, delimiter.size (), delimiter) == 0) {
                docstring = out;
                return true;
            }
            if (line[k] != '\\') {
                out += line[k];
            } else if (k + 1 >= line.size ()) {
                joinsNextLine = true;
                if (isRaw) {
                    out += '\\';
                }
            } else {
                const char next = line[++k];
                if (isRaw || (next != '\\' && next != '"' && next != '\'')) {
                    out += '\\';
                }
                out += next;
            }
        }
        if (++j >= lines.size () || (!isTripleQuoted && !joinsNextLine)) {
            return false;   // the string does not end
        }
        if (!joinsNextLine || isRaw) {
            out += '\n';
        }
        line = lines[j];
    }
}

void ParseDocstring (const std::string& docstring, ScriptInfo& info)
{
    std::vector<std::vector<std::string>> paragraphs;
    std::vector<std::string> current;
    for (const std::string& rawLine : SplitLines (docstring)) {
        const std::string line = Trim (rawLine);
        if (line.empty ()) {
            if (!current.empty ()) {
                paragraphs.push_back (current);
                current.clear ();
            }
        } else {
            current.push_back (line);
        }
    }
    if (!current.empty ()) {
        paragraphs.push_back (current);
    }
    if (paragraphs.empty ()) {
        return;
    }

    auto join = [] (const std::vector<std::string>& paragraph) {
        std::string result;
        for (const std::string& line : paragraph) {
            if (!result.empty ()) {
                result += ' ';
            }
            result += line;
        }
        return result;
    };

    size_t summaryIndex = 0;
    const std::vector<std::string>& first = paragraphs[0];
    if (paragraphs.size () > 1 && first.size () == 1 && first[0].size () <= MaxTitleLength && first[0].back () != '.') {
        info.title = ToUniString (first[0]);
        summaryIndex = 1;
    }
    info.summary = ToUniString (Shorten (join (paragraphs[summaryIndex]), MaxSummaryLength));
}

} // namespace

ScriptInfo ParseScriptInfo (const std::string& utf8Text)
{
    ScriptInfo info;

    std::string text = utf8Text;
    if (text.compare (0, 3, "\xEF\xBB\xBF") == 0) {
        text.erase (0, 3);
    }
    if (!IsValidUTF8 (text)) {
        return info;
    }

    const std::vector<std::string> lines = SplitLines (text);
    ParseHeader (lines, info);

    std::string docstring;
    if (FindDocstring (lines, docstring)) {
        ParseDocstring (docstring, info);
    }
    return info;
}

ScriptInfo ReadScriptInfo (const IO::Location& scriptFile)
{
    IO::Name name;
    if (scriptFile.GetLastLocalName (&name) != NoError) {
        return {};
    }
    if (name.GetExtension ().Compare ("py", CaseInsensitive) != GS::UniString::Equal) {
        return {};
    }

    IO::File file (scriptFile, IO::File::OnNotFound::Fail);
    if (file.GetStatus () != NoError || file.Open (IO::File::ReadMode) != NoError) {
        return {};
    }

    std::string content;
    USize size = 0;
    if (file.GetDataLength (&size) == NoError && size > 0) {
        content.resize (size < MaxReadSize ? size : MaxReadSize);
        USize readSize = 0;
        if (file.ReadBin (&content[0], static_cast<USize> (content.size ()), &readSize) != NoError) {
            readSize = 0;
        }
        content.resize (readSize);
        if (size > MaxReadSize) {
            // Drops the last character when it is not ASCII: it may be cut. Only the top of the file matters.
            size_t end = content.size ();
            while (end > 0 && (static_cast<unsigned char> (content[end - 1]) & 0xC0) == 0x80) {
                --end;
            }
            if (end > 0 && (static_cast<unsigned char> (content[end - 1]) & 0x80) != 0) {
                --end;
            }
            content.resize (end);
        }
    }
    file.Close ();

    return ParseScriptInfo (content);
}
