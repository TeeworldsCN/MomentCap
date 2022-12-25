const fs = require('fs');

const dirs = fs.readdirSync('poses');

const intsToStr = (ints) => {
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
}

const strToInts = (str, num) => {
  const chars = Buffer.from(str, 'utf-8');
  let index = 0;
  const ints = [] ;
  while (num > 0) {
    const buf = [0,0,0,0];
    for (let c = 0; c < 4 && chars.length > index; c++, index++) {
      buf[c] = chars[index];
    }
    ints.push((buf[0] + 128) << 24 | (buf[1] + 128) << 16 | (buf[2] + 128) << 8 | (buf[3] + 128));
    num--;
  }
  ints[ints.length - 1] &= 0xffffff00;
  return ints;
}

const nameToTees = {};

const hackName = (data, name) => {
  const ints = strToInts(name, 4);
  for (let i = 0; i < ints.length; i++) {
    data.writeInt32LE(ints[i], i * 4);
  }
}

for (const dir of dirs) {
  const path = `poses/${dir}/latest.poses`;
  const buffer = fs.readFileSync(path);
  // read 8 bytes of integer
  const size = buffer.readBigInt64LE(0);
  console.log(`${dir}: ${size}`);
  
  for (let i = 0; i < size; i++) {
    // extract 248 bytes of data
    const data = buffer.slice(8 + i * 248, 8 + (i + 1) * 248);
    const ints = [data.readInt32LE(0), data.readInt32LE(4),data.readInt32LE(8),data.readInt32LE(12)];
    const name = intsToStr(ints);
    const timeoutRaw = data.slice(136, 136 + 64);
    const timeout = timeoutRaw.slice(0, timeoutRaw.indexOf(0)).toString('utf-8');
    const addressRaw = data.slice(200, 200 + 48);
    const address = addressRaw.slice(0, addressRaw.indexOf(0)).toString('utf-8');
    const normalized = name.normalize('NFKC');
    if (nameToTees[normalized]) {
      nameToTees[normalized].push({
        name,
        region: dir,
        data,
        timeout,
        address,
      });
    } else {
      nameToTees[normalized] = [{
        name,
        region: dir,
        data,
        timeout,
        address,
      }];
    }
  }
}

const extractSkinData = (data) => {
  const start = 56;
  const custom = data.readInt32LE(start);
  const body = data.readInt32LE(start + 4);
  const feet = data.readInt32LE(start + 8);
  return {
    custom, body, feet
  }
}

const sameIdentity = (a, b) => {
  const addressSameRange =  a.address.slice(0, a.address.lastIndexOf('.')) == b.address.slice(0, b.address.lastIndexOf('.'));
  const timeoutCodeMatch = (a.timeout == b.timeout && a.timeout.length > 0 && b.timeout.length > 0);
  const skinA = extractSkinData(a.data);
  const skinB = extractSkinData(b.data);
  const sameBodySkinData = skinA.body != 65408 && skinB.body != 65408 && skinA.body == skinB.body;
  const sameFeetSkinData = skinA.feet != 65408 && skinB.feet != 65408 && skinA.feet == skinB.feet;
  return addressSameRange || timeoutCodeMatch || sameBodySkinData || sameFeetSkinData;
}

// remove duplicates
for (const name in nameToTees) {
  const tees = nameToTees[name];

  if (tees.length > 1) {
    for (let i = 0; i < tees.length; i++) {
      tees[i].include = true;
      for (let j = 0; j < i; j++) {
        if (!tees[j].include) continue;
        if (sameIdentity(tees[i], tees[j])) {
          const which = (tees[i].data.byteOffset + tees[j].data.byteOffset) % 2;
          if (which == 0) {
            tees[i].include = false;
            tees[j].include = true;
          } else {
            tees[i].include = true;
            tees[j].include = false;
          }
        }
      }
    }
  } else {
    tees[0].include = true;
  }
}

// put all included ones in output
const output = [];
for (const name in nameToTees) {
  const tees = nameToTees[name];
  for (const tee of tees) {
    if (tee.include) {
      output.push(tee);
    }
  }
}

const nameSet = {};
const finalOutput = [];
for (const tee of output) {
  if (!nameSet[tee.name]) {
    nameSet[tee.name] = 1;
  } else {
    for (let i = 0; i < nameSet[tee.name]; i++) {
      tee.name = ` ${tee.name} `;
    }
    if (Buffer.from(tee.name, 'utf-8').length >= 16)
      tee.include = false;
    else
      hackName(tee.data, tee.name);
    
    nameSet[tee.name]++;
  }
  if (tee.include) finalOutput.push(tee);
}

console.log(`${finalOutput.length} unique tees`);
const binaryOutput = Buffer.alloc(248 * finalOutput.length + 8);
binaryOutput.writeBigInt64LE(BigInt(finalOutput.length), 0);
let i = 0;
for (const tee of finalOutput) {
  tee.data.copy(binaryOutput, 8 + i * 248, 0, 248);
  i++;
}

// write to latest.poses
fs.writeFileSync(`latest.poses`, binaryOutput);