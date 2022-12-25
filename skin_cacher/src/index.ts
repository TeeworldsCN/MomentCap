import express from 'express';
import axios from 'axios';
import fs from 'fs';

const SKIN_DB = ['https://skins.ddnet.tw/skin/', 'https://api.skins.tw/api/resolve/skins/'];
const CACHE_PATH = './cache/';

const app = express();

app.get('/:file', async (req, res) => {
  const file = req.params.file;

  if (fs.existsSync(CACHE_PATH + file)) {
    fs.readFile(CACHE_PATH + file, (err, data) => {
      res.contentType('image/png');
      res.send(data);
    });

    return;
  }

  for (const db of SKIN_DB) {
    try {
      const response = await axios.get(`${db}${file}`, { responseType: 'arraybuffer' });
      if (response.status === 200) {
        res.contentType('image/png');
        res.send(response.data);
        fs.writeFileSync(CACHE_PATH + file, response.data);
        return;
      }
    } catch (e) {
      continue;
    }
  }
  res.sendStatus(404);
});

app.listen(4294);
