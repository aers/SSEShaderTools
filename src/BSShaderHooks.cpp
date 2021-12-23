#include "BSShader.h"
#include "ShaderCompiler.h"

namespace BSShaderHooks
{
	typedef void (*LoadShaders_t)(REX::BSShader* shader, std::uintptr_t stream);
	REL::Relocation<LoadShaders_t*> LoadShaders{ REL::ID(108326) };
	LoadShaders_t oLoadShaders;

    void hk_LoadShaders(REX::BSShader* bsShader, std::uintptr_t stream)
	{
		oLoadShaders(bsShader, stream);

		if (strcmp("Lighting", bsShader->m_LoaderType) == 0) {
			logger::info("BSShader::LoadShaders called on {}, {} - ps count {}"sv, bsShader->m_Type, bsShader->m_LoaderType, bsShader->m_PixelShaderTable.size());

			std::unordered_map<REX::TechniqueID, std::wstring> techniqueFileMap;

			const auto shaderDir = std::filesystem::current_path() /= "Data/SKSE/plugins/shaders/"sv;

			logger::info("{}", shaderDir.generic_string());

			std::size_t foundCount = 0;
			std::size_t successCount = 0;
			std::size_t failedCount = 0;

			for (const auto& entry : std::filesystem::directory_iterator(shaderDir)) {
				if (entry.path().extension().generic_string() != ".hlsl"sv)
					continue;

				auto filenameStr = entry.path().filename().string();
				auto techniqueIdStr = filenameStr.substr(0, filenameStr.find('_'));
				const REX::TechniqueID techniqueId = std::strtoul(techniqueIdStr.c_str(), nullptr, 16);
				logger::info("found shader technique id {:08x} with path {}"sv, techniqueId, entry.path().generic_string());
				foundCount++;
				techniqueFileMap.insert(std::make_pair(techniqueId, absolute(entry.path()).wstring()));
			}

			for (const auto& entry : bsShader->m_PixelShaderTable) {
				auto tFileIt = techniqueFileMap.find(entry->m_TechniqueID);
				if (tFileIt != techniqueFileMap.end()) {

                    if (const auto shader = ShaderCompiler::CompileAndRegisterPixelShader(tFileIt->second)) {
						logger::info("shader compiled successfully, replacing old shader"sv);
						successCount++;
						entry->m_Shader = shader;
					} else {
						failedCount++;
					}
				}
			}

			 logger::info("found shaders: {} successfully replaced: {} failed to replace: {}", foundCount, successCount, failedCount);
		}
	}

	void Install()
	{
		logger::info("installing BSShader::LoadShaders hook"sv);
		{
		    struct Patch : Xbyak::CodeGenerator
		    {
				Patch()
				{
					Xbyak::Label origFuncJzLabel;

					test(rdx, rdx);
					jz(origFuncJzLabel);
					jmp(ptr[rip]);
					dq(LoadShaders.address() + 0x9);

					L(origFuncJzLabel);
					jmp(ptr[rip]);
					dq(LoadShaders.address() + 0xF0);
				}
		    };

			Patch p;
			p.ready();

			auto& trampoline = SKSE::GetTrampoline();
			oLoadShaders = static_cast<LoadShaders_t>(trampoline.allocate(p));
			trampoline.write_branch<6>(
				LoadShaders.address(),
				hk_LoadShaders);
		}
		logger::info("installed"sv);
	}


}
