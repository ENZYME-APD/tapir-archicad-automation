""" Writes a table into a CSV or an Excel (.xlsx) file, with the standard library only. """

import csv
import re
import zipfile
from xml.sax.saxutils import escape

FILE_TYPES = [("Excel workbook", "*.xlsx"), ("CSV (comma separated)", "*.csv")]

_INVALID_XML = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f]")


def write_table(path: str, headers: list[str], rows: list[list], sheet_name: str = "Sheet1") -> None:
    """ An .xlsx path gets an Excel workbook, anything else a UTF-8 CSV that Excel opens as well. """
    if path.lower().endswith(".xlsx"):
        _write_xlsx(path, headers, rows, sheet_name)
    else:
        with open(path, "w", newline="", encoding="utf-8-sig") as file:
            writer = csv.writer(file)
            writer.writerow(headers)
            writer.writerows(rows)


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


def _write_xlsx(path: str, headers: list[str], rows: list[list], sheet_name: str) -> None:
    widths = [max([len(str(h))] + [len(str(r[i])) for r in rows if i < len(r)]) for i, h in enumerate(headers)]
    columns = "".join(f'<col min="{i + 1}" max="{i + 1}" width="{min(60, w + 2)}" customWidth="1"/>'
                      for i, w in enumerate(widths))
    sheet_rows = ['<row r="1">' + "".join(_cell(1, c, h, style=1) for c, h in enumerate(headers)) + "</row>"]
    for r, row in enumerate(rows, start=2):
        sheet_rows.append(f'<row r="{r}">' + "".join(_cell(r, c, v) for c, v in enumerate(row)) + "</row>")
    last = f"{_column(max(0, len(headers) - 1))}{len(rows) + 1}"
    sheet = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
        '<sheetViews><sheetView workbookViewId="0"><pane ySplit="1" topLeftCell="A2" activePane="bottomLeft" state="frozen"/>'
        '</sheetView></sheetViews>'
        f'<cols>{columns}</cols><sheetData>{"".join(sheet_rows)}</sheetData>'
        f'<autoFilter ref="A1:{last}"/></worksheet>')
    sheet_name = escape(re.sub(r"[\[\]:*?/\\]", "", sheet_name)[:31] or "Sheet1")
    parts = {
        "[Content_Types].xml":
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
            '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
            '<Default Extension="xml" ContentType="application/xml"/>'
            '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
            '<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
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
            f'<sheets><sheet name="{sheet_name}" sheetId="1" r:id="rId1"/></sheets>'
            f'<definedNames><definedName name="_xlnm._FilterDatabase" localSheetId="0" hidden="1">'
            f"'{sheet_name}'!$A$1:${_column(max(0, len(headers) - 1))}${len(rows) + 1}</definedName></definedNames>"
            '</workbook>',
        "xl/_rels/workbook.xml.rels":
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>'
            '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>'
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
        "xl/worksheets/sheet1.xml": sheet,
    }
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, content in parts.items():
            archive.writestr(name, content)
