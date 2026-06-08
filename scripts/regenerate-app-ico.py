"""Regenerate app.ico from a JPG/PNG source. Usage: python regenerate-app-ico.py <src_image> <out.ico>"""
import sys
from pathlib import Path

from PIL import Image


def main() -> None:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    img = Image.open(src).convert("RGBA")
    out.parent.mkdir(parents=True, exist_ok=True)
    # BMP 帧：资源管理里对 exe 图标的显示比默认的 PNG 压缩帧更稳（旧缓存下也更易刷新）。
    img.save(
        out,
        format="ICO",
        sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)],
        bitmap_format="bmp",
    )
    print(out, out.stat().st_size, "bytes")


if __name__ == "__main__":
    main()
