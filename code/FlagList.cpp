#include "FlagList.h"
#include "ProfileManager.h"
#include "ids.h"

#include "wxLauncherSetup.h"

struct FlagInfo {
	wxString flag;
	wxString category;
	bool takesArg;
};
#include "FlagInfo.cpp"

#define WIDTH_OF_CHECKBOX 16

Flag::Flag() {
	this->checkbox = NULL;
}

#include <wx/listimpl.cpp> // Magic Incantation
WX_DEFINE_LIST(FlagList);

FlagListBox::FlagListBox(wxWindow* parent, SkinSystem *skin)
:wxVListBox(parent,wxID_ANY) {
	wxASSERT(skin != NULL);
	this->skin = skin;
	wxString tcPath, exeName;
	wxFileName exename;
	this->drawStatus = DRAW_OK;
	wxLogDebug(_T("Initializing FlagList"));
	if ( !ProMan::GetProfileManager()->Get()
		->Read(PRO_CFG_TC_ROOT_FOLDER, &tcPath) ) {
			this->drawStatus = MISSING_TC;
	} else if ( !ProMan::GetProfileManager()->Get()
		->Read(PRO_CFG_TC_CURRENT_BINARY, &exeName)) {
			this->drawStatus = MISSING_EXE;
	} else {
		exename.Assign(tcPath, exeName);
		if (!exename.FileExists()) {
			this->drawStatus = INVALID_BINARY;
		} else {
			wxArrayString output;
			wxString commandline = wxString::Format(_T("%s -get_flags"), exename.GetFullPath().c_str());
			long ret = ::wxExecute(commandline, output, 0);
			wxLogDebug(_T(" FSO returned %d when polled for the flags"), ret);

			wxFileName flagfile(tcPath, _T("flags.lch"));
			if ( !flagfile.FileExists() ) {
				this->drawStatus = FLAG_FILE_NOT_GENERATED;
			} else {
				this->drawStatus = this->ParseFlagFile(flagfile);
				if ( this->drawStatus == DRAW_OK ) {
					::wxRemoveFile(flagfile.GetFullPath());
					this->SetItemCount(this->allSupportedFlags.GetCount());
				}
			}
		}
	}
	this->errorText = new wxStaticText(this, wxID_ANY, wxEmptyString);
	if ( this->drawStatus != DRAW_OK ) {
		this->SetItemCount(1);
	}
}

FlagListBox::DrawStatus FlagListBox::ParseFlagFile(wxFileName &flagfilename) {
	wxCHECK_MSG(flagfilename.FileExists(), FLAG_FILE_NOT_GENERATED,
		_T("The flag file was not generated by the fso binary"));

	wxFile flagfile(flagfilename.GetFullPath());
	// Flagfile requires that we use 32 bit little-endian numbers
	wxInt32 easy_flag_size, flag_size, num_easy_flags, num_flags;
	size_t bytesRead;

	bytesRead = flagfile.Read(&easy_flag_size, sizeof(easy_flag_size));
	if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_flag_size) ) {
		wxLogError(_T(" Flag file is too short (failed to read easy_flag_size)"));
		return FLAG_FILE_NOT_VALID;
	}
	if ( easy_flag_size != 32 ) {
		wxLogError(_T("  Easy Flag size (%d) is not supported"), easy_flag_size);
		return FLAG_FILE_NOT_SUPPORTED;
	}

	bytesRead = flagfile.Read(&flag_size, sizeof(flag_size));
	if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(flag_size) ) {
		wxLogError(_T(" Flag file is too short (failed to read flag_size)"));
		return FLAG_FILE_NOT_VALID;
	}
	if ( flag_size != 344 ) {
		wxLogError(_T(" Exe flag structure (%d) size is not supported"), flag_size);
		return FLAG_FILE_NOT_SUPPORTED;
	}

	bytesRead = flagfile.Read(&num_easy_flags, sizeof(num_easy_flags));
	if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(num_easy_flags) ) {
		wxLogError(_T(" Flag file is too short (failed to read num_easy_flags)"));
		return FLAG_FILE_NOT_VALID;
	}

	for ( int i = 0; i < num_easy_flags; i++ ) {
		char easy_flag[32];
		bytesRead = flagfile.Read(&easy_flag, sizeof(easy_flag));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_flag) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_flag)"), sizeof(easy_flag), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}
		wxString flag(easy_flag, wxConvUTF8, sizeof(easy_flag));
		this->easyflags.Add(flag);
	}

	bytesRead = flagfile.Read(&num_flags, sizeof(num_flags));
	if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(num_flags) ) {
		wxLogError(_T(" Flag file is too short (failed to read num_flags)"));
		return FLAG_FILE_NOT_VALID;
	}

	for ( int i = 0; i < num_flags; i++ ) {
		char flag_string[20];
		char description[40];
		wxInt32 fso_only, easy_on_flags, easy_off_flags;
		char easy_catagory[16], web_url[256];

		bytesRead = flagfile.Read(&flag_string, sizeof(flag_string));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(flag_string) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (flag_string)"), sizeof(flag_string), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&description, sizeof(description));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(description) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (description)"), sizeof(description), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&fso_only, sizeof(fso_only));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(fso_only) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (fso_only)"), sizeof(fso_only), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&easy_on_flags, sizeof(easy_on_flags));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_on_flags) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_on_flags)"), sizeof(easy_on_flags), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&easy_off_flags, sizeof(easy_off_flags));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_off_flags) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_off_flags)"), sizeof(easy_off_flags), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&easy_catagory, sizeof(easy_catagory));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_catagory) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_catagory)"), sizeof(easy_catagory), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&web_url, sizeof(web_url));
		if ( wxInvalidOffset == bytesRead || bytesRead != sizeof(web_url) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (web_url)"), sizeof(web_url), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		Flag* flag = new Flag();
		flag->checkbox = new wxCheckBox(this, wxID_ANY, wxEmptyString);
		flag->checkbox->Hide();
		flag->checkboxSizer = new wxBoxSizer(wxVERTICAL);
		flag->checkboxSizer->AddStretchSpacer(1);
		flag->checkboxSizer->Add(flag->checkbox);
		flag->checkboxSizer->AddStretchSpacer(1);

		flag->isRecomendedFlag = true; //!< \todo implement logic to do the recomnmended flag
		flag->flagString = wxString(flag_string, wxConvUTF8, sizeof(flag_string));
		flag->shortDescription = wxString(description, wxConvUTF8, sizeof(description));
		flag->webURL = wxString(web_url, wxConvUTF8, sizeof(web_url));
		flag->fsoCatagory = wxString(easy_catagory, wxConvUTF8, sizeof(easy_catagory));

		this->allSupportedFlags.Append(flag);
	}		

	wxLogDebug(_T(" easy_flag_size: %d, %d; flag_size: %d, %d; num_easy_flags: %d, %d; num_flags: %d, %d"),
		easy_flag_size, sizeof(easy_flag_size),
		flag_size, sizeof(flag_size),
		num_easy_flags, sizeof(num_easy_flags),
		num_flags, sizeof(num_flags));

	wxByte buildCaps;
	bytesRead = flagfile.Read(&buildCaps, sizeof(buildCaps));
	if ( wxInvalidOffset == bytesRead ) {
		wxLogInfo(_T(" Old build that does not output its capabilities, must not support openAL"));
		buildCaps = 0;
	}

	return DRAW_OK;
}

FlagListBox::~FlagListBox() {
	FlagList::iterator iter = this->allSupportedFlags.begin();
	while ( iter != this->allSupportedFlags.end() ) {
		Flag *flag = *iter;
		delete flag;
		iter++;
	}
	this->allSupportedFlags.Clear();
}

void FlagListBox::OnDrawItem(wxDC &dc, const wxRect &rect, size_t n) const {
	if ( this->drawStatus == DRAW_OK ) {
		this->errorText->Hide();
		Flag* item = this->allSupportedFlags.Item(n)->GetData();
		wxCHECK_RET(item != NULL, _T("Flag pointer is null"));
	
		if ( item->isRecomendedFlag ) {
			dc.DrawBitmap(this->skin->GetIdealIcon(), rect.x, rect.y);
		}

		item->checkbox->Show();
		item->checkboxSizer->SetDimension(
			rect.x + SkinSystem::IdealIconWidth,
			rect.y,
			SkinSystem::IdealIconWidth,
			rect.height);

		if ( item->shortDescription.IsEmpty() ) {
			dc.DrawText(item->flagString, rect.x + SkinSystem::IdealIconWidth + WIDTH_OF_CHECKBOX, rect.y);
		} else {
			dc.DrawText(item->shortDescription, rect.x + SkinSystem::IdealIconWidth + WIDTH_OF_CHECKBOX, rect.y);
		}
	} else {
		wxASSERT_MSG( n == 0, _T("FLAGLISTBOX: Trying to draw background n != 0") );
	}
}

wxCoord FlagListBox::OnMeasureItem(size_t WXUNUSED(n)) const {
	if ( this->drawStatus == DRAW_OK ) {
		return SkinSystem::IdealIconHeight;
	} else {
		return this->GetSize().y;
	}
}

void FlagListBox::OnDrawBackground(wxDC &dc, const wxRect &rect, size_t n) const {
	dc.DestroyClippingRegion();
	if ( this->drawStatus == DRAW_OK && this->IsSelected(n) ) {
		wxColour highlighted = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
		wxBrush b(highlighted);
		dc.SetPen(wxPen(highlighted));
		dc.SetBackground(b);
		dc.SetBrush(b);
		dc.DrawRoundedRectangle(rect, 10.0);
	} else {
		dc.DestroyClippingRegion();
		wxColour background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		wxBrush b(background);
		dc.SetPen(wxPen(background));
		dc.SetBrush(b);
		dc.SetBackground(b);
		dc.DrawRectangle(rect);
	}
}

void FlagListBox::OnSize(wxSizeEvent &event) {
	wxVListBox::OnSize(event); // call parents onSize
	if ( this->drawStatus != DRAW_OK ) {
		wxRect rect(0, 0, this->GetSize().x, this->GetSize().y);

		wxString msg;
		switch(this->drawStatus) {
			case MISSING_TC:
				msg = _("No Total Conversion has been selected.\nPlease select a Total Conversion on the Basic Settings tab.");
				break;
			case MISSING_EXE:
				msg = _("No Executable has been selected.\nPlease select an Executable on the Basic Settings tab.");
				break;
			case INVALID_BINARY:
				msg = _("Selected binary does not exist.\nPlease choose another on the Basic Settings tab.");
				break;
			case WAITING_FOR_FLAGFILE:
				msg = _("Waiting for flagfile to be produced and parsed.");
				break;
			case FLAG_FILE_NOT_GENERATED:
				msg = _("A flag file was not generated by the executable.\nMake sure that it is a FreeSpace Open excecutable.");
				break;
			case FLAG_FILE_NOT_VALID:
				msg = _("Generated flag file was not complete.\nPlease talk to a maintainer of this launcher, you probably found a bug.");
				break;
			case FLAG_FILE_NOT_SUPPORTED:
				msg = _("Generated flag file is not supported.\nUpdate the launcher or talk to a maintainer of this launcher if you have the most recent version of the launcher.");
				break;
			default:
				msg = _("Unkown error occured while obtaing the flags from the FreeSpace Open excutable");
				break;
		}
		this->errorText->Show();
		this->errorText->SetLabel(msg);
		wxFont errorFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
		this->errorText->SetFont(errorFont);

		this->errorText->SetSize(rect, wxSIZE_FORCE);
		this->errorText->Wrap(rect.width);
	}
}

BEGIN_EVENT_TABLE(FlagListBox, wxVListBox)
EVT_SIZE(FlagListBox::OnSize)
END_EVENT_TABLE()
