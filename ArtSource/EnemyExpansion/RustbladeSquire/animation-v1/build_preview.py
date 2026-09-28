"""Package existing PNG bytes into an offline preview; does not edit artwork."""
from pathlib import Path
import base64, json

root = Path(__file__).resolve().parent
manifest = json.loads((root / 'atlas-manifest.json').read_text())
for entry in manifest['entries']:
    entry['imageSource'] = 'data:image/png;base64,' + base64.b64encode((root / entry['file']).read_bytes()).decode('ascii')
template = (root / 'preview-template.html').read_text(encoding='utf-8')
html = template.replace('__MANIFEST__', json.dumps(manifest))
for name in ['preview.html', 'preview-playback-fix.html']:
    (root / name).write_text(html, encoding='utf-8')
print(f'Built self-contained preview with {len(manifest["entries"])} embedded sheets.')
