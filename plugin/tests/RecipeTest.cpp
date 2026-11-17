#include "../Source/engine/RecipeJson.h"
#include "../Source/engine/PromptCompose.h"
#include <iostream>
#define check(x) do{if(!(x)){std::cerr<<__LINE__<<": " #x "\n";std::abort();}}while(false)
using namespace composer;
int main(){
 GenerationRecipe recipe;recipe.direction="4 bars energy100";recipe.seed=UINT32_MAX;recipe.manual.values.mood.energy=0;recipe.manual.values.mood.darkness=73;recipe.manual.pinned.set(size_t(Field::energy));
 auto bytes=encodeRecipeJson(recipe);check(bytes&&bytes.json.size()<32768);auto read=decodeRecipeJson(bytes.json);check(read.recipe&&read.recipe->seed==UINT32_MAX&&read.recipe->direction==recipe.direction);
 check(read.recipe->manual.pinned==recipe.manual.pinned&&read.recipe->manual.values.mood.darkness==73&&read.recipe->manual.values.mood.energy==0);
 auto a=generateResolvedLoop(LocalPromptInterpreter{}.interpret(recipe.direction,recipe.seed,recipe.manual));auto b=generateResolvedLoop(LocalPromptInterpreter{}.interpret(read.recipe->direction,read.recipe->seed,read.recipe->manual));check(a.composition&&b.composition&&encodeCompositionJson(*a.composition).json==encodeCompositionJson(*b.composition).json);
 for(auto bad:{std::string("{}"),std::string("not JSON"),bytes.json+"{}",std::string(32769,' ')})check(!decodeRecipeJson(bad).recipe);
 for(auto change: {std::pair{"\"version\":1","\"version\":2"},std::pair{"\"seed\":4294967295","\"seed\":4294967296"},std::pair{"\"pins\":2","\"pins\":128"},std::pair{"[73,0,50","[73,101,50"}}){auto bad=bytes.json;auto at=bad.find(change.first);check(at!=std::string::npos);bad.replace(at,std::string(change.first).size(),change.second);check(!decodeRecipeJson(bad).recipe);}
 recipe.manual.pinned.set(size_t(Field::bpm));check(!encodeRecipeJson(recipe));recipe.manual.pinned.reset();recipe.direction=std::string(4097,'a');check(!encodeRecipeJson(recipe));
 recipe.direction="4 такта минор";check(decodeRecipeJson(encodeRecipeJson(recipe).json).recipe);
 std::cout<<"recipe roundtrip, deterministic regeneration and strict bounds passed\n";
}
