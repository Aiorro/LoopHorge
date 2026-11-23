/* LoopForge musical engine - independent, rule-based, deterministic; no external services. */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.LoopForgeCore = factory();
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';

  // ---------------------------------------------------------------------------
  // Musical constants
  // ---------------------------------------------------------------------------

  const PPQ = 480;
  const SCALES = {
    minor: [0, 2, 3, 5, 7, 8, 10],
    major: [0, 2, 4, 5, 7, 9, 11],
    dorian: [0, 2, 3, 5, 7, 9, 10],
    phrygian: [0, 1, 3, 5, 7, 8, 10]
  };
  const ROOTS = {'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11};
  const STYLES = {
    trap:      { density: 0.66, octave: 5, bassOctave: 2, progression: [0,5,2,6], sync: .62 },
    'boom-bap':{ density: 0.48, octave: 5, bassOctave: 2, progression: [0,3,4,0], sync: .35 },
    'r&b':     { density: 0.60, octave: 5, bassOctave: 2, progression: [0,5,3,4], sync: .55 },
    ambient:   { density: 0.30, octave: 5, bassOctave: 2, progression: [0,3,5,4], sync: .18 },
    'nu-metal':{ density: 0.60, octave: 4, bassOctave: 2, progression: [0,6,5,4], sync: .70 }
  };

  const LAYER_NAMES = ['bass', 'chords', 'melody'];

  // Per-style musical parameters that shape the three layers.
  const CHORD_DURATIONS = {ambient: 3.8, 'nu-metal': 2, default: 3.5};
  const SEVENTH_CHORD_STYLES = new Set(['r&b', 'ambient']);
  const BASS_RHYTHMS = {
    ambient: [0, 2],
    'nu-metal': [0, .5, 1, 2, 2.5, 3],
    default: [0, 1.5, 2.5, 3.5]
  };

  // Melodic contour parameters (degree offsets are relative to the current degree).
  const PROGRESSION_SHIFT_PROBABILITY = .15;
  const CHORD_DELAY_PROBABILITY = .2;
  const BASS_SKIP_PROBABILITY = .19;
  const BASS_PASSING_PROBABILITY = .35;
  const MELODY_INTERVALS = [-2, -1, 0, 0, 0, 1, 2, 4];
  const MELODY_MAX_LEAP = 4;
  const MELODY_LEAP_RECOVERY = 3;

  // ---------------------------------------------------------------------------
  // Deterministic pseudo-random generator (xorshift32)
  // ---------------------------------------------------------------------------

  function rng(seed) {
    let state = (Number(seed) >>> 0) || 1;
    return function () {
      state ^= state << 13; state ^= state >>> 17; state ^= state << 5;
      return (state >>> 0) / 4294967296;
    };
  }

  const clip = (x, low, high) => Math.max(low, Math.min(high, x));

  function hashString(str) {
    let h = 2166136261;
    for (const ch of String(str)) { h ^= ch.charCodeAt(0); h = Math.imul(h, 16777619); }
    return h >>> 0;
  }

  // ---------------------------------------------------------------------------
  // Music theory helpers
  // ---------------------------------------------------------------------------

  // Maps a scale degree (may exceed 0..6) to a MIDI pitch within the given octave.
  function midiPitch(degree, octave, root, scale) {
    const idx = ((degree % 7) + 7) % 7;
    const oct = Math.floor(degree / 7);
    return clip((octave + 1) * 12 + root + scale[idx] + oct * 12, 0, 127);
  }

  function chordTones(degree, octave, root, scale, seventh) {
    const intervals = seventh ? [0, 2, 4, 6] : [0, 2, 4];
    return intervals.map(i => midiPitch(degree + i, octave, root, scale));
  }

  function note(pitch, start, duration, velocity, layer) {
    return {
      pitch: clip(Math.round(pitch), 0, 127),
      start: Math.round(start * 1000) / 1000,
      duration: Math.round(clip(duration, .0625, 32) * 1000) / 1000,
      velocity: clip(Math.round(velocity), 1, 127),
      layer
    };
  }

  // ---------------------------------------------------------------------------
  // Offline generation
  // ---------------------------------------------------------------------------

  function validateGenerationOptions(key, mode, style, bpm, bars) {
    if (!Object.hasOwn(ROOTS, key)) throw new Error('Unknown musical key: ' + key);
    if (!Object.hasOwn(SCALES, mode)) throw new Error('Unknown scale: ' + mode);
    if (!Object.hasOwn(STYLES, style)) throw new Error('Unknown style: ' + style);
    if (!Number.isFinite(Number(bpm)) || bpm < 40 || bpm > 240) throw new Error('BPM must be 40..240');
    if (![4, 8].includes(Number(bars))) throw new Error('Bars must be 4 or 8');
  }

  // Occasionally raises a progression degree by one to keep loops from repeating exactly.
  function shiftedProgression(progression, random) {
    return progression.map(x => (x + (random() < PROGRESSION_SHIFT_PROBABILITY ? 1 : 0)) % 7);
  }

  function chordDurationFor(style) {
    return typeof style === 'string' && Object.hasOwn(CHORD_DURATIONS, style)
      ? CHORD_DURATIONS[style] : CHORD_DURATIONS.default;
  }

  function bassRhythmFor(style) {
    return typeof style === 'string' && Object.hasOwn(BASS_RHYTHMS, style)
      ? BASS_RHYTHMS[style] : BASS_RHYTHMS.default;
  }

  // The random() call order inside each add*Layer function is part of the
  // engine's determinism contract: layers are filled bar by bar, chords first,
  // then bass, then melody. Do not reorder the calls.

  function addChordLayer(chords, {degree, barStart, style, root, scale, totalBeats}, random) {
    const seventh = SEVENTH_CHORD_STYLES.has(style);
    const duration = chordDurationFor(style);
    chordTones(degree, 3, root, scale, seventh).forEach((pitch, idx) => {
      const start = barStart + (random() < CHORD_DELAY_PROBABILITY ? .25 : 0);
      const velocity = 66 + Math.floor(random() * 24);
      chords.push(note(pitch, start, Math.min(duration - idx * .06, totalBeats - start), velocity, 'chords'));
    });
  }

  function addBassLayer(bass, {degree, barStart, style, preset, root, scale}, random) {
    const bassRoot = midiPitch(degree, preset.bassOctave, root, scale);
    const rhythm = bassRhythmFor(style);
    for (let i = 0; i < rhythm.length; i++) {
      if (i > 0 && random() < BASS_SKIP_PROBABILITY) continue;
      const start = barStart + rhythm[i];
      const next = rhythm[i + 1] ?? 4;
      // The final beat may jump to the chord's fifth (degree + 4) as a passing note.
      const passing = i === rhythm.length - 1 && random() < BASS_PASSING_PROBABILITY ? 7 : 0;
      const pitch = passing ? midiPitch((degree + 4) % 7, preset.bassOctave, root, scale) : bassRoot;
      const duration = Math.min(next - rhythm[i], 1.1) * .84;
      const velocity = 78 + Math.floor(random() * 35);
      bass.push(note(pitch, start, duration, velocity, 'bass'));
    }
  }

  function addMelodyLayer(melody, {degree, barStart, preset, root, scale, progression, bar}, random) {
    let previousDegree = degree + 7;
    for (let step = 0; step < 8; step++) {
      if (random() > preset.density) continue;
      const start = barStart + step * .5 + (random() < preset.sync ? .125 : 0);
      // Mostly follow the current chord, sometimes lean towards the next one.
      let candidate = (random() < .7 ? degree : progression[(bar + 1) % progression.length]) + 7;
      candidate += MELODY_INTERVALS[Math.floor(random() * 8)];
      // Limit leaps to keep the motif singable.
      if (Math.abs(candidate - previousDegree) > MELODY_MAX_LEAP) {
        candidate = previousDegree + Math.sign(candidate - previousDegree) * MELODY_LEAP_RECOVERY;
      }
      previousDegree = candidate;
      const length = random() < .2 ? 1 : random() < .4 ? .5 : .25;
      const pitch = midiPitch(candidate, preset.octave - 1, root, scale);
      const velocity = 64 + Math.floor(random() * 46);
      melody.push(note(pitch, start, Math.min(length, barStart + 4 - start), velocity, 'melody'));
    }
  }

  function generateOne(opts, seed) {
    const {key = 'C', mode = 'minor', style = 'trap', bpm = 100, bars = 4} = opts;
    validateGenerationOptions(key, mode, style, bpm, bars);
    const root = ROOTS[key];
    const scale = SCALES[mode];
    const preset = STYLES[style];
    const random = rng(seed);
    const layers = {bass: [], chords: [], melody: []};
    const progression = shiftedProgression(preset.progression, random);
    const totalBeats = bars * 4;
    for (let bar = 0; bar < bars; bar++) {
      const context = {degree: progression[bar % progression.length], barStart: bar * 4,
        style, preset, root, scale, progression, bar, totalBeats};
      addChordLayer(layers.chords, context, random);
      addBassLayer(layers.bass, context, random);
      addMelodyLayer(layers.melody, context, random);
    }
    return {metadata: {key, mode, style, bpm: Number(bpm), bars: Number(bars), seed: seed >>> 0}, layers};
  }

  // Derived per-idea seeds are spread with the golden-ratio multiplier so that
  // neighbouring seeds produce musically unrelated ideas.
  const SEED_SPREAD_MULTIPLIER = 2654435761;
  const SEED_SPACE = 4294967296;

  function generateEight(opts, baseSeed) {
    const seed = baseSeed === undefined ? Math.floor(Math.random() * SEED_SPACE) : baseSeed >>> 0;
    return Array.from({length: 8}, (_, i) => generateOne(opts, (seed + Math.imul(i + 1, SEED_SPREAD_MULTIPLIER)) >>> 0));
  }

  function transposePattern(pattern, semitones) {
    const n = Number(semitones) || 0;
    return {...pattern, layers: Object.fromEntries(Object.entries(pattern.layers).map(([name, notes]) =>
      [name, notes.map(x => ({...x, pitch: clip(Math.round(x.pitch + n), 0, 127)}))]))};
  }

  // ---------------------------------------------------------------------------
  // MIDI encoding (format 1)
  // ---------------------------------------------------------------------------

  const NOTE_OFF_STATUS = 0x80;
  const NOTE_ON_STATUS = 0x90;
  const META_EVENT = 255;
  const META_TEMPO = 81;       // 0x51 set tempo
  const META_TIME_SIGNATURE = 88; // 0x58
  const META_TRACK_NAME = 3;   // 0x03
  const META_TEXT = 1;         // 0x01
  const END_OF_TRACK_BYTES = [0, META_EVENT, 47, 0]; // delta 0, FF 2F 00

  function variableLengthQuantity(value) {
    let n = Math.max(0, Math.round(value));
    const out = [n & 127];
    while ((n >>= 7) > 0) out.unshift((n & 127) | 128);
    return out;
  }

  function bigEndianBytes(value, bytes) {
    return Array.from({length: bytes}, (_, i) => (value >>> ((bytes - i - 1) * 8)) & 255);
  }

  const utf8 = (s) => Array.from(new TextEncoder().encode(s));

  function chunkBytes(id, bytes) {
    return [...id.split('').map(x => x.charCodeAt(0)), ...bigEndianBytes(bytes.length, 4), ...bytes];
  }

  function track(events) {
    const sorted = events.slice().sort((a, b) => a.tick - b.tick || a.priority - b.priority);
    let lastTick = 0;
    const out = [];
    for (const ev of sorted) {
      out.push(...variableLengthQuantity(ev.tick - lastTick), ...ev.bytes);
      lastTick = ev.tick;
    }
    out.push(...END_OF_TRACK_BYTES);
    return chunkBytes('MTrk', out);
  }

  function metaTrack(bpm, durationTicks) {
    const tempo = Math.round(60000000 / bpm);
    return track([
      {tick: 0, priority: 0, bytes: [META_EVENT, META_TEMPO, 3, ...bigEndianBytes(tempo, 3)]},
      {tick: 0, priority: 1, bytes: [META_EVENT, META_TIME_SIGNATURE, 4, 4, 2, 24, 8]},
      {tick: 0, priority: 2, bytes: [META_EVENT, META_TRACK_NAME, 9, ...utf8('LoopForge')]},
      {tick: durationTicks, priority: 4, bytes: [META_EVENT, META_TEXT, 0]}
    ]);
  }

  // Converts layer notes into on/off edges clamped to the loop duration.
  function noteEdges(layerNotes, {transpose, durationTicks}) {
    const edges = [];
    for (const n of layerNotes || []) {
      const start = clip(Math.round(n.start * PPQ), 0, durationTicks - 1);
      const end = clip(Math.round((n.start + n.duration) * PPQ), start + 1, durationTicks);
      const pitch = clip(Math.round(n.pitch + transpose), 0, 127);
      edges.push({tick: start, pitch, velocity: clip(Math.round(n.velocity), 1, 127), on: true});
      edges.push({tick: end, pitch, velocity: 0, on: false});
    }
    // Off before on at equal ticks; stable order keeps simultaneous-unison velocity last-wins.
    edges.sort((a, b) => a.tick - b.tick || (a.on === b.on ? 0 : a.on ? 1 : -1));
    return edges;
  }

  // Builds one MIDI track from the given layers. Each layer keeps its own
  // channel (its index in LAYER_NAMES) and its own hold/unison state; only the
  // event stream itself is shared. Overlapping same-pitch notes within a layer
  // get an early note-off, simultaneous unisons within a layer share one onset
  // whose velocity is the last one seen.
  function buildLayerTrack(selectedNames, pattern, {transpose, durationTicks}) {
    const events = [];
    for (const name of selectedNames) {
      const channel = LAYER_NAMES.indexOf(name);
      const held = new Array(128).fill(0);
      const onsetTick = new Array(128).fill(-1);
      const onsetIndex = new Array(128).fill(0);
      for (const edge of noteEdges(pattern.layers[name], {transpose, durationTicks})) {
        const pitch = edge.pitch;
        if (edge.on) {
          if (onsetTick[pitch] === edge.tick) {
            // Simultaneous unisons share one audible onset; retain every lifetime.
            events[onsetIndex[pitch]].bytes[2] = edge.velocity;
          } else {
            if (held[pitch]) {
              events.push({tick: edge.tick, priority: 2, bytes: [NOTE_OFF_STATUS | channel, pitch, 0]});
            }
            onsetTick[pitch] = edge.tick;
            onsetIndex[pitch] = events.length;
            events.push({tick: edge.tick, priority: 3, bytes: [NOTE_ON_STATUS | channel, pitch, edge.velocity]});
          }
          ++held[pitch];
        } else if (--held[pitch] === 0) {
          events.push({tick: edge.tick, priority: 1, bytes: [NOTE_OFF_STATUS | channel, pitch, 0]});
        }
      }
    }
    return track(events);
  }

  function createMidi(pattern, {split = true, transpose = 0} = {}) {
    const {bpm = 100, bars = 4} = pattern.metadata;
    const durationTicks = Math.round(bars * 4 * PPQ);
    const meta = metaTrack(bpm, durationTicks);
    const layout = split ? LAYER_NAMES.map(name => [name]) : [LAYER_NAMES];
    const tracks = [meta, ...layout.map(names => buildLayerTrack(names, pattern, {transpose, durationTicks}))];
    const header = chunkBytes('MThd', [...bigEndianBytes(1, 2), ...bigEndianBytes(tracks.length, 2), ...bigEndianBytes(PPQ, 2)]);
    return new Uint8Array([...header, ...tracks.flat()]);
  }

  return {PPQ, SCALES, ROOTS, STYLES, generateOne, generateEight, createMidi, transposePattern, hashString};
});
