﻿// Copyright © 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "Core/Engine.h"
#include "TESDataHandlerHook.h"
#include "Editor API/FO4/TESDataHandler.h"

namespace CreationKitPlatformExtended
{
	namespace Patches
	{
		namespace Fallout4
		{
			TESDataHandlerPatch::TESDataHandlerPatch() : Module(GlobalEnginePtr)
			{}

			bool TESDataHandlerPatch::HasOption() const
			{
				return false;
			}

			bool TESDataHandlerPatch::HasCanRuntimeDisabled() const
			{
				return false;
			}

			const char* TESDataHandlerPatch::GetOptionName() const
			{
				return nullptr;
			}

			const char* TESDataHandlerPatch::GetName() const
			{
				return "TESDataHandler";
			}

			bool TESDataHandlerPatch::HasDependencies() const
			{
				return false;
			}

			Array<String> TESDataHandlerPatch::GetDependencies() const
			{
				return {};
			}

			bool TESDataHandlerPatch::QueryFromPlatform(EDITOR_EXECUTABLE_TYPE eEditorCurrentVersion,
				const char* lpcstrPlatformRuntimeVersion) const
			{
				return eEditorCurrentVersion <= EDITOR_EXECUTABLE_TYPE::EDITOR_FALLOUT_C4_LAST;
			}

			bool TESDataHandlerPatch::Activate(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem)
			{
				if (lpRelocationDatabaseItem->Version() == 1)
				{
					TESDataHandler::Instance = (TESDataHandler*)lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(0));

					return true;
				}

				return false;
			}

			bool TESDataHandlerPatch::Shutdown(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem)
			{
				return false;
			}
		}
	}
}