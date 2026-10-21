#!/usr/bin/env bash
# Regenerate the assembly of the USA release after names change in Ghidra. The Ghidra project must
# be open with the HTTP bridge on port 8090. The routines asm/us/converted.txt records are removed
# again.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
python="${root}/.sbclaude-venv/bin/python"
exported="${root}/.wiswa-ci/splat/symbol_addrs.us.txt"
mkdir -p "$(dirname "${exported}")"
"${python}" -I - "${root}" "${exported}" << 'EOF'
import json, sys, urllib.request
from pathlib import Path
root, exported = Path(sys.argv[1]), sys.argv[2]
code = (root / 'tools' / 'splat' / 'SymbolsExport.java').read_text()
request = urllib.request.Request(
    'http://127.0.0.1:8090/run_script_inline?program=usa.z64',
    data=json.dumps({'code': code, 'args': f'{exported} 0x80382bb0'}).encode(),
    headers={'Content-Type': 'application/json'})
text = urllib.request.urlopen(request, timeout=600).read().decode()
if 'SCRIPT COMPLETED SUCCESSFULLY' not in text:
    sys.exit(text)
EOF
"${python}" -I "${root}/tools/splat/symbols.py" "${exported}" "${TGR_ROM:?Set TGR_ROM to the USA cartridge image.}" \
  "${root}/tools/splat/symbol_addrs.us.txt"
converted="$(cat "${root}/asm/us/converted.txt" 2> /dev/null || true)"
rm -rf "${root}/asm/us"
mkdir -p "${root}/asm/us"
printf '%s\n' "${converted}" > "${root}/asm/us/converted.txt"
(cd "${root}/tools/splat" && "${root}/.sbclaude-venv/bin/splat" split us.yaml > /dev/null)
rm -rf "${root}/include"
"${python}" -I "${root}/tools/splat/postprocess.py" us
