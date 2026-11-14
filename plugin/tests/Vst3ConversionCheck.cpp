#include "Vst3ConversionCheck.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_processors/format_types/juce_VST3Headers.h>
#include <juce_audio_processors/utilities/juce_FlagCache.h>
#include <juce_audio_processors/format_types/juce_VST3Common.h>
bool allPacketsSurviveVst3(juce::MidiBuffer& midi){
    juce::MidiEventList delivered;juce::MidiEventList::pluginToHostEventList(delivered,midi);
    return delivered.getEventCount()==midi.getNumEvents();
}
