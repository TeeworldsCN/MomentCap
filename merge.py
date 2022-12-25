from PIL import Image

total_tile_x = 58
total_tile_y = 38
tile_x = 58
tile_y = 38
source_w = 2300
source_h = 2000
offset_x = 1000
offset_y = 850
crop_left = 400
crop_top = 380
feather_radius = 100

final_w = offset_x * (tile_x - 1) + source_w
final_h = offset_y * (tile_y - 1) + source_h

folder = "C:/Users/tsfre/AppData/Roaming/Teeworlds/screenshots/"

final_image = Image.new("RGB", (final_w, final_h))

def process_vec(x, y):
    index = y * total_tile_x + x

    filename = folder + "%04d_%02d_%02d.png" % (index, x, y)
    print("Processing: " + filename)
    image = Image.open(filename)

    paste_left = offset_x * x + crop_left;
    paste_top = offset_y * y + crop_top;

    tmp_crop_w = source_w - (crop_left * 2);
    tmp_crop_h = source_h - (crop_top * 2);
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
        paste_left = offset_x * x
        feather = False
    
    if y == tile_y-1:
        tmp_crop_top = 0
        tmp_crop_h = source_h
        paste_top = offset_y * y
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