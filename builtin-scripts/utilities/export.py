""" Writes a table into a CSV or an Excel (.xlsx) file, or several tables into the sheets of one
Excel workbook, with the standard library only. """

import csv
import re
import zipfile
from xml.sax.saxutils import escape

FILE_TYPES = [("Excel workbook", "*.xlsx"), ("CSV (comma separated)", "*.csv")]

_INVALID_XML = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f]")
_INVALID_SHEET_NAME = re.compile(r"[\[\]:*?/\\]")


def write_table(path: str, headers: list[str], rows: list[list], sheet_name: str = "Sheet1") -> None:
    """ An .xlsx path gets an Excel workbook, anything else a UTF-8 CSV that Excel opens as well. """
    if path.lower().endswith(".xlsx"):
        write_workbook(path, [(sheet_name, headers, rows)])
    else:
        with open(path, "w", newline="", encoding="utf-8-sig") as file:
            writer = csv.writer(file)
            writer.writerow(headers)
            writer.writerows(rows)


def write_workbook(path: str, sheets: list[tuple[str, list[str], list[list]]]) -> None:
    """ An Excel workbook (.xlsx) with one sheet per (sheet name, headers, rows), in that order.
    The names are cut to Excel's 31 characters and made unique within the workbook. """
    names = _sheet_names([name for name, _headers, _rows in sheets])
    parts = {
        "[Content_Types].xml":
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
            '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
            '<Default Extension="xml" ContentType="application/xml"/>'
            '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
            + "".join(f'<Override PartName="/xl/worksheets/sheet{i + 1}.xml" '
                      'ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
                      for i in range(len(sheets))) +
            '<Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>'
            '</Types>',
        "_rels/.rels":
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>'
            '</Relationships>',
        "xl/workbook.xml":
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
            'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
            '<sheets>'
            + "".join(f'<sheet name="{escape(name, {chr(34): "&quot;"})}" sheetId="{i + 1}" r:id="rId{i + 1}"/>' for i, name in enumerate(names)) +
            '</sheets><definedNames>'
            + "".join(f'<definedName name="_xlnm._FilterDatabase" localSheetId="{i}" hidden="1">'
                      f"'{escape(name.replace(chr(39), chr(39) * 2))}'!$A$1:${_column(max(0, len(headers) - 1))}${len(rows) + 1}"
                      '</definedName>'
                      for i, (name, (_name, headers, rows)) in enumerate(zip(names, sheets))) +
            '</definedNames></workbook>',
        "xl/_rels/workbook.xml.rels":
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            + "".join(f'<Relationship Id="rId{i + 1}" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" '
                      f'Target="worksheets/sheet{i + 1}.xml"/>' for i in range(len(sheets))) +
            f'<Relationship Id="rId{len(sheets) + 1}" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>'
            '</Relationships>',
        "xl/styles.xml":
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
            '<fonts count="2"><font><sz val="11"/><name val="Calibri"/></font><font><b/><sz val="11"/><name val="Calibri"/></font></fonts>'
            '<fills count="2"><fill><patternFill patternType="none"/></fill><fill><patternFill patternType="gray125"/></fill></fills>'
            '<borders count="1"><border><left/><right/><top/><bottom/><diagonal/></border></borders>'
            '<cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>'
            '<cellXfs count="2"><xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>'
            '<xf numFmtId="0" fontId="1" fillId="0" borderId="0" xfId="0" applyFont="1"/></cellXfs>'
            '<cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles>'
            '</styleSheet>',
    }
    for i, (_name, headers, rows) in enumerate(sheets):
        parts[f"xl/worksheets/sheet{i + 1}.xml"] = _sheet_xml(headers, rows)
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, content in parts.items():
            archive.writestr(name, content)


def _sheet_names(wanted: list[str]) -> list[str]:
    """ Excel's rules: at most 31 characters, none of []:*?/\\, no ' at either end, unique ignoring case. """
    names: list[str] = []
    for index, name in enumerate(wanted):
        base = _INVALID_SHEET_NAME.sub("", _INVALID_XML.sub("", str(name))).strip().strip("'")[:31] or f"Sheet{index + 1}"
        unique, number = base, 2
        while unique.lower() in (n.lower() for n in names):
            suffix = f" ({number})"
            unique, number = base[:31 - len(suffix)].rstrip("'") + suffix, number + 1
        names.append(unique)
    return names


def _column(index: int) -> str:
    name = ""
    index += 1
    while index:
        index, remainder = divmod(index - 1, 26)
        name = chr(65 + remainder) + name
    return name


def _cell(row: int, column: int, value, style: int = 0) -> str:
    ref = f"{_column(column)}{row}"
    style_attr = f' s="{style}"' if style else ""
    if isinstance(value, bool) or value is None:
        value = "" if value is None else str(value)
    if isinstance(value, (int, float)):
        return f'<c r="{ref}"{style_attr}><v>{value}</v></c>'
    text = escape(_INVALID_XML.sub("", str(value)))
    return f'<c r="{ref}" t="inlineStr"{style_attr}><is><t xml:space="preserve">{text}</t></is></c>'


def _sheet_xml(headers: list[str], rows: list[list]) -> str:
    widths = [max([len(str(h))] + [len(str(r[i])) for r in rows if i < len(r)]) for i, h in enumerate(headers)]
    columns = "".join(f'<col min="{i + 1}" max="{i + 1}" width="{min(60, w + 2)}" customWidth="1"/>'
                      for i, w in enumerate(widths))
    sheet_rows = ['<row r="1">' + "".join(_cell(1, c, h, style=1) for c, h in enumerate(headers)) + "</row>"]
    for r, row in enumerate(rows, start=2):
        sheet_rows.append(f'<row r="{r}">' + "".join(_cell(r, c, v) for c, v in enumerate(row)) + "</row>")
    last = f"{_column(max(0, len(headers) - 1))}{len(rows) + 1}"
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
        '<sheetViews><sheetView workbookViewId="0"><pane ySplit="1" topLeftCell="A2" activePane="bottomLeft" state="frozen"/>'
        '</sheetView></sheetViews>'
        + (f'<cols>{columns}</cols>' if columns else "") +
        f'<sheetData>{"".join(sheet_rows)}</sheetData>'
        f'<autoFilter ref="A1:{last}"/></worksheet>')
