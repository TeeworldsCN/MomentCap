from PIL import Image

tile_x = 17
tile_y = 24
source_w = 1920
source_h = 1080
offset_x = 1630
offset_y = 970
crop_w = 1200
crop_h = 675
feather_radius = 100

left_step = offset_x - source_w // 2
top_step = offset_y - source_h // 2

left_start = offset_x - crop_w // 2 - left_step
top_start = offset_y - crop_h // 2 - top_step

final_w = left_start * 2 + left_step * tile_x
final_h = top_start * 2 + top_step * tile_y

crop_left = (source_w - crop_w) // 2
crop_top = (source_h - crop_h) // 2

folder = "/Users/tsfreddie/Library/Application Support/Teeworlds/screenshots/"

final_image = Image.new("RGB", (final_w, final_h))

def process_vec(x, y):
    index = y * tile_x + x

    filename = folder + "%04d_%02d_%02d.png" % (index, x, y)
    print("Processing: " + filename)
    image = Image.open(filename)

    paste_left = left_start + left_step * x
    paste_top = top_start + top_step * y

    tmp_crop_w = crop_w
    tmp_crop_h = crop_h
    tmp_crop_left = crop_left
    tmp_crop_top = crop_top

    feather = True
    if x == 0:
        tmp_crop_left = 0
        tmp_crop_w = source_w
        paste_left = 0
        feather = False

    if y == 0:
        tmp_crop_top = 0
        tmp_crop_h = source_h
        paste_top = 0
        feather = False
    
    if x == tile_x-1:
        tmp_crop_left = 0
        tmp_crop_w = source_w
        paste_left = left_step * x
        feather = False
    
    if y == tile_y-1:
        tmp_crop_top = 0
        tmp_crop_h = source_h
        paste_top = top_step * y
        feather = False
    
    image = image.crop((tmp_crop_left, tmp_crop_top, tmp_crop_w, tmp_crop_h))
    final_image.paste(image, (paste_left, paste_top))

for y in [0, tile_y - 1]:
    for x in [0, tile_x - 1]:
        process_vec(x, y)

for y in [0, tile_y - 1]:
    for x in range(1, tile_x - 1):
        process_vec(x, y)

for y in range(1, tile_y - 1):
    for x in [0, tile_x - 1]:
        process_vec(x, y)

for y in range(1, tile_y - 1):
    for x in range(1, tile_x - 1):
        process_vec(x, y)

final_image.save("final.png")