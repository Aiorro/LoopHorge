"""LoopForge's standalone MIDI-LLM setup and one-shot local worker."""
import ctypes
import json
import os
from pathlib import Path
import queue
import shutil
import subprocess
import sys
import threading
import venv
import webbrowser

REPOSITORY = "slseanwu/MIDI-LLM_Llama-3.2-1B"
MODEL_PAGE = f"https://huggingface.co/{REPOSITORY}"
TEXT_VOCAB = 128256
MIDI_BOS = TEXT_VOCAB + 55026
MODEL_FILES = ("config.json", "generation_config.json", "model.safetensors",
               "tokenizer.json", "tokenizer_config.json", "special_tokens_map.json")
GIB = 1024 ** 3


def app_root():
    if os.name == "nt":
        base = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
    else:
        base = Path(os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config"))
    return base / "LoopForge" / "MIDI-LLM"


def profile():
    if os.name == "nt":
        class Memory(ctypes.Structure):
            _fields_ = [("size", ctypes.c_ulong), ("load", ctypes.c_ulong)] + [
                (name, ctypes.c_ulonglong) for name in (
                    "ram", "available", "pagefile", "page_available", "virtual",
                    "virtual_available", "extended")]
        stat = Memory()
        stat.size = ctypes.sizeof(stat)
        ram = stat.ram if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(stat)) else 0
    else:
        ram = os.sysconf("SC_PHYS_PAGES") * os.sysconf("SC_PAGE_SIZE")
    base = next((p for p in (app_root(), *app_root().parents) if p.is_dir()), Path.home())
    free = shutil.disk_usage(base).free
    vram = 0
    locator = shutil.which("nvidia-smi")
    if locator:
        try:
            probe = subprocess.run([locator, "--query-gpu=memory.total", "--format=csv,noheader,nounits"],
                                   capture_output=True, text=True, check=True, timeout=4)
            vram = max(int(value.strip()) for value in probe.stdout.splitlines()) * 1024 ** 2
        except (OSError, ValueError, subprocess.SubprocessError):
            pass
    return ram, free, vram


def install(root, cuda, report):
    root.mkdir(parents=True, exist_ok=True, mode=0o700)
    environment = root / ("cuda" if cuda else "cpu")
    python = environment / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
    report("Preparing the isolated Python runtime...")
    venv.EnvBuilder(with_pip=True).create(environment)
    def pip(*args):
        subprocess.run([str(python), "-I", "-m", "pip", "--isolated", "--disable-pip-version-check", "install", *args],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=1800)
    device = "cu126" if cuda else "cpu"
    report("Installing PyTorch and Transformers...")
    pip("torch==2.8.0", "--index-url", f"https://download.pytorch.org/whl/{device}")
    pip("transformers==4.57.1", "huggingface_hub==0.36.0",
        "safetensors==0.6.2", "accelerate==1.12.0", "--index-url", "https://pypi.org/simple")
    report("Checking size and downloading the published model files...")
    subprocess.run([str(python), "-I", str(Path(__file__).resolve()), "download", str(root), device],
                   check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=7200)
    report("MIDI-LLM is ready. Return to LoopForge to compose.")


def download(root, device):
    os.environ["HF_HUB_DISABLE_TELEMETRY"] = "1"
    from huggingface_hub import HfApi, snapshot_download
    import torch
    if device != "cpu" and not (torch.cuda.is_available() and torch.cuda.is_bf16_supported()):
        raise RuntimeError("This CUDA runtime requires a GPU with BF16 support")
    metadata = HfApi().model_info(REPOSITORY, files_metadata=True)
    available = {file.rfilename: file.size for file in metadata.siblings}
    required = {"config.json", "model.safetensors", "tokenizer.json", "tokenizer_config.json"}
    if (not required.issubset(available) or not metadata.sha or len(metadata.sha) != 40
            or any(c not in "0123456789abcdef" for c in metadata.sha)):
        raise RuntimeError("The published model files changed")
    size = sum(available.get(file) or 0 for file in MODEL_FILES)
    if size <= 0 or size > 4 * GIB or shutil.disk_usage(root).free < size + GIB:
        raise RuntimeError("Insufficient disk space or unexpected download size")
    model = root / "models" / metadata.sha
    snapshot_download(REPOSITORY, revision=metadata.sha, allow_patterns=list(MODEL_FILES), local_dir=model)
    config = json.loads((model / "config.json").read_text(encoding="utf-8"))
    if config.get("model_type") != "llama" or config.get("vocab_size") != 183286 or config.get("auto_map"):
        raise RuntimeError("Unexpected model architecture")
    worker = root / "worker.py"
    # Setup ships as a separate file. Keep the applied runtime available to any host.
    source = Path(__file__).resolve()
    if worker.resolve() != source:
        pending_worker = root / "worker.pending"
        shutil.copyfile(source, pending_worker)
        pending_worker.replace(worker)
    pending = root / "runtime.pending"
    pending.write_text(json.dumps({"version": 1, "python": sys.executable, "revision": metadata.sha,
                                   "device": device}), encoding="utf-8")
    pending.replace(root / "runtime.json")


def generate(model, request_path, response_path):
    # Generation uses only the validated local snapshot. No remote code or
    # Hugging Face API calls occur in a DAW generation request.
    os.environ["HF_HUB_OFFLINE"] = "1"
    os.environ["TRANSFORMERS_OFFLINE"] = "1"
    os.environ["HF_HUB_DISABLE_TELEMETRY"] = "1"
    import torch
    from transformers import AutoModelForCausalLM, AutoTokenizer

    request = json.loads(request_path.read_text(encoding="utf-8"))
    bars = request["bars"]
    if request["version"] != 1 or bars not in (4, 8, 16):
        raise ValueError("Invalid loop length")
    cuda = request["device"] == "cu126"
    if cuda and not (torch.cuda.is_available() and torch.cuda.is_bf16_supported()):
        raise RuntimeError("CUDA device unavailable")
    device = "cuda" if cuda else "cpu"
    dtype = torch.bfloat16 if cuda else torch.float32
    if not cuda:
        torch.set_num_threads(max(1, min(8, os.cpu_count() or 1)))
    tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=False, local_files_only=True)
    network = AutoModelForCausalLM.from_pretrained(model, local_files_only=True, trust_remote_code=False,
                                                  use_safetensors=True, dtype=dtype).to(device).eval()
    prompt = ("You are a world-class composer. Please compose some music according to the following description: "
              f"{request['direction']} "
              f"Length: {bars} bars. Tempo: {request['bpm']} BPM. "
              f"Key: {request['key']}. Meter: {request['meter']}. {request['hints']} ")
    prefix = tokenizer(prompt, return_tensors="pt", truncation=True, max_length=1536)["input_ids"].to(device)
    prefix = torch.cat((prefix, torch.tensor([[MIDI_BOS]], device=device)), dim=1)
    prompt_length = prefix.shape[1]
    eos = tokenizer.eos_token_id
    if not isinstance(eos, int):
        raise RuntimeError("Unknown model EOS")
    # Each AMT triplet is absolute time, duration, and instrument/pitch.
    groups = [list(range(TEXT_VOCAB, TEXT_VOCAB+10000)) + [eos],
              list(range(TEXT_VOCAB+10000, TEXT_VOCAB+11000)),
              list(range(TEXT_VOCAB+11000, TEXT_VOCAB+11000+129*128))]
    torch.manual_seed(request["seed"])
    with torch.inference_mode():
        generated = network.generate(
            prefix, max_new_tokens=min(2046, 126*bars), do_sample=True, temperature=1.0, top_p=0.98,
            eos_token_id=eos, pad_token_id=eos,
            prefix_allowed_tokens_fn=lambda _batch, ids: groups[(ids.shape[0]-prompt_length) % 3],
        )
    sequence = generated[0, prompt_length:].tolist()
    if sequence and sequence[-1] == eos:
        sequence.pop()
    response_path.write_text(json.dumps({"version": 1, "tokens": sequence}, separators=(",", ":")),
                             encoding="utf-8")


def wizard():
    import tkinter as tk
    from tkinter import ttk
    window = tk.Tk()
    window.title("LoopForge  •  MIDI-LLM setup")
    window.resizable(False, False)
    frame = ttk.Frame(window, padding=24)
    frame.grid()
    ram, disk, vram = profile()
    suitable_python = (3, 11) <= sys.version_info[:2] <= (3, 12) and sys.maxsize > 2**32
    cuda = vram >= 15 * GIB
    available = suitable_python and disk >= (20 if cuda else 9) * GIB and (cuda or ram >= 12 * GIB)
    ttk.Label(frame, text="Local MIDI-LLM", font=("Segoe UI", 20, "bold")).grid(sticky="w")
    ttk.Label(frame, text=f"RAM {ram/GIB:.1f} GiB   •   Free disk {disk/GIB:.1f} GiB   •   NVIDIA VRAM {vram/GIB:.1f} GiB").grid(sticky="w", pady=(9, 5))
    if not suitable_python:
        guidance = "Install 64-bit Python 3.11 or 3.12 and reopen Setup."
    elif not available:
        guidance = "This computer is below the setup requirements. The built-in LoopForge Composer is available."
    elif cuda:
        guidance = "Recommended runtime: CUDA/BF16. Model weights are about 2.8 GB. Runtime downloads are additional."
    else:
        guidance = "Suggested runtime: CPU. Model weights are about 2.8 GB. Generation can take several minutes."
    ttk.Label(frame, text=guidance, wraplength=480).grid(sticky="w", pady=(0, 10))
    ttk.Button(frame, text="View model and license on Hugging Face", command=lambda: webbrowser.open(MODEL_PAGE)).grid(sticky="w")
    consent = tk.BooleanVar()
    use_cuda = tk.BooleanVar(value=cuda)
    status = tk.StringVar(value="Built with Llama 3.2. The model and runtime are stored on this computer.")
    messages = queue.SimpleQueue()
    checking = False

    def refresh_button():
        gpu = use_cuda.get()
        eligible = suitable_python and disk >= (20 if gpu else 9)*GIB and (gpu or ram >= 12*GIB)
        install_button.configure(state="normal" if eligible and consent.get() and not checking else "disabled")

    def begin():
        nonlocal checking
        checking = True
        gpu = use_cuda.get()
        refresh_button()
        def work():
            try:
                install(app_root(), gpu, lambda message: messages.put((False, message)))
            except (OSError, ValueError, RuntimeError, subprocess.SubprocessError):
                messages.put((False, "Setup failed. Check network access, disk space and your Python/CUDA runtime."))
            finally:
                messages.put((True, None))
        threading.Thread(target=work, daemon=False).start()

    def poll():
        nonlocal checking
        while not messages.empty():
            done, message = messages.get_nowait()
            if message:
                status.set(message)
            if done:
                checking = False
                refresh_button()
        window.after(150, poll)

    def close():
        if checking:
            status.set("Setup is running. Leave this window open until setup completes.")
        else:
            window.destroy()

    if cuda:
        ttk.Checkbutton(frame, text="Use NVIDIA CUDA (BF16 required). Clear to use CPU.",
                        variable=use_cuda, command=refresh_button).grid(sticky="w")
    ttk.Checkbutton(frame, text="I have reviewed and accept the Llama 3.2 license", variable=consent,
                    command=refresh_button).grid(sticky="w", pady=8)
    install_button = ttk.Button(frame, text="Download runtime and model", command=begin)
    install_button.grid(sticky="w")
    ttk.Label(frame, textvariable=status, wraplength=480).grid(sticky="w", pady=(12, 0))
    refresh_button()
    window.protocol("WM_DELETE_WINDOW", close)
    window.after(150, poll)
    window.mainloop()


if __name__ == "__main__":
    if len(sys.argv)>1 and sys.argv[1] == "generate":
        generate(*(Path(value) for value in sys.argv[2:]))
    elif len(sys.argv)>1 and sys.argv[1] == "download":
        download(Path(sys.argv[2]), sys.argv[3])
    else:
        wizard()
