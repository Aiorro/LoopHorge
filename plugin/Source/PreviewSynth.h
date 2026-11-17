#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// Fixed-cost audition, independent of the exported MIDI/instrument presets.
//64preview voices may steal; host MIDI remains complete.
class PreviewSynth {
public:
 PreviewSynth()noexcept{prepare(44100);}
 void prepare(double rate)noexcept{if(!std::isfinite(rate)||rate<1||rate>768000)rate=44100;for(int p=0;p<128;++p)increments[size_t(p)]=6.283185307179586*440*std::pow(2.0,(p-69)/12.0)/rate;fast=std::exp(-1/(rate*.025));slow=std::exp(-1/(rate*.08));clear();}
 void clear()noexcept{voices.fill(Voice{});map.fill(-1);damper.fill(false);sostenuto.fill(false);hold2.fill(false);volume.fill(1);expression.fill(1);next=0;noise=0x31415926;}
 void message(uint8_t status,uint8_t pitch,uint8_t value)noexcept{
  if(pitch>127||value>127)return;const size_t ch=status&15,key=ch*128+pitch;const auto type=status&0xf0;
  if(type==0x90&&value){int index=map[key];const bool latched=index>=0&&voices[size_t(index)].latched;if(index<0){for(int i=0;i<64;++i)if(voices[size_t(i)].key>=2048){index=i;break;}if(index<0){index=next;next=(next+1)%64;}kill(voices[size_t(index)]);map[key]=int16_t(index);}voices[size_t(index)]={uint16_t(key),float(value)/127,0,true,latched};return;}
  if(type==0x80||(type==0x90&&!value)){const auto index=map[key];if(index>=0){auto& v=voices[size_t(index)];v.down=false;if(!damper[ch]&&!hold2[ch]&&!v.latched)kill(v);}return;}
  if(type!=0xb0)return;
  if(pitch==7)volume[ch]=float(value)/127;
  if(pitch==11)expression[ch]=float(value)/127;
  if(pitch==64)damper[ch]=value>=64;
  if(pitch==69)hold2[ch]=value>=64;
  if(pitch==66){const bool down=value>=64;for(auto& v:voices)if(v.key/128==ch){if(down&&!sostenuto[ch])v.latched=v.down;if(!down)v.latched=false;}sostenuto[ch]=down;}
  if(pitch==120){for(auto& v:voices)if(v.key/128==ch)kill(v);}
  if(pitch==123){for(auto& v:voices)if(v.key/128==ch)v.down=false;}
  if(pitch==121){damper[ch]=hold2[ch]=sostenuto[ch]=false;volume[ch]=expression[ch]=1;for(auto& v:voices)if(v.key/128==ch)v.latched=false;}
  for(auto& v:voices)if(v.key/128==ch&&!v.down&&!damper[ch]&&!hold2[ch]&&!v.latched)kill(v);
 }
 float sample()noexcept{
  float total=0;for(auto& v:voices){if(v.key>=2048)continue;const size_t ch=v.key/128,pitch=v.key%128;v.phase=std::fmod(v.phase+increments[pitch],6.283185307179586);float wave=float(std::sin(v.phase));
   if(ch==9){noise^=noise<<13;noise^=noise>>17;noise^=noise<<5;if(pitch>36)wave=.25f*wave+.75f*(float(noise&0xffffff)/8388608-1);v.level*=float(pitch==42||pitch==44||pitch==46?fast:slow);}
   total+=wave*v.level*volume[ch]*expression[ch]*.025f;if(v.level<.00001f)kill(v);
  }return std::tanh(total);
 }
private:
 struct Voice{uint16_t key=2048;float level=0;double phase=0;bool down=false,latched=false;};
 std::array<Voice,64> voices;std::array<int16_t,2048> map;std::array<double,128> increments;std::array<bool,16> damper,sostenuto,hold2;std::array<float,16> volume,expression;int next=0;uint32_t noise=0;double fast=0,slow=0;
 void kill(Voice& v)noexcept{if(v.key<2048)map[v.key]=-1;v=Voice{};}
};
