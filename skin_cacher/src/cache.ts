import axios from 'axios';
import fs from 'fs';

const intsToStr = (ints: number[]) => {
  const chars = [];
  for (const i of ints) {
    chars.push(((i >> 24) & 0xff) - 128);
    chars.push(((i >> 16) & 0xff) - 128);
    chars.push(((i >> 8) & 0xff) - 128);
    chars.push((i & 0xff) - 128);
  }
  chars.splice(chars.indexOf(0));
  const buffer = Buffer.from(chars);
  return buffer.toString('utf-8');
};

const path = `latest.poses`;
const buffer = fs.readFileSync(path);
// read 8 bytes of integer
const size = buffer.readBigInt64LE(0);

const notFound: { [key: string]: boolean } = {};

const task = async () => {
  for (let i = 0; i < size; i++) {
    // extract 248 bytes of data
    const data = buffer.slice(8 + i * 248, 8 + (i + 1) * 248);
    const skin = intsToStr([
      data.readInt32LE(32),
      data.readInt32LE(32 + 4),
      data.readInt32LE(32 + 8),
      data.readInt32LE(32 + 12),
      data.readInt32LE(32 + 16),
      data.readInt32LE(32 + 20),
    ]);

    if (notFound[skin]) {
      continue;
    }

    try {
      await axios.get(`http://localhost:4294/${skin}.png`, { timeout: 300000 });
    } catch (e) {
      notFound[skin] = true;
      console.log(`${skin} not found`);
    }
  }
};

task();
