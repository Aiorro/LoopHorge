"""Exercise the native worker transport without downloading weights."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

REVISION = "a" * 40
with tempfile.TemporaryDirectory(prefix="loopforge model fixture ") as base:
    root = Path(base)
    model = root / "models" / REVISION
    model.mkdir(parents=True)
    for file in ("config.json", "model.safetensors"):
        (model / file).touch()
    (root / "runtime.json").write_text(json.dumps({"version": 1, "python": sys.executable,
                                                   "revision": REVISION, "device": "cpu"}), encoding="utf-8")
    (root / "worker.py").write_text(
        """import json, pathlib, sys, time
command, model, request, response = sys.argv[1:]
assert command == "generate"
data = json.loads(pathlib.Path(request).read_text(encoding="utf-8"))
if data["direction"] == "Slow":
    time.sleep(10)
assert data["direction"] == 'Jazz "guitar" and bass'
assert data["bars"] == 4 and data["bpm"] == 120 and data["seed"] == 17
events = [128256,138306,139316,128306,138266,155676]
pathlib.Path(response).write_text(json.dumps({"version":1,"tokens":events}),encoding="utf-8")
""", encoding="utf-8")
    subprocess.run([sys.argv[1], base], check=True, timeout=20)
    assert {p.name for p in root.iterdir()} == {"models", "runtime.json", "worker.py"}
