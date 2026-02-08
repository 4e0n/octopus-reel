#!/usr/bin/env python3
import os
import re
import time
import html
import shutil
import subprocess
from pathlib import Path
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import unquote

BASE = Path("/opt/octopus/stor/heeg").resolve()
ZIP_CMD = shutil.which("zip") or "/usr/bin/zip"
HOST = "0.0.0.0"
PORT = 8081

SAFE_NAME = re.compile(r"^[A-Za-z0-9._-]+$")

def fmt_time(ts: float) -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(ts))

def human_size(n: int) -> str:
    # Simple human-readable size
    units = ["B", "KB", "MB", "GB", "TB"]
    f = float(n)
    i = 0
    while f >= 1024.0 and i < len(units) - 1:
        f /= 1024.0
        i += 1
    if i == 0:
        return f"{int(f)} {units[i]}"
    return f"{f:.1f} {units[i]}"

def session_files(session_dir: Path):
    # list files only in the session root (not recursive)
    files = []
    total = 0
    try:
        for p in session_dir.iterdir():
            if p.is_file():
                try:
                    st = p.stat()
                    files.append((p.name, st.st_size, st.st_mtime))
                    total += st.st_size
                except FileNotFoundError:
                    pass
    except FileNotFoundError:
        pass
    files.sort(key=lambda x: x[0].lower())
    return files, total

def session_status(session_name: str, files):
    """
    Minimal 'completeness' heuristic:
    - BrainVision trio: .eeg + .vhdr + .vmrk (any basename)
    - plus at least one .wav
    You can tighten this later if you want strict naming rules.
    """
    has_eeg = any(f[0].lower().endswith(".eeg") for f in files)
    has_vhdr = any(f[0].lower().endswith(".vhdr") for f in files)
    has_vmrk = any(f[0].lower().endswith(".vmrk") for f in files)
    has_wav = any(f[0].lower().endswith(".wav") for f in files)
    complete = has_eeg and has_vhdr and has_vmrk and has_wav
    return complete, (has_eeg, has_vhdr, has_vmrk, has_wav)

def list_sessions():
    items = []
    if not BASE.exists():
        return items
    for p in BASE.iterdir():
        if p.is_dir():
            try:
                st = p.stat()
                items.append((p.name, st.st_mtime))
            except FileNotFoundError:
                pass
    items.sort(key=lambda x: x[1], reverse=True)  # newest first
    return items

def lock_path(session_name: str) -> Path:
    return BASE / f".ziplock_{session_name}"

def zip_paths(session_name: str):
    """Create zip beside the directory if it doesn't exist, atomically."""
    if not SAFE_NAME.match(session_name):
        raise ValueError("Bad session name")

    session_dir = BASE / session_name
    if not session_dir.is_dir():
        raise FileNotFoundError("No such directory")

    zip_final = BASE / f"{session_name}.zip"
    zip_tmp = BASE / f"{session_name}.zip.tmp"

    if zip_final.exists():
        return zip_final

    # Per-session lock to prevent concurrent zips
    lp = lock_path(session_name)
    with open(lp, "w") as lockf:
        try:
            import fcntl
            fcntl.flock(lockf.fileno(), fcntl.LOCK_EX)
        except Exception:
            # If flock isn't available for some reason, continue without locking.
            pass

        # Check again after lock
        if zip_final.exists():
            return zip_final

        # Build zip (run in BASE so zip contains session_name/...)
        cmd = [ZIP_CMD, "-r", "-q", str(zip_tmp.name), session_name]
        proc = subprocess.run(
            cmd,
            cwd=str(BASE),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=600,   # plenty; adjust if you like
            check=False,
        )
        if proc.returncode != 0:
            # leave a visible error marker inside the session folder
            errfile = session_dir / "ZIP_FAILED.txt"
            try:
                errfile.write_text(
                    "zip failed\n"
                    f"cmd: {' '.join(cmd)}\n"
                    f"rc: {proc.returncode}\n"
                    f"stderr:\n{proc.stderr.decode(errors='replace')}\n"
                )
            except Exception:
                pass
            if zip_tmp.exists():
                try: zip_tmp.unlink()
                except Exception: pass
            raise RuntimeError("zip failed")

        # Atomic rename
        zip_tmp.replace(zip_final)
        try:
            os.chmod(zip_final, 0o444)  # read-only
        except Exception:
            pass
        return zip_final

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        # Route:
        # /app/                -> HTML page listing sessions + buttons
        # /zip/<session>       -> build (if needed) and download zip
        if self.path == "/" or self.path == "/app" or self.path == "/app/":
            self.serve_app()
            return

        if self.path.startswith("/zip/"):
            session = unquote(self.path[len("/zip/"):]).strip("/")
            self.serve_zip(session)
            return

        self.send_error(404, "Not found")

    def serve_app(self):
        sessions = list_sessions()
        latest_name = sessions[0][0] if sessions else None

        rows = []
        for name, mtime in sessions:
            # Basic name safety
            esc_name = html.escape(name)
            dir_path = (BASE / name)

            files, total = session_files(dir_path)
            fcount = len(files)

            complete, parts = session_status(name, files)
            (has_eeg, has_vhdr, has_vmrk, has_wav) = parts

            zip_exists = (BASE / f"{name}.zip").exists()

            badge = ""
            if name == latest_name:
                badge = " <span class='badge latest'>LATEST</span>"

            status_txt = "Complete" if complete else "Incomplete"
            status_class = "ok" if complete else "warn"

            zip_txt = "Ready" if zip_exists else "Not yet"
            zip_class = "ok" if zip_exists else "warn"

            # Small completeness breakdown
            parts_txt = []
            parts_txt.append("eeg" if has_eeg else "<span class='miss'>eeg</span>")
            parts_txt.append("vhdr" if has_vhdr else "<span class='miss'>vhdr</span>")
            parts_txt.append("vmrk" if has_vmrk else "<span class='miss'>vmrk</span>")
            parts_txt.append("wav" if has_wav else "<span class='miss'>wav</span>")
            parts_html = " / ".join(parts_txt)

            # File list (expandable)
            file_lines = []
            for fn, sz, mt in files:
                file_lines.append(
                    f"<div class='fileline'>"
                    f"<span class='fname'>{html.escape(fn)}</span>"
                    f"<span class='fmeta'>{html.escape(human_size(sz))} &nbsp;·&nbsp; {html.escape(fmt_time(mt))}</span>"
                    f"</div>"
                )
            file_list_html = "".join(file_lines) if file_lines else "<div class='fileline'><i>No files</i></div>"

            rows.append(
                f"<tr>"
                f"<td class='mono'>{esc_name}{badge}</td>"
                f"<td>{html.escape(fmt_time(mtime))}</td>"
                f"<td>{fcount}</td>"
                f"<td>{html.escape(human_size(total))}</td>"
                f"<td><span class='pill {status_class}'>{status_txt}</span><div class='sub'>{parts_html}</div></td>"
                f"<td><span class='pill {zip_class}'>{zip_txt}</span></td>"
                f"<td class='actions'>"
                f"<a class='btn' href='/zip/{esc_name}'>Download ZIP</a>"
                f"<a class='link' href='/files/{esc_name}/' target='_blank'>Browse files</a>"
                f"<details class='details'>"
                f"<summary>Show files</summary>"
                f"<div class='filebox'>{file_list_html}</div>"
                f"</details>"
                f"</td>"
                f"</tr>"
            )

        body = f"""<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>Octopus node-stor downloads</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {{
      font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial;
      margin: 24px;
      color: #111;
    }}
    .wrap {{ max-width: 1200px; }}
    h2 {{ margin: 0 0 6px 0; }}
    .hint {{ margin: 0 0 16px 0; color: #444; }}
    table {{ border-collapse: collapse; width: 100%; }}
    thead th {{
      text-align: left;
      padding: 10px 10px;
      border-bottom: 1px solid #ddd;
      background: #fafafa;
      position: sticky;
      top: 0;
    }}
    tbody td {{
      padding: 10px 10px;
      border-bottom: 1px solid #eee;
      vertical-align: top;
    }}
    .mono {{ font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; }}
    .actions .btn {{
      display: inline-block;
      padding: 6px 10px;
      background: #2d6cdf;
      color: white;
      text-decoration: none;
      border-radius: 8px;
      margin-right: 10px;
      white-space: nowrap;
    }}
    .actions .link {{
      color: #2d6cdf;
      text-decoration: none;
      margin-right: 10px;
      white-space: nowrap;
    }}
    .pill {{
      display: inline-block;
      padding: 3px 8px;
      border-radius: 999px;
      font-size: 12px;
      border: 1px solid #ddd;
      background: #f5f5f5;
    }}
    .pill.ok {{ background: #eef8ee; border-color: #cfe8cf; }}
    .pill.warn {{ background: #fff5e6; border-color: #ffe0b2; }}
    .sub {{ font-size: 12px; color: #444; margin-top: 4px; }}
    .miss {{ color: #b00020; font-weight: 600; }}
    .badge {{
      font-size: 11px;
      padding: 2px 6px;
      border-radius: 6px;
      border: 1px solid #ddd;
      margin-left: 6px;
    }}
    .badge.latest {{ background: #f0f7ff; border-color: #cfe4ff; }}
    .details {{ display: inline-block; margin-top: 8px; }}
    summary {{ cursor: pointer; color: #333; }}
    .filebox {{
      margin-top: 8px;
      padding: 10px;
      border: 1px solid #eee;
      border-radius: 10px;
      background: #fafafa;
      min-width: 420px;
      max-width: 720px;
    }}
    .fileline {{
      display: flex;
      justify-content: space-between;
      gap: 12px;
      padding: 4px 0;
      border-bottom: 1px dashed #eee;
    }}
    .fileline:last-child {{ border-bottom: none; }}
    .fname {{ font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; }}
    .fmeta {{ color: #555; font-size: 12px; white-space: nowrap; }}
    @media (max-width: 900px) {{
      thead th:nth-child(3),
      thead th:nth-child(4),
      tbody td:nth-child(3),
      tbody td:nth-child(4) {{
        display: none;
      }}
      .filebox {{ min-width: auto; width: 100%; }}
      .fileline {{ flex-direction: column; align-items: flex-start; }}
      .fmeta {{ white-space: normal; }}
    }}
  </style>
</head>
<body>
  <div class="wrap">
    <h2>Octopus node-stor recordings</h2>
    <p class="hint">
      Click <b>Download ZIP</b> to download the whole session as one file.
      If ZIP is not ready yet, you can still <b>Browse files</b>.
    </p>

    <table>
      <thead>
        <tr>
          <th>Session</th>
          <th>Modified</th>
          <th>Files</th>
          <th>Total size</th>
          <th>Status</th>
          <th>ZIP</th>
          <th>Actions</th>
        </tr>
      </thead>
      <tbody>
        {''.join(rows) if rows else "<tr><td colspan='7' style='padding:12px;'><i>No sessions found.</i></td></tr>"}
      </tbody>
    </table>
  </div>
</body>
</html>
"""
        data = body.encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def serve_zip(self, session: str):
        try:
            zpath = zip_paths(session)
        except FileNotFoundError:
            self.send_error(404, "No such session")
            return
        except ValueError:
            self.send_error(400, "Bad session name")
            return
        except subprocess.TimeoutExpired:
            self.send_error(504, "Zipping timed out")
            return
        except Exception:
            self.send_error(500, "Zipping failed")
            return

        # Stream the zip file
        try:
            st = zpath.stat()
            self.send_response(200)
            self.send_header("Content-Type", "application/zip")
            self.send_header("Content-Length", str(st.st_size))
            self.send_header("Content-Disposition", f'attachment; filename="{zpath.name}"')
            self.end_headers()
            with open(zpath, "rb") as f:
                shutil.copyfileobj(f, self.wfile)
        except FileNotFoundError:
            self.send_error(404, "Zip not found")
        except Exception:
            self.send_error(500, "Error sending zip")

def main():
    # Safety: ensure BASE exists
    BASE.mkdir(parents=True, exist_ok=True)
    httpd = HTTPServer((HOST, PORT), Handler)
    print(f"Serving on http://{HOST}:{PORT}/app/  BASE={BASE}")
    httpd.serve_forever()

if __name__ == "__main__":
    main()
