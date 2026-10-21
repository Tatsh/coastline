#!/usr/bin/env bash
# Regenerate the assembly of one release (us, jp, or eu, default us) after names change in Ghidra.
# The Ghidra project must be open with the HTTP bridge on port 8090. TGR_ROM is the cartridge image
# of the release. The routines asm/<region>/converted.txt records are removed again.
set -euo pipefail
region="${1:-us}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
python="${root}/.sbclaude-venv/bin/python"
exported="${root}/.wiswa-ci/splat/symbol_addrs.${region}.txt"
mkdir -p "$(dirname "${exported}")"
"${python}" -I - "${root}" "${exported}" "${region}" << 'EOF'
import json, sys, urllib.request
from pathlib import Path
root, exported, region = Path(sys.argv[1]), sys.argv[2], sys.argv[3]
sys.path.insert(0, str(root / 'tools' / 'splat'))
from symbols import REGIONS
release = REGIONS[region]
code = (root / 'tools' / 'splat' / 'SymbolsExport.java').read_text()
request = urllib.request.Request(
    f'http://127.0.0.1:8090/run_script_inline?program={release.program}',
    data=json.dumps({'code': code, 'args': f'{exported} 0x{release.bss_end:x}'}).encode(),
    headers={'Content-Type': 'application/json'})
text = urllib.request.urlopen(request, timeout=600).read().decode()
if 'SCRIPT COMPLETED SUCCESSFULLY' not in text:
    sys.exit(text)
EOF
"${python}" -I "${root}/tools/splat/symbols.py" --region "${region}" "${exported}" \
  "${TGR_ROM:?Set TGR_ROM to the cartridge image of the release.}" \
  "${root}/tools/splat/symbol_addrs.${region}.txt"
converted="$(cat "${root}/asm/${region}/converted.txt" 2> /dev/null || true)"
rm -rf "${root}/asm/${region}"
mkdir -p "${root}/asm/${region}"
printf '%s\n' "${converted}" > "${root}/asm/${region}/converted.txt"
(cd "${root}/tools/splat" && "${root}/.sbclaude-venv/bin/splat" split "${region}.yaml" > /dev/null)
rm -rf "${root}/include"
"${python}" -I "${root}/tools/splat/postprocess.py" "${region}"
