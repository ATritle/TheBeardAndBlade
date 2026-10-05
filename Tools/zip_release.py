"""Archive a complete staged Windows build; omit debug symbols and user data."""
from pathlib import Path
import zipfile,hashlib,argparse
parser=argparse.ArgumentParser()
parser.add_argument('--version',default='v0.4.2')
parser.add_argument('--build-dir',type=Path,help='Fresh archive directory containing Windows, for same-version refreshes')
parser.add_argument('--output-dir',type=Path,help='Separate output folder; existing archives are never overwritten')
parser.add_argument('--streaming',action='store_true',help='Use StreamPixel Windows layout and hosting checklist')
parser.add_argument('--vagon',action='store_true',help='Put Windows build contents at ZIP root for Vagon')
args=parser.parse_args()
assert not (args.streaming and args.vagon), 'Choose one hosting layout'
assert all(c.isalnum() or c in '.-_' for c in args.version)
root=Path(__file__).resolve().parents[1]
build=(args.build_dir if args.build_dir else root/'Builds'/args.version)/'Windows'
assert (build/'TheBeardAndBlade.exe').is_file()
assert list(build.rglob('*.ucas'))
assert not (build/'BeardAndBlade').exists(), 'Old project payload must not ship'
assert not (build/'BeardAndBlade.exe').exists(), 'Old launcher must not ship'
assert (build/'Engine/Extras/Redist/en-us/vc_redist.x64.exe').is_file()
prefix='TheBeardAndBlade-Windows-Vagon' if args.vagon else 'TheBeardAndBlade-StreamPixel' if args.streaming else 'TheBeardAndBlade-Windows'
archive_root=Path('.') if args.vagon else Path('Windows' if args.streaming else 'TheBeardAndBlade')
output_dir=args.output_dir if args.output_dir else root/'Builds'
output_dir.mkdir(parents=True,exist_ok=True)
out=output_dir/f'{prefix}-{args.version}.zip'
assert not out.exists(), 'Choose a new version; preserve existing player archives'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for file in sorted(build.rglob('*')):
        if not file.is_file() or file.suffix.lower() in ('.pdb','.log') or 'Saved' in file.relative_to(build).parts:continue
        if file.name.startswith('Manifest_'):continue
        z.write(file,archive_root/file.relative_to(build))
    docs=('STREAMPixel_SETUP.md',) if args.streaming else ('README.md','RELEASE_NOTES.md','PLAYTEST.md')
    for name in docs:
        z.write(root/name,archive_root/name)
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
checksum=hashlib.file_digest(out.open('rb'),'sha256').hexdigest()
out.with_suffix('.sha256').write_text(f'{checksum}  {out.name}\n')
print(out, out.stat().st_size, 'bytes; SHA256',checksum)
