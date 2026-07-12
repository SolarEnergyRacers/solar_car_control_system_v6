#!/usr/bin/env python3
"""Generate a browser-based preview for the DriverDisplay layout.

The script parses the display-related constants from the C++ sources in this
project and creates a self-contained HTML file that lets you tweak the values
and immediately see the layout change in a simple display mock-up.

Usage:
  python3 tools/driver_display_layout_preview.py
  python3 tools/driver_display_layout_preview.py --serve
"""

from __future__ import annotations

import argparse
import ast
import http.server
import json
import os
import re
import socketserver
import sys
import textwrap
from pathlib import Path
from html import escape
from typing import Dict, List, Tuple


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "tools" / "driver_display_layout_preview.html"

SOURCES = [
    ROOT / "lib/DriverDisplay/DriverDisplay.h",
    ROOT / "lib/DriverDisplay/DriverDisplay.cpp",
    ROOT / "lib/Display/Display.h",
    ROOT / "lib/Display/Display.cpp",
]

DISPLAY_DEFAULTS = {
    "width": 320,
    "height": 240,
    "lifeSignRadius": 4,
}


def should_include(name: str) -> bool:
    lowered = name.lower()
    if any(token in lowered for token in ["frame", "lifesign", "height", "width", "radius", "textsize", "text"]):
        return not any(token in lowered for token in ["last", "color", "bg", "cache", "value", "label"])
    return False


def parse_assignments(path: Path, source_index: int) -> List[Tuple[int, int, str, str]]:
    entries: List[Tuple[int, int, str, str]] = []
    text = path.read_text(encoding="utf-8", errors="ignore")
    for lineno, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if not stripped or stripped.startswith("//") or stripped.startswith("/*"):
            continue
        m = re.match(r"^(?:const\s+)?(?:int|float|double)\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^;]+);", stripped)
        if m:
            name, expr = m.group(1), m.group(2).strip()
            if should_include(name):
                entries.append((source_index, lineno, name, expr))
        else:
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^;]+);", stripped)
            if m:
                name, expr = m.group(1), m.group(2).strip()
                if should_include(name):
                    entries.append((source_index, lineno, name, expr))
    return entries


def parse_display_value_positions(path: Path, source_index: int) -> List[Tuple[int, int, str, int, int]]:
    entries: List[Tuple[int, int, str, int, int]] = []
    text = path.read_text(encoding="utf-8", errors="ignore")
    for lineno, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if not stripped or stripped.startswith("//"):
            continue
        m = re.match(r"^\s*DisplayValue<[^>]+>\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*DisplayValue<[^>]+>\(([^;]+)\);", stripped)
        if not m:
            continue
        args = [arg.strip() for arg in m.group(2).split(",")]
        if len(args) < 2:
            continue
        try:
            x = int(float(args[0]))
            y = int(float(args[1]))
        except ValueError:
            continue
        entries.append((source_index, lineno, m.group(1), x, y))
    return entries


class EvalContext(dict):
    def __init__(self, initial: Dict[str, float] | None = None):
        super().__init__(initial or {})

    def __missing__(self, key: str):
        if key in {"display", "carState", "tft", "spiBus", "console"}:
            return 0
        if key in {"width", "height"}:
            return DISPLAY_DEFAULTS.get(key, 0)
        return 0


def eval_expression(expression: str, values: Dict[str, float]) -> float | None:
    text = expression.strip()
    if not text:
        return None
    # Remove trailing comments, common in C++ examples.
    text = re.sub(r"//.*$", "", text).strip()
    text = text.replace("&&", " and ").replace("||", " or ")
    text = re.sub(r"\b([A-Za-z_][A-Za-z0-9_]*)\b", lambda m: m.group(1) if m.group(1) in values else m.group(1), text)

    # Replace display.* and carState.* access with a simple default screen size.
    text = text.replace("display.width", str(DISPLAY_DEFAULTS["width"]))
    text = text.replace("display.height", str(DISPLAY_DEFAULTS["height"]))
    text = text.replace("tft->height()", str(DISPLAY_DEFAULTS["height"]))
    text = text.replace("tft->width()", str(DISPLAY_DEFAULTS["width"]))
    text = text.replace("carState.DriverDisplayInfoFrameY", "0")
    text = text.replace("carState.DriverDisplayDataFrameY", "0")

    # Replace common names with their current values if known.
    def replace_name(match: re.Match[str]) -> str:
        name = match.group(1)
        if name in values:
            return str(values[name])
        return match.group(1)

    text = re.sub(r"\b([A-Za-z_][A-Za-z0-9_]*)\b", replace_name, text)

    try:
        node = ast.parse(text, mode="eval")
    except SyntaxError:
        return None

    def _eval(node: ast.AST):
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return float(node.value)
        if isinstance(node, ast.Name):
            return float(values.get(node.id, 0))
        if isinstance(node, ast.UnaryOp):
            if isinstance(node.op, ast.UAdd):
                return +_eval(node.operand)
            if isinstance(node.op, ast.USub):
                return -_eval(node.operand)
        if isinstance(node, ast.BinOp):
            left = _eval(node.left)
            right = _eval(node.right)
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            if isinstance(node.op, ast.Mult):
                return left * right
            if isinstance(node.op, ast.Div):
                return left / right
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
            func = node.func.id
            args = [_eval(arg) for arg in node.args]
            if func == "abs":
                return abs(args[0])
            if func == "max":
                return max(args)
            if func == "min":
                return min(args)
            if func == "round":
                return round(args[0])
        raise ValueError(f"unsupported expression: {ast.dump(node)}")

    try:
        return _eval(node.body)
    except Exception:
        return None


def collect_values() -> Tuple[Dict[str, float], List[Tuple[str, int, int]]]:
    values: Dict[str, float] = DISPLAY_DEFAULTS.copy()
    entries: List[Tuple[int, int, str, str]] = []

    for source_index, path in enumerate(SOURCES):
        if not path.exists():
            continue
        entries.extend(parse_assignments(path, source_index))

    display_entries = parse_display_value_positions(ROOT / "lib/DriverDisplay/DriverDisplay.h", 0)
    entries.sort(key=lambda item: (item[0], item[1]))
    for _, _, name, expr in entries:
        if name in values and expr == values[name]:
            continue
        if not should_include(name):
            continue
        parsed = eval_expression(expr, values)
        if parsed is not None:
            values[name] = parsed

    display_items = [(name, x, y) for _, _, name, x, y in display_entries]
    return values, display_items


def render_html(values: Dict[str, float], display_items: List[Tuple[str, int, int]]) -> str:
    entries = []
    for name in sorted(values):
        if name in {"width", "height", "lifesignradius"}:
            continue
        default_value = values[name]
        label = name.replace("Frame", " Frame ").replace("TextSize", " Text Size ").replace("info", "info ").replace("main", "main ")
        label = re.sub(r"(?<!^)(?=[A-Z])", " ", label).replace("_", " ").strip()
        entries.append((name, label, default_value))

    # Keep a sensible ordering for the preview controls.
    preferred_order = [
        "infoFrameY", "infoFrameSizeY", "infoTextSize", "dataFrameY", "speedFrameX", "speedFrameY", "speedFrameSizeX",
        "speedFrameSizeY", "speedTextSize", "targetValueFrameX", "targetValueFrameY", "targetValueFrameSizeX",
        "targetValueFrameSizeY", "targetValueTextSize", "accFrameX", "accFrameY", "accFrameSizeX", "accFrameSizeY",
        "accTextSize", "batFrameY", "pvFrameY", "motorFrameY", "constantModeY", "controlModeStepY", "driveDirectionY",
        "lightY", "ecoPwrModeY", "lifeSignRadius", "width", "height"
    ]
    ordered_entries = []
    seen = set()
    for name in preferred_order:
        if name in values:
            item = next((item for item in entries if item[0] == name), (name, name.replace("_", " "), values[name]))
            ordered_entries.append(item)
            seen.add(name)
    for item in entries:
        if item[0] not in seen:
            ordered_entries.append(item)
            seen.add(item[0])

    field_markup = []
    for name, label, default_value in ordered_entries:
        field_markup.append(
            f"<label class='field'><span>{escape(label)}</span><input type='number' data-name='{escape(name)}' value='{int(default_value) if float(default_value).is_integer() else default_value}' /></label>"
        )

    return f"""<!doctype html>
<html lang='en'>
<head>
  <meta charset='utf-8' />
  <meta name='viewport' content='width=device-width, initial-scale=1' />
  <title>Driver Display Layout Preview</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 0; background: #111827; color: #f3f4f6; }}
    .page {{ display: grid; grid-template-columns: 360px 1fr; gap: 24px; padding: 20px; }}
    .panel {{ background: #1f2937; border: 1px solid #374151; border-radius: 12px; padding: 16px; }}
    h1 {{ margin-top: 0; font-size: 1.2rem; }}
    .form {{ display: grid; gap: 10px; max-height: 80vh; overflow: auto; }}
    .field {{ display: grid; grid-template-columns: 1fr 90px; gap: 8px; align-items: center; font-size: 0.9rem; }}
    .field input {{ width: 100%; padding: 6px; border-radius: 6px; border: 1px solid #6b7280; background: #111827; color: #f9fafb; }}
    .preview {{ display: flex; justify-content: center; align-items: flex-start; }}
    canvas {{ border: 2px solid #9ca3af; border-radius: 12px; background: #000; max-width: 100%; }}
    .hint {{ font-size: 0.85rem; color: #9ca3af; margin-top: 10px; }}
  </style>
</head>
<body>
  <div class='page'>
    <div class='panel'>
      <h1>DriverDisplay layout</h1>
      <div class='form'>
        {''.join(field_markup)}
      </div>
      <div class='hint'>Edit the values and the preview updates instantly. The defaults are parsed from the C++ sources in this workspace.</div>
    </div>
    <div class='panel preview'>
      <canvas id='preview' width='320' height='240'></canvas>
    </div>
  </div>
  <script>
    const canvas = document.getElementById('preview');
    const ctx = canvas.getContext('2d');
    const inputs = Array.from(document.querySelectorAll('input[data-name]'));

    const defaults = {json.dumps({name: values[name] for name, _, _ in ordered_entries}, indent=2)};
    const displayValues = {json.dumps(display_items)};

    function getValues() {{
      const result = {{}};
      inputs.forEach((input) => {{ result[input.dataset.name] = Number(input.value); }});
      return result;
    }}

    function draw(values) {{
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = '#000';
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      const width = values.width || 320;
      const height = values.height || 240;
      const speedFrameX = values.speedFrameX >= 0 ? values.speedFrameX : Math.floor((width - (values.speedFrameSizeX || 156)) / 2);
      const speedFrameY = values.speedFrameY || 16;
      const speedFrameSizeX = values.speedFrameSizeX || 156;
      const speedFrameSizeY = values.speedFrameSizeY || 76;
      const accFrameSizeX = values.accFrameSizeX >= 0 ? values.accFrameSizeX : Math.max(8, speedFrameX - 3);
      const accFrameY = values.accFrameY || 54;
      const accFrameSizeY = values.accFrameSizeY || 38;
      const targetValueFrameX = values.targetValueFrameX >= 0 ? values.targetValueFrameX : speedFrameX + speedFrameSizeX + 1;
      const targetValueFrameY = values.targetValueFrameY || 54;
      const targetValueFrameSizeX = values.targetValueFrameSizeX >= 0 ? values.targetValueFrameSizeX : width - accFrameSizeX - speedFrameSizeX - 6;
      const targetValueFrameSizeY = values.targetValueFrameSizeY || 38;
      const infoFrameY = values.infoFrameY >= 0 ? values.infoFrameY : height - (values.infoFrameSizeY || 64);
      const infoFrameSizeY = values.infoFrameSizeY || 64;
      const dataFrameY = values.dataFrameY || 0;
      ctx.save();
      ctx.scale(canvas.width / width, canvas.height / height);

      // outer display frame
      ctx.strokeStyle = '#00ff00';
      ctx.lineWidth = 2;
      ctx.strokeRect(0, 0, width, height);

      // main content frame
      ctx.strokeStyle = '#22c55e';
      ctx.strokeRect(0, dataFrameY, width, height - dataFrameY - infoFrameSizeY - 2);

      // info frame
      ctx.fillStyle = '#111827';
      ctx.fillRect(0, infoFrameY, width, infoFrameSizeY);
      ctx.strokeStyle = '#38bdf8';
      ctx.strokeRect(0, infoFrameY, width, infoFrameSizeY);

      // speed frame
      ctx.strokeStyle = '#facc15';
      ctx.strokeRect(speedFrameX, speedFrameY, speedFrameSizeX, speedFrameSizeY);
      ctx.fillStyle = '#facc15';
      ctx.fillRect(speedFrameX, speedFrameY, 6, 6);

      // acceleration frame
      ctx.strokeStyle = '#f59e0b';
      ctx.strokeRect(values.accFrameX || 2, accFrameY, accFrameSizeX, accFrameSizeY);

      // target value frame
      ctx.strokeStyle = '#f59e0b';
      ctx.strokeRect(targetValueFrameX, targetValueFrameY, targetValueFrameSizeX, targetValueFrameSizeY);

      // value displays from DriverDisplay.h
      ctx.fillStyle = '#ffffff';
      ctx.font = '11px sans-serif';
      displayValues.forEach((item) => {{
        const name = item[0];
        const x = item[1];
        const y = item[2];
        const label = name === 'TargetSpeedPower' ? 'Target' :
          name === 'Speed' ? 'Speed00' :
          name === 'Acceleration99' ? 'Accel' :
          name === 'MotorCurrent' ? 'MotorC:' :
          name === 'MotorOn' ? 'MotorOn' :
          name === 'BatteryVoltage' ? 'Bat :' :
          name === 'BatteryOn' ? 'BatOn' :
          name === 'PhotoVoltaicCurrent' ? 'PV :' :
          name === 'PhotoVoltaicOn' ? 'PVOn' :
          name === 'DriverInfo' ? 'Info' :
          name === 'DateTimeStamp' ? 'Time' :
          name === 'StepSize' ? 'Step' :
          name;
        const width = label.length * 6 + 8;
        ctx.strokeStyle = '#60a5fa';
        ctx.strokeRect(x, y, Math.max(56, width), 16);
        ctx.fillText(label, x + 4, y + 12);
      }});

      // status boxes
      ctx.fillStyle = '#ffffff';
      ctx.font = '14px sans-serif';
      ctx.fillText('Speed', 20, 40);
      ctx.fillText('Target', 210, 40);
      ctx.fillText('Motor', 12, 120);
      ctx.fillText('Battery', 12, 140);
      ctx.fillText('PV', 12, 160);

      // life sign
      ctx.fillStyle = '#16a34a';
      ctx.beginPath();
      ctx.arc(values.lifeSignX || (width - (values.lifeSignRadius || 4) - 4), values.lifeSignY || (height - (values.lifeSignRadius || 4) - 4), values.lifeSignRadius || 4, 0, Math.PI * 2);
      ctx.fill();

      ctx.restore();
    }}

    inputs.forEach((input) => input.addEventListener('input', () => draw(getValues())));
    draw(defaults);
  </script>
</body>
</html>
"""


def write_output(html: str) -> None:
    OUTPUT.write_text(html, encoding="utf-8")
    print(f"Wrote {OUTPUT}")


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, directory: str | None = None, **kwargs):
        super().__init__(*args, directory=directory, **kwargs)

    def log_message(self, format: str, *args) -> None:  # noqa: A003
        return


def serve(output_html: Path, port: int = 8000) -> None:
    handler = lambda *args, **kwargs: QuietHandler(*args, directory=str(output_html.parent), **kwargs)
    with socketserver.TCPServer(("127.0.0.1", port), handler) as httpd:
        print(f"Serving preview at http://127.0.0.1:{port}/{output_html.name}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nStopped preview server.")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate a DriverDisplay layout preview")
    parser.add_argument("--serve", action="store_true", help="Serve the generated HTML locally")
    parser.add_argument("--port", type=int, default=8000, help="Port for the local preview server")
    args = parser.parse_args()

    values, display_items = collect_values()
    html = render_html(values, display_items)
    write_output(html)

    if args.serve:
        serve(OUTPUT, port=args.port)
    else:
        print("Open the generated HTML in a browser to adjust the layout values.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
