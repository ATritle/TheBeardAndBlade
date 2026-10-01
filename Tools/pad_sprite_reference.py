"""Add transparent working space without resizing the source artwork."""
import argparse
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('output');p.add_argument('--margin',type=int,default=96);a=p.parse_args()
im=Image.open(a.source).convert('RGBA')
out=Image.new('RGBA',(im.width+2*a.margin,im.height+2*a.margin))
out.alpha_composite(im,(a.margin,a.margin));out.save(a.output)
