const sharp = require('sharp');

sharp('final.png', {limitInputPixels: false}).webp({nearLossless:true}).tile({size: 256, overlap: 2}).toFile('D:\\Projects\\EventScreenshots\\2022\\data\\2022.dzi');