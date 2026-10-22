"""Generate the checked-in 10x16 bitmap subset from OFL Noto Sans Mono."""
from pathlib import Path
import sys
from PIL import Image, ImageFont, ImageDraw
font = ImageFont.truetype(sys.argv[1], 13)
codes = [ord('?')] + [c for c in range(32,127) if c!=ord('?')] + list(range(0x410,0x450)) + [0x401,0x451,0xAB,0xBB,0x2014,0xA9]
rows=[]
for cp in codes:
    im=Image.new('L',(10,16)); d=ImageDraw.Draw(im)
    d.text((0,-1),chr(cp),font=font,fill=255)
    rows.append([sum((1<<(9-x)) for x in range(10) if im.getpixel((x,y))>=96) for y in range(16)])
output='#pragma once\n#include <cstddef>\n#include <cstdint>\n// Generated from Noto Sans Mono, SIL OFL 1.1. See assets/OFL.txt.\n'
output+='inline constexpr size_t font_count='+str(len(codes))+';\n'
output+='inline constexpr uint32_t font_codes[]={'+','.join(map(str,codes))+'};\n'
output+='inline constexpr uint16_t font_rows[][16]={\n'+',\n'.join('{'+','.join(map(str,r))+'}' for r in rows)+'\n};\n'
Path(__file__).resolve().parents[1].joinpath('src/presentation/font_data.hpp').write_text(output,encoding='utf-8')
