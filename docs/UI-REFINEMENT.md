# UI refinement and Modern Web Guidance

The user requested both the browser and native JUCE interfaces. Dark studio styling and existing musical/provider/export contracts are preserved. task.md remains the progress source; no release/DAW acceptance is implied.

## Installed skill

Official GoogleChrome/modern-web-guidance source pinned at4dd968349e54ca45a05dd5c818fb035c9054b022, native Codex plugin modern-web-guidance@googlechrome1.0.0 installed/enabled in the environment. The cached SKILL.md matches the reviewed source. This confirms the native CLI installation, not a separate Codex App plugin connection. No hooks or runtime app dependency were added. Searches/retrieval used DISABLE_TELEMETRY=1 and a temporary npm cache.

Guides consulted, with original documentation licensed CC-BY4.0:
- [responsive-design](https://github.com/GoogleChrome/modern-web-guidance/blob/4dd968349e54ca45a05dd5c818fb035c9054b022/skills/modern-web-guidance/guides/css/responsive-design.md): dynamic viewport units with vh fallback, rem-bounded fluid type and component container rules.
- [navbar](https://github.com/GoogleChrome/modern-web-guidance/blob/4dd968349e54ca45a05dd5c818fb035c9054b022/skills/modern-web-guidance/guides/ui-components/navbar.md): one semantic nav tree and current-location indication. Three short links fit/wrap, so no overlay, unsupported anchor positioning or popover polyfill is introduced.
- [accessibility](https://github.com/GoogleChrome/modern-web-guidance/blob/4dd968349e54ca45a05dd5c818fb035c9054b022/skills/modern-web-guidance/guides/accessibility/accessibility.md): native controls, explicit labels/descriptions, selected states, keyboard/focus preservation and visible outlines. The dense2D note graphic keeps its editing gesture; labeled precision controls provide an alternative to precision dragging.

Browser APIs do not apply to JUCE. Their interaction principles informed native labels/titles, logical focus order, keyboard actions and a resizable editing area using JUCE's own APIs.

## Browser usage

Studio, Piano roll and AI connection navigation remain available on mobile. Select an idea; keyboard focus stays on its card. The piano surface scrolls in both directions with keys/notes synchronized; external font downloads are no longer required.

Select a note through the labeled Note selector, click it, or tab into the selected native note button. Arrows move by a semitone/quarter beat; Shift+Left/Right resizes. Delete removes a note, preserving at least one note in the whole idea. Add/Delete controls avoid double-click/Alt precision gestures. The inspector accepts0.001beat timing precision, matching the existing core/server schema. Derived limits avoid binary-float rejection of valid end-boundary notes. Empty layers focus Add note rather than a disabled selector.

Ctrl/Alt/Meta navigation is not interpreted as a musical edit. Pointer cancellation or a layer/idea replacement rolls back the unfinished gesture. Generation remains focusable but guarded while busy, so repeated Enter does not send duplicates. Stopping preview clears the old timeout so it cannot stop a newer preview.

## Native usage

The window can expand from1000x740 to1600x1100; the roll uses the extra space. Fields have visible labels and accessible titles. Tab to the piano roll; brackets select previous/next notes, Home/End select the first/last, arrows move, Shift+Left/Right resizes and Delete removes. Add/Delete controls are available beside undo/redo. Modified navigation keys pass through. A keyboard event following composition replacement does not apply an old selection to new data. Successful mouse/keyboard edits preserve the current selection.

## Reproduce browser verification

Runtime remains dependency-free. Browser tools are isolated verification dependencies:

```sh
npm install --prefix /tmp/loopforge-ui-tools --ignore-scripts --no-audit --no-fund playwright@1.62.1 axe-core@4.14.0
/tmp/loopforge-ui-tools/node_modules/.bin/playwright install chromium
LOOPFORGE_PORT=4180 npm start
# In another terminal; CHROMIUM_PATH may select an already installed browser.
NODE_PATH=/tmp/loopforge-ui-tools/node_modules LOOPFORGE_AXE=1 node scripts/verify-web-ui.cjs
```

The script tests320/390/768/1440px reflow, native card activation/focus, pending-generation focus and duplicate protection, precision/keyboard notes, cancelled drag, tail limits, empty-layer focus, last-note retention, preview timer isolation, actual downloaded type1 MIDI, reduced motion and forced-colour focus. Optional LOOPFORGE_UI_SCREENSHOTS saves images. Dedicated Web UI CI installs pinned tools outside app dependencies and runs this script with axe4.14.0.

## Evidence and limits

Local Node42/42 and native CTest12/12 passed, including real native key/resize/control tests; VST3/Standalone build and HTTP scripts passed. Samples remain byte-identical. Browser regressions were observed failing before fixes; a linked old-timeout browser probe also reproduced interruption of a new preview. Axe reports0violations for the selected WCAG2/2.1/2.2 rule tags in the tested states at four widths. This is not a full WCAG certification or a replacement for actual screen-reader testing. Windows/native source-specific CI and browser CI are recorded in task.md after observation; Windows REAPER/OS drag remains unverified under T12.
