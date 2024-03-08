// Copyright � 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "Core/Engine.h"
#include "ResponseWindowF4.h"
#include "Editor API/EditorUI.h"
#include "Editor API/BSString.h"
#include "Editor API/FO4/TESTopic.h"
#include "Editor API/FO4/BGSVoiceType.h"
#include "Patches/ConsolePatch.h"

namespace CreationKitPlatformExtended
{
	namespace Patches
	{
		namespace Fallout4
		{
			using namespace EditorAPI;
			using namespace EditorAPI::Fallout4;

			ResponseWindow* GlobalResponseWindowPtr = nullptr;
			bool EnableLipGeneration = false;

			uintptr_t pointer_ResponseWindow_data = 0;
			uintptr_t pointer_ResponseWindow_sub1 = 0;
			uintptr_t pointer_ResponseWindow_sub2 = 0;
			uintptr_t pointer_ResponseWindow_sub3 = 0;

			static const char* V_TOOLPATH = "Tools\\LipGen\\";
			static const char* V_WRAPPER = "FaceFXWrapper.exe";
			static const char* V_FXDATA = "FonixData.cdf";
			static const char* V_CMD_FMT = "\"%s\" Fallout4 USEnglish FonixData.cdf \"%s\" \"%s\" \"%s\"";

			typedef struct RESPONSE_DATA
			{
				const char* Title;
				uint64_t UnkParm;
				TESTopicInfo::Response* ResponseSource;
				TESTopicInfo::Response* ResponseTemp;
				TESTopic* Topic;
				TESTopicInfo* TopicInfo;
				struct AUDIO
				{
					BGSVoiceType* VoiceType;
					char pad08[0x4];
					char AudioFileName[MAX_PATH];
				} Audio;
			} *LPRESPONSE_DATA;

			bool ResponseWindow::HasOption() const
			{
				return false;
			}

			bool ResponseWindow::HasCanRuntimeDisabled() const
			{
				return false;
			}

			const char* ResponseWindow::GetOptionName() const
			{
				return nullptr;
			}

			const char* ResponseWindow::GetName() const
			{
				return "Response Window";
			}

			bool ResponseWindow::HasDependencies() const
			{
				return true;
			}

			Array<String> ResponseWindow::GetDependencies() const
			{
				return { "Console" };
			}

			bool ResponseWindow::QueryFromPlatform(EDITOR_EXECUTABLE_TYPE eEditorCurrentVersion,
				const char* lpcstrPlatformRuntimeVersion) const
			{
				return eEditorCurrentVersion <= EDITOR_FALLOUT_C4_LAST;
			}

			bool ResponseWindow::Activate(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem)
			{
				if (lpRelocationDatabaseItem->Version() == 1)
				{
					*(uintptr_t*)&_oldWndProc =
						Detours::X64::DetourFunctionClass(lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(0)),
							(uintptr_t)&HKWndProc);

					pointer_ResponseWindow_data = lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(1));
					pointer_ResponseWindow_sub1 = lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(2));
					pointer_ResponseWindow_sub2 = lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(3));
					pointer_ResponseWindow_sub3 = lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(4));

					// Hooking the jump. I don't allow the deny button.
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(5), (uintptr_t)&sub);
					// Then continue in the same spirit, remove the button.... skip it
					lpRelocator->PatchNop(lpRelocationDatabaseItem->At(6), 25);
					// In case of cancellation .wav triggers and closes the button, we will remove everything
					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(7), (uintptr_t)&sub);
					
					return true;
				}

				return false;
			}

			bool ResponseWindow::Shutdown(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem)
			{
				return false;
			}

			ResponseWindow::ResponseWindow() : BaseWindow(), Classes::CUIBaseWindow()
			{
				Assert(!GlobalResponseWindowPtr);
				GlobalResponseWindowPtr = this;
			}

			LRESULT CALLBACK ResponseWindow::HKWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
			{
				if (Message == WM_INITDIALOG)
				{
					GlobalResponseWindowPtr->m_hWnd = Hwnd;
					GlobalResponseWindowPtr->ListViewItems = GetDlgItem(Hwnd, 2168);
					
					auto LipGenPath = BSString::Utils::GetApplicationPath() + V_TOOLPATH;

					EnableLipGeneration = 
						Utils::FileExists((LipGenPath + V_WRAPPER).c_str()) &&
						Utils::FileExists((LipGenPath + V_FXDATA).c_str());

					if (!EnableLipGeneration)
						ConsolePatch::Log("LIPGEN: \"%s\", \"%s\" is missing from \"<GAMEDIR>%s\" directory."
							" LIP generation will be disabled.", V_WRAPPER, V_FXDATA, LipGenPath);
				}
				else if (Message == WM_COMMAND)
				{
					switch (const UINT32 param = LOWORD(wParam); param)
					{
						// "Generate Lip File"
						case 1016:
						// "From WAV"
						case 2379:
						{
							auto item = (RESPONSE_DATA::AUDIO*)
								EditorUI::ListViewGetSelectedItem(GlobalResponseWindowPtr->ListViewItems.Handle);

							if (!EnableLipGeneration || !item)
								return S_FALSE;

							// "Generate Lip File"
							if (param == 1016)
							{
								BSString AudioFilePath = BSString::Utils::GetApplicationPath() + item->AudioFileName;

								// only .wav
								AudioFilePath = BSString::Utils::ChangeFileExt(AudioFilePath, ".wav");
								if (!Utils::FileExists(AudioFilePath.c_str()))
								{
									ConsolePatch::Log("LIPGEN: File \"%s\" no found. Trying WAV extension fallback.",
										AudioFilePath.c_str());
									MessageBoxA(Hwnd, "Unable to find audio file on disk", "Error", MB_ICONERROR);

									return S_FALSE;
								}

								BSString LipFilePath = BSString::Utils::ChangeFileExt(AudioFilePath, ".lip");

								auto data = *(LPRESPONSE_DATA*)pointer_ResponseWindow_data;

								// It became unnecessary, and so i know everything, thanks RE
								// auto topic = fastCall<int64_t>(pointer_ResponseWindow_sub1, *(int64_t*)(data + 0x28), 
								//	 *(uint8_t*)(*(int64_t*)(data + 0x18) + 0x1A));
								// BSString InputText = fastCall<const char*>(pointer_ResponseWindow_sub2, topic);

								// Abandoning CreationKit32
								// fastCall<bool>(pointer_ResponseWindow_sub3, Hwnd, AudioFilePath.c_str(), InputText.c_str());

								if (!GenerationLip(AudioFilePath.c_str(), LipFilePath.c_str(), data->ResponseTemp->ResponseText.Get()) ||
									!Utils::FileExists(LipFilePath.c_str()))
								{
									ConsolePatch::Log("LIPGEN: File \"%s\" LIP generation failed.", LipFilePath.c_str());
									return S_FALSE;
								}
							}
						}
					}
				}
				else if (Message == WM_NOTIFY)
				{
					auto notify = (LPNMHDR)lParam;

					if (notify->code == LVN_ITEMCHANGED && notify->idFrom == 2168)
						EditorAPI::EditorUI::HKSendMessageA(Hwnd, WM_COMMAND, 2379, 0);
				}

				return CallWindowProc(GlobalResponseWindowPtr->GetOldWndProc(), Hwnd, Message, wParam, lParam);
			}

			void ResponseWindow::sub(HWND hWndButtonGenerate)
			{
				HWND hDlg = GetParent(hWndButtonGenerate);
				HWND hList = GetDlgItem(hDlg, 0x878);
				HWND hLTFCheck = GetDlgItem(hDlg, 0x94C);

				// no support .ltf
				EnableWindow(hLTFCheck, FALSE);
				// wav check default
				CheckDlgButton(hDlg, 0x94B, BST_CHECKED);

				bool bEnableGenerate = false;

				if (int iCount = ListView_GetItemCount(hList); iCount > 0) {
					if (auto item = (int64_t)EditorUI::ListViewGetSelectedItem(hList); item) {
						bEnableGenerate =
							IsDlgButtonChecked(hDlg, 2379) && *(uint32_t*)(item + 0x110) ||
							IsDlgButtonChecked(hDlg, 2380) && *(uint32_t*)(item + 0x118);
					}
				}

				EnableWindow(hWndButtonGenerate, bEnableGenerate);
			}

			bool ResponseWindow::GenerationLip(const char* AudioPath, const char* LipPath, const char* ResponseText)
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
				memset(&pi, 0, sizeof(pi));
				
				si.wShowWindow = SW_SHOW;

				auto LipGenPath = BSString::Utils::GetApplicationPath() + V_TOOLPATH;
				auto CmdLine = BSString::FormatString(V_CMD_FMT, (LipGenPath + V_WRAPPER).c_str(), AudioPath, LipPath, ResponseText);

				if (!CreateProcessA(
					NULL,
					const_cast<LPSTR>(CmdLine.c_str()),
					NULL,
					NULL,
					FALSE,
					NORMAL_PRIORITY_CLASS,
					NULL,
					LipGenPath.c_str(),
					&si,
					&pi)
					)
					return false;

				WaitForSingleObject(pi.hProcess, INFINITE);

				DWORD ExitCode = 0;
				//GetExitCodeProcess(pi.hProcess, &ExitCode);
				CloseHandle(pi.hProcess);

				return !ExitCode;
			}
		}
	}
}