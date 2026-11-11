-- LoopForge MIDI importer for REAPER. Run from Actions > Show action list > Load ReaScript.
-- Choose an exported standard MIDI file. REAPER inserts it at the edit cursor.
local rv, filename = reaper.GetUserFileNameForRead('', 'Choose LoopForge MIDI', '.mid')
if not rv then return end
reaper.Undo_BeginBlock()
reaper.InsertMedia(filename, 0)
reaper.Undo_EndBlock('Import LoopForge MIDI', -1)
reaper.UpdateArrange()
