#pragma once
#include "Direction.h"
#include "CompositionJson.h"
namespace composer {
inline constexpr std::array<Field,7> recipeFields{Field::darkness,Field::energy,Field::melancholy,Field::tension,Field::aggression,Field::atmosphere,Field::complexity};
inline constexpr std::array<int Mood::*,7> recipeMembers{&Mood::darkness,&Mood::energy,&Mood::melancholy,&Mood::tension,&Mood::aggression,&Mood::atmosphere,&Mood::complexity};
struct GenerationRecipe {std::string direction;uint32_t seed=42;ManualOverrides manual;};
struct RecipeReadResult {std::optional<GenerationRecipe> recipe;std::string error;};
inline CompositionJsonResult encodeRecipeJson(const GenerationRecipe& recipe){
 try{
  if(recipe.direction.size()>4096)throw jsonDetail::Invalid{};
  std::bitset<fieldCount> supported;unsigned pins=0;
  for(size_t i=0;i<7;++i){supported.set(size_t(recipeFields[i]));if(recipe.manual.pinned[size_t(recipeFields[i])])pins|=1U<<i;int v=recipe.manual.values.mood.*recipeMembers[i];if(v<0||v>100)throw jsonDetail::Invalid{};}
  if((recipe.manual.pinned&~supported).any())throw jsonDetail::Invalid{};
  jsonDetail::SchemaWriter w;w.raw("{");w.field("version",1);w.field("kind",std::string_view("loopforge-recipe"));w.field("engineVersion",5);w.field("direction",recipe.direction);w.field("seed",int64_t(recipe.seed));w.field("pins",int(pins));
  w.key("mood");w.raw("[");for(size_t i=0;i<7;++i){if(i)w.raw(",");w.integer(recipe.manual.values.mood.*recipeMembers[i]);}w.raw("]}");
  if(w.output.size()>32768)throw jsonDetail::Invalid{};
  return {std::move(w.output),{}};
 }catch(const jsonDetail::Invalid&){return {{},"Invalid recipe or unsupported manual fields"};}catch(const std::bad_alloc&){return {{},"Insufficient memory to save recipe"};}
}
inline RecipeReadResult decodeRecipeJson(std::string_view text){
 if(text.empty()||text.size()>32768)return {{},"Recipe must be between1byte and32KiB"};
 try{
  jsonDetail::Reader r{text};GenerationRecipe recipe;unsigned pins=0;
  r.object({"version","kind","engineVersion","direction","seed","pins","mood"},[&](size_t i){switch(i){
   case 0:if(r.integer()!=1)throw jsonDetail::Invalid{};break;
   case 1:if(r.string(32)!="loopforge-recipe")throw jsonDetail::Invalid{};break;
   case 2:if(r.integer()!=5)throw jsonDetail::Invalid{};break;
   case 3:recipe.direction=r.string(4096);break;
   case 4:recipe.seed=uint32_t(r.integer(0,UINT32_MAX));break;
   case 5:pins=unsigned(r.integer(0,127));break;
   case 6:{auto mood=r.array<int>(7,[&]{return int(r.integer(0,100));});if(mood.size()!=7)throw jsonDetail::Invalid{};for(size_t n=0;n<7;++n)recipe.manual.values.mood.*recipeMembers[n]=mood[n];break;}
  }});
  if(!r.done())throw jsonDetail::Invalid{};
  for(size_t i=0;i<7;++i)recipe.manual.pinned.set(size_t(recipeFields[i]),(pins&(1U<<i))!=0);
  return {std::move(recipe),{}};
 }catch(const jsonDetail::Invalid&){return {{},"Invalid or unsupported recipe JSON"};}catch(const std::bad_alloc&){return {{},"Insufficient memory to open recipe"};}
}
}
