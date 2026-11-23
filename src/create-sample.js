'use strict';
const fs = require('node:fs');
const path = require('node:path');
const {generateOne, createMidi} = require('./core');

const sampleDirectory = path.resolve(__dirname, '../samples/generated');
const idea = generateOne({key: 'D', mode: 'minor', bpm: 105, bars: 8, style: 'nu-metal'}, 0xC0FFEE);
const midiPath = path.join(sampleDirectory, 'loopforge-demo.mid');

fs.mkdirSync(sampleDirectory, {recursive: true});
fs.writeFileSync(midiPath, createMidi(idea));
fs.writeFileSync(path.join(sampleDirectory, 'loopforge-demo.json'), JSON.stringify(idea, null, 2));
console.log(midiPath);
