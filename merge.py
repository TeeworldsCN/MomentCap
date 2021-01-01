from PIL import Image

tile_x = 16
tile_y = 23
source_w = 1920
source_h = 1080
offset_x = 1630
offset_y = 970
crop_w = 1200
crop_h = 675

left_start = offset_x - crop_w // 2
top_start = offset_y - crop_h // 2

left_step = offset_x - source_w // 2
top_step = offset_y - source_h // 2

final_w = left_start*2 + left_step * tile_x
final_h = top_start*2 + top_step * tile_y

folder = "GoodTest/"

final_image = Image.new("RGB", (final_w, final_h))

for y in range(tile_y):
    for x in range(tile_x):
        index = y * tile_x + x
    
        filename = folder + "%04d_%02d_%02d.png" % (index, x, y)
        print("Processing: " + filename)
        image = Image.open(filename)
        crop_left = (source_w - crop_w) // 2
        crop_top = (source_h - crop_h) // 2
        image = image.crop((crop_left, crop_top, crop_w, crop_h))

        paste_left = left_start + left_step * x
        paste_top = top_start + top_step * y

        final_image.paste(image, (paste_left, paste_top))

final_image.save("final.png")