/*
Copyright (C) 2009-2010 wxLauncher Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef BASICSETTINGSPAGE_H
#define BASICSETTINGSPAGE_H

#include <vector>

#include <wx/wx.h>

#include "controls/TruncatableChoice.h"

#include "global/ModDefaults.h"

class BasicSettingsPage : public wxPanel {
public:
	BasicSettingsPage(wxWindow* parent);
	~BasicSettingsPage();

	void OnSelectTC(wxCommandEvent &event);
	void OnTCChanged(wxCommandEvent &event);

	void OnSelectExecutable(wxCommandEvent &event);
	void OnPressExecutableChoiceRefreshButton(wxCommandEvent &event);
	void OnSelectFredExecutable(wxCommandEvent &event);
	void OnPressFredExecutableChoiceRefreshButton(wxCommandEvent &event);

	void OnSelectVideoResolution(wxCommandEvent &event);
	void OnSelectVideoDepth(wxCommandEvent &event);
	void OnSelectVideoAnisotropic(wxCommandEvent &event);
	void OnSelectVideoAntiAlias(wxCommandEvent &event);
	void OnSelectVideoTextureFilter(wxCommandEvent &event);

	void OnSelectSpeechVoice(wxCommandEvent &event);
	void OnChangeSpeechVolume(wxCommandEvent &event);
	void OnPlaySpeechText(wxCommandEvent &event);
	void OnToggleSpeechInTechroom(wxCommandEvent &event);
	void OnToggleSpeechInBriefing(wxCommandEvent &event);
	void OnToggleSpeechInGame(wxCommandEvent &event);
	void OnToggleSpeechInMulti(wxCommandEvent &event);
	void OnGetMoreVoices(wxCommandEvent &event);

	void OnSelectNetworkType(wxCommandEvent &event);
	void OnSelectNetworkSpeed(wxCommandEvent &event);
	void OnChangePort(wxCommandEvent &event);
	void OnChangeIP(wxCommandEvent &event);

	void OnSelectSoundDevice(wxCommandEvent &event);
	void OnSelectCaptureDevice(wxCommandEvent &event);
	void OnToggleEnableEFX(wxCommandEvent &event);
	void OnChangeSampleRate(wxCommandEvent &event);
	void OnDownloadOpenAL(wxCommandEvent& event);
	void OnDetectOpenAL(wxCommandEvent& event);

	void OnSelectJoystick(wxCommandEvent& event);
	void OnCheckForceFeedback(wxCommandEvent& event);
	void OnCheckDirectionalHit(wxCommandEvent& event);
	void OnCalibrateJoystick(wxCommandEvent& event);
	void OnDetectJoystick(wxCommandEvent& event);

	void ProfileChanged(wxCommandEvent& event);
	void OnFlagFileProcessingStatusChanged(wxCommandEvent& event);
	void OnFREDEnabledChanged(wxCommandEvent& event);
	void OnActiveModChanged(wxCommandEvent& event);

private:
	static void FillFSOExecutableDropBox(wxChoice* exeChoice, wxFileName path);
	static void FillFredExecutableDropBox(wxChoice* exeChoice, wxFileName path);
	static void FillExecutableDropBox(wxChoice* exeChoice, wxArrayString exes);
	
	void SetUpResolution(
		long minHorizRes = DEFAULT_MOD_RESOLUTION_MIN_HORIZONTAL_RES,
		long minVertRes  = DEFAULT_MOD_RESOLUTION_MIN_VERTICAL_RES);
	static void FillResolutionDropBox(wxChoice* resChoice, long minHorizRes, long minVertRes);
	static int GetMaxSupportedResolution(const wxChoice& resChoice, long& width, long& height);
	
	enum ReasonForExecutableDisabling {
		MISSING_TC_ROOT_FOLDER,
		NONEXISTENT_TC_ROOT_FOLDER,
		INVALID_TC_ROOT_FOLDER
	};
	
	void InitializeMemberVariables();
	void DisableExecutableChoiceControls(const ReasonForExecutableDisabling reason);
	void OnCurrentBinaryChanged(wxCommandEvent& event);
	void OnCurrentFredBinaryChanged(wxCommandEvent& event);
	void ShowSettings(const bool showSettings);
	void OpenNonSCPWebSite(wxString url);
	
	enum SoundDeviceType {
		PLAYBACK,
		CAPTURE
	};
	
	void InitializeSoundDeviceDropDownBox(const SoundDeviceType deviceType);
	void SetupOpenALSection();
	void SetupJoystickSection();
	void SetupControlsForJoystick(unsigned int i);
	
	DECLARE_EVENT_TABLE();

private:
	wxStaticText* soundDeviceText;
	TruncatableChoice* soundDeviceCombo;
	wxStaticText* captureDeviceText;
	TruncatableChoice* captureDeviceCombo;
	wxStaticText* openALVersion;
	wxButton* downloadOpenALButton;
	wxButton* detectOpenALButton;

	wxSizer* audioButtonsSizer;
	wxSizer* audioOldSoundSizer;
	wxSizer* audioNewSoundSizer;
	wxSizer* audioNewSoundDeviceSizer;
	wxSizer* audioSizer;

	wxChoice* joystickSelected;
#if IS_WIN32
	wxCheckBox* joystickForceFeedback;
	wxCheckBox* joystickDirectionalHit;
	wxButton* joystickCalibrateButton;
	wxButton* joystickDetectButton;
#endif
	bool isTcRootFolderValid;
	bool isCurrentBinaryValid;
	bool isCurrentFredBinaryValid;
};

#endif
