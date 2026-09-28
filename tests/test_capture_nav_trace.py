import importlib.util
from pathlib import Path

path = Path(__file__).resolve().parents[1] / "tools" / "capture_nav_trace.py"
spec = importlib.util.spec_from_file_location("capture_nav_trace", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

assert module.fnv1a32(b"hello") == 0x4F9F2CAB
assert len(module.HEADER.split(",")) == 31
payload = "NAVCSV,1," + ",".join(["x"] * 28)
good = payload + f",{module.fnv1a32(payload.encode('ascii')):08X}"
assert module.valid_record(good)
assert not module.valid_record(good.replace(",x,", ",y,", 1))
assert not module.valid_record(module.HEADER)
print("capture checksum/schema: PASS")
