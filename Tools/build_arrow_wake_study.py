from pathlib import Path
from PIL import Image
import io,base64
root=Path(__file__).resolve().parents[1]
def data(path,size):
    im=Image.open(path).convert('RGBA');im.thumbnail(size,Image.Resampling.LANCZOS)
    out=io.BytesIO();im.save(out,format='PNG',optimize=True)
    return 'data:image/png;base64,'+base64.b64encode(out.getvalue()).decode()
text=(root/'Tools/arrow-wake-study.template.html').read_text()
text=text.replace('__HERO__',data(root/'ArtSource/HeroFullBodyV1/runtime-test/FullBody_BowDrawFire_2.png',(512,256)))
text=text.replace('__ARROW__',data(root/'ArtSource/BowsV2/BowFlight_0.png',(64,64)))
out=Path('C:/Users/tritl/.codex/visualizations/2026/09/20/01a0bffb-e891-7110-bda7-4f3879f2880b/arrow-air-wake-study.html')
out.write_text(text,encoding='utf-8');assert out.stat().st_size<1000000
print(out, out.stat().st_size)
