#define MMDPLUGIN_D3DX9_HOOK
#include "mmdplugin/mmd_plugin.h"
#include <vector>

class MMEHookSample;

namespace
{
  MMEHookSample* self;
  mmp::HookEffectBeginPF oridinal_begin;
}

HRESULT CHookBeginFunc(LPD3DXEFFECT effect, UINT *pPasses, DWORD Flags);


class MMEHookSample : public MMDPluginDLL4
{
private:

  struct Effect
  {
    Effect(LPD3DXEFFECT e) :effect(e)
    {
      effect->AddRef();
    }
    Effect(const Effect& e)
      :effect(e.effect), kind(e.kind), my_semantic_handle(e.my_semantic_handle)
    {
      effect->AddRef();
    }
    ~Effect()
    {
      effect->Release();
    }

    LPD3DXEFFECT effect;
    int kind;
    D3DXHANDLE my_semantic_handle;

    bool operator<(const Effect& r)const { return effect < r.effect; }
  };

  std::vector<Effect> _effects;
public:

  const char* getPluginTitle() const override { return "MME_Hook_Sample"; }


  void D3DXCreateEffectFromFileExW(LPDIRECT3DDEVICE9& pDevice, LPCWSTR& pSrcFile, const D3DXMACRO*& pDefines, LPD3DXINCLUDE& pInclude, LPCSTR& pSkipConstants, DWORD& Flags, LPD3DXEFFECTPOOL& pPool, LPD3DXEFFECT*& ppEffect, LPD3DXBUFFER*& ppCompilationErrors) override;
  void PostD3DXCreateEffectFromFileExW(LPDIRECT3DDEVICE9& pDevice, LPCWSTR& pSrcFile, const D3DXMACRO*& pDefines, LPD3DXINCLUDE& pInclude, LPCSTR& pSkipConstants, DWORD& Flags, LPD3DXEFFECTPOOL& pPool, LPD3DXEFFECT*& ppEffect, LPD3DXBUFFER*& ppCompilationErrors, HRESULT& res) override;
  void EndScene() override;
  HRESULT HookBeginFunc(LPD3DXEFFECT effect, UINT* p_passes, DWORD flags);
};


void MMEHookSample::D3DXCreateEffectFromFileExW(LPDIRECT3DDEVICE9& pDevice, LPCWSTR& pSrcFile, const D3DXMACRO*& pDefines,
                                                LPD3DXINCLUDE& pInclude, LPCSTR& pSkipConstants, DWORD& Flags, LPD3DXEFFECTPOOL& pPool,
                                                LPD3DXEFFECT*& ppEffect, LPD3DXBUFFER*& ppCompilationErrors)
{
  // should be static or member variable.
  static std::vector<D3DXMACRO> macros;

  macros.clear();
  for ( auto it = pDefines; it && it->Definition && it->Name; it++ )
  {
    macros.push_back(*it);
  }


  D3DXMACRO myMacro;
  myMacro.Name = "MME_HOOK_SAMPLE";
  myMacro.Definition = "1";
  macros.push_back(myMacro);

  D3DXMACRO endData;
  myMacro.Name = nullptr;
  myMacro.Definition = nullptr;
  macros.push_back(myMacro);

  // do not local pointer.
  pDefines = macros.data();
}

void MMEHookSample::PostD3DXCreateEffectFromFileExW(LPDIRECT3DDEVICE9& pDevice, LPCWSTR& pSrcFile, const D3DXMACRO*& pDefines,
                                                    LPD3DXINCLUDE& pInclude, LPCSTR& pSkipConstants, DWORD& Flags, LPD3DXEFFECTPOOL& pPool,
                                                    LPD3DXEFFECT*& ppEffect, LPD3DXBUFFER*& ppCompilationErrors, HRESULT& res)
{
  if ( FAILED(res) || ppEffect == nullptr || *ppEffect == nullptr )
  {
    return;
  }

  auto d3d_effect = *ppEffect;



  int kind = 0;
  if ( FAILED(d3d_effect->GetInt(d3d_effect->GetParameterBySemantic(nullptr, "MMEHookSample_KIND"), &kind)) )
  {
    return;
  }

  oridinal_begin = mmp::HookEffectBegin(d3d_effect, &CHookBeginFunc);

  Effect effect(d3d_effect);
  effect.kind = kind;
  effect.my_semantic_handle = d3d_effect->GetParameterBySemantic(nullptr, "MY_SEMANTIC");
  _effects.push_back(effect);
  std::sort(_effects.begin(), _effects.end());
}

void MMEHookSample::EndScene()
{

  /* It is checked whether it has been deleted or not */
  for ( auto it = _effects.begin(); it != _effects.end();)
  {
    auto count = it->effect->AddRef();
    it->effect->Release();
    if ( count == 2 )
    {
      it = _effects.erase(it);
    }
    else
    {
      ++it;
    }
  }

  /*
   * Processing order.
   *
   * D3D9Device::BeginScene()
   * for (charctor-model and accessory-model)
   *   D3DX9Effect::SetXXX() etc...
   *   D3DX9Effect::Begin()           // hook-point1:
   *   D3D9Device::DrawXXX()
   * D3D9Device::EndScene()                // hook-point2:
   * UpdateBone()
   * D3D9Device::Present()
   */

   /*
    * We can use ExpGetXXX() that MMD standard function, We do not know which effects are applied.
    * We use data set by MME to acquire data on MMD, effect->GetXXX(). (TODO: Check if it can actually be acquired)
    *
    * There are two timings that can be acquired.
    * hook-point1:
    *   If you get here, the process is blocked.
    * hook-point2:
    *   When hooking with EndScene(), data delayed by 1 frame is used but it can be executed asynchronously until Begin() is reached.
    *   When implementing, it is conceivable to use std::async.
    *
    *
    * This sample  hooks point1.
    */
}

HRESULT MMEHookSample::HookBeginFunc(LPD3DXEFFECT effect, UINT* p_passes, DWORD flags)
{
  auto& my_effect = *std::lower_bound(_effects.begin(), _effects.end(), Effect{ effect });

  if ( my_effect.kind == 0 )
  {
    float red[] = { 1,0,0,1 };
    effect->SetFloatArray(my_effect.my_semantic_handle, red, 4);
  }
  else
  {
    float blue[] = { 0,0,1,1 };
    effect->SetFloatArray(my_effect.my_semantic_handle, blue, 4);
  }
  return oridinal_begin(effect, p_passes, flags);
}


HRESULT WINAPI CHookBeginFunc(LPD3DXEFFECT effect, UINT *pPasses, DWORD Flags)
{
  return self->HookBeginFunc(effect, pPasses, Flags);
}


int version() { return 4; }

MMDPluginDLL4* create4(IDirect3DDevice9* device)
{
  return self = new MMEHookSample();
}

void destroy4(MMDPluginDLL4* p)
{
  delete p;
}

