# LoopForge instrument interface

## Reading the four supplied references

The first image shows the working hierarchy. A short command row sits above a wide colour field. Track selectors touch the dark piano roll. The lower edge holds export and timing controls. Most of the frame is quiet enough that notes and the current selection carry the contrast.

The second image repeats that hierarchy at promotional scale. Its headline and room backdrop belong to the advertisement. The third image changes the shell to white and the field to blue while preserving the instrument layout. The fourth crop shows why this works at small sizes: long, bright note gates remain legible against alternating dark grid cells.

## Implementation

Use a graphite shell, pale type and LoopForge lime for focus and primary actions. Build the field from original nested bands of forest, teal, chartreuse and ivory. An off-centre core gives it direction. Keep it static and render it locally. The field provides visual context for the selected loop. Controls with a real action use buttons.

In Standalone and VST3, centre the direction/seed/compose/transport row. Keep the MIDI timing overview and track buttons on a dark surface within the field, with the tempo at its core. Preserve piano-roll, overview, settings and export entry points.

On the web, replace the dashboard sidebar and promotional hero with an instrument header. Put generation controls above the field, keep the selected key, scale and tempo on the field, then place eight compact idea selectors directly above the editor. Keep the editor's keyboard controls, focus outline and scrollable pitch grid. At narrow widths wrap controls and idea selectors in reading order.

## Acceptance

Build the JUCE editor for both native formats and exercise its editor test. Check the browser at 320, 390, 768 and 1440 CSS pixels, including keyboard note edits, download and accessibility checks. Colour-field artwork must not intercept input. Project format, MIDI timing and audio rendering are independent of this presentation change.

## Implemented views

- [Native JUCE editor with a generated four-track loop](../../images/instrument/native-editor.png)
- [Browser editor with the chords layer selected](../../images/instrument/browser-editor.png)
- [Browser composer at 390 CSS pixels](../../images/instrument/mobile-editor.png)
