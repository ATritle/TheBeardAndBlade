"""Extract the generated inventory icon atlas without changing the artwork."""
from pathlib import Path
from PIL import Image
import shutil,sys
root=Path(__file__).resolve().parents[1]
source=root/'ArtSource/InventoryExpansion/InventoryIcons-v2.png'
shutil.copy2(sys.argv[1],source)
atlas=Image.open(source).convert('RGBA')
for i,name in enumerate(['Satchel','Helmet','Gloves','Pants','Boots']):
    y,x=divmod(i,3)
    tile=atlas.crop((x*atlas.width//3,y*atlas.height//2,(x+1)*atlas.width//3,(y+1)*atlas.height//2))
    box=tile.getchannel('A').point(lambda a:255 if a>100 else 0).getbbox()
    assert box,name
    tile=tile.crop(box);tile.thumbnail((112,112),Image.Resampling.NEAREST)
    out=Image.new('RGBA',(128,128));out.alpha_composite(tile,((128-tile.width)//2,(128-tile.height)//2))
    out.save(root/'Content/Art/V2'/f'Inventory{name}.png')
print('INVENTORY_ICONS_PREPARED',atlas.getchannel('A').getextrema())
