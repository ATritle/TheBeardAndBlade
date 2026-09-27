"""Normalize generated ring art and build a crisp serif glyph atlas for dynamic item cards."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import shutil,sys
root=Path(__file__).resolve().parents[1]
src=root/'ArtSource/InventoryExpansion';src.mkdir(parents=True,exist_ok=True)
out=root/'Content/Art/V2'
for i in range(24):
    icon=Image.open(out/f'Loot_{i}.png').convert('RGBA')
    icon=icon.crop(icon.getchannel('A').getbbox())
    scale=min(100/icon.width,224/icon.height)
    icon=icon.resize((round(icon.width*scale),round(icon.height*scale)),Image.Resampling.NEAREST)
    tile=Image.new('RGBA',(128,256));tile.alpha_composite(icon,((128-icon.width)//2,(256-icon.height)//2))
    tile.save(out/f'BagWeapon_{i}.png')
shutil.copy2(sys.argv[1],src/'RingAtlas-v1.png')
im=Image.open(src/'RingAtlas-v1.png').convert('RGBA')
for i in range(12):
    y,x=divmod(i,4)
    tile=im.crop((x*im.width//4,y*im.height//3,(x+1)*im.width//4,(y+1)*im.height//3))
    box=tile.getchannel('A').point(lambda a:255 if a>100 else 0).getbbox()
    assert box
    tile=tile.crop(box);tile.thumbnail((112,112),Image.Resampling.LANCZOS)
    canvas=Image.new('RGBA',(128,128));canvas.alpha_composite(tile,((128-tile.width)//2,(128-tile.height)//2))
    canvas.save(out/f'Loot_{48+i}.png')
# TrueType is rasterized once; the game renders a bitmap atlas rather than its default UI font.
font=ImageFont.truetype('C:/Windows/Fonts/georgiab.ttf',26)
atlas=Image.new('RGBA',(512,256));d=ImageDraw.Draw(atlas)
for c in range(32,128):
    x=(c-32)%16*32;y=(c-32)//16*40
    d.text((x+16,y+18),chr(c),font=font,anchor='mm',fill='white')
atlas.save(out/'InventoryGlyphs.png')
# Approved front-facing athletic design, preserved rather than inventing another face.
shutil.copy2(sys.argv[3],src/'AdventurerPortrait-v1.png')
hero=Image.open(src/'AdventurerPortrait-v1.png').convert('RGBA')
hero.save(out/'InventoryAdventurer.png')
if len(sys.argv)>2:
    shutil.copy2(sys.argv[2],src/'ItemCard-v1.png');shutil.copy2(sys.argv[2],out/'InventoryCard.png')
print('INVENTORY_ART_PREPARED')
