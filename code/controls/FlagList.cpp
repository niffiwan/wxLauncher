#include "controls/FlagList.h"
#include "tabs/AdvSettingsPage.h"
#include "apis/ProfileManager.h"
#include "global/ids.h"

#include "global/MemoryDebugging.h"

struct FlagInfo {
	wxString flag;
	wxString category;
	bool takesArg;
};
#include "datastructures/FlagInfo.cpp"

#define WIDTH_OF_CHECKBOX 16

Flag::Flag() {
	this->checkbox = NULL;
}

#include <wx/listimpl.cpp> // Magic Incantation
WX_DEFINE_LIST(FlagList);

#include <wx/listimpl.cpp> // Magic Incantation
WX_DEFINE_LIST(FlagCategoryList);

FlagSet::FlagSet(wxString Name) {
	this->Name = Name;
}

#include <wx/listimpl.cpp> // Magic Incantation
WX_DEFINE_LIST(FlagSetsList);

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
					
					size_t itemCount = 0;
					FlagCategoryList::iterator iter =
						this->allSupportedFlagsByCategory.begin();
					while ( iter != this->allSupportedFlagsByCategory.end() ) {
						itemCount += (*iter)->flags.GetCount();
						iter++;
					}
					this->SetItemCount(itemCount);
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
	if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_flag_size) ) {
		wxLogError(_T(" Flag file is too short (failed to read easy_flag_size)"));
		return FLAG_FILE_NOT_VALID;
	}
	if ( easy_flag_size != 32 ) {
		wxLogError(_T("  Easy Flag size (%d) is not supported"), easy_flag_size);
		return FLAG_FILE_NOT_SUPPORTED;
	}

	bytesRead = flagfile.Read(&flag_size, sizeof(flag_size));
	if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(flag_size) ) {
		wxLogError(_T(" Flag file is too short (failed to read flag_size)"));
		return FLAG_FILE_NOT_VALID;
	}
	if ( flag_size != 344 ) {
		wxLogError(_T(" Exe flag structure (%d) size is not supported"), flag_size);
		return FLAG_FILE_NOT_SUPPORTED;
	}

	bytesRead = flagfile.Read(&num_easy_flags, sizeof(num_easy_flags));
	if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(num_easy_flags) ) {
		wxLogError(_T(" Flag file is too short (failed to read num_easy_flags)"));
		return FLAG_FILE_NOT_VALID;
	}

	for ( int i = 0; i < num_easy_flags; i++ ) {
		char easy_flag[32];
		bytesRead = flagfile.Read(&easy_flag, sizeof(easy_flag));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_flag) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_flag)"), sizeof(easy_flag), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}
		wxString flag(easy_flag, wxConvUTF8, sizeof(easy_flag));
		this->easyflags.Add(flag);
	}

	bytesRead = flagfile.Read(&num_flags, sizeof(num_flags));
	if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(num_flags) ) {
		wxLogError(_T(" Flag file is too short (failed to read num_flags)"));
		return FLAG_FILE_NOT_VALID;
	}

	for ( int i = 0; i < num_flags; i++ ) {
		char flag_string[20];
		char description[40];
		wxInt32 fso_only, easy_on_flags, easy_off_flags;
		char easy_catagory[16], web_url[256];

		bytesRead = flagfile.Read(&flag_string, sizeof(flag_string));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(flag_string) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (flag_string)"), sizeof(flag_string), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&description, sizeof(description));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(description) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (description)"), sizeof(description), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&fso_only, sizeof(fso_only));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(fso_only) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (fso_only)"), sizeof(fso_only), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&easy_on_flags, sizeof(easy_on_flags));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_on_flags) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_on_flags)"), sizeof(easy_on_flags), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&easy_off_flags, sizeof(easy_off_flags));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_off_flags) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_off_flags)"), sizeof(easy_off_flags), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&easy_catagory, sizeof(easy_catagory));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(easy_catagory) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (easy_catagory)"), sizeof(easy_catagory), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		bytesRead = flagfile.Read(&web_url, sizeof(web_url));
		if ( (size_t)wxInvalidOffset == bytesRead || bytesRead != sizeof(web_url) ) {
			wxLogError(_T(" Flag file is too short, expected %d, got %d bytes (web_url)"), sizeof(web_url), bytesRead);
			return FLAG_FILE_NOT_VALID;
		}

		flag_string[sizeof(flag_string)-1] = _T('\0');
		description[sizeof(description)-1] = _T('\0');
		easy_catagory[sizeof(easy_catagory)-1] = _T('\0');
		web_url[sizeof(web_url)-1] = _T('\0');

		Flag* flag = new Flag();
		flag->checkbox = new wxCheckBox(this, wxID_ANY, wxEmptyString);
		flag->checkbox->Hide();
		this->m_parent->Connect(flag->checkbox->GetId(),
			wxEVT_COMMAND_CHECKBOX_CLICKED,
			wxCommandEventHandler(AdvSettingsPage::OnNeedUpdateCommandLine));
		flag->checkboxSizer = new wxBoxSizer(wxVERTICAL);
		flag->checkboxSizer->AddStretchSpacer(1);
		flag->checkboxSizer->Add(flag->checkbox);
		flag->checkboxSizer->AddStretchSpacer(1);

		flag->isRecomendedFlag = true; //!< \todo implement logic to do the recomnmended flag
		flag->flagString = wxString(flag_string, wxConvUTF8, strlen(flag_string));
		flag->shortDescription = wxString(description, wxConvUTF8, strlen(description));
		flag->webURL = wxString(web_url, wxConvUTF8, strlen(web_url));
		flag->fsoCatagory = wxString(easy_catagory, wxConvUTF8, strlen(easy_catagory));

		flag->easyDisable = easy_off_flags;
		flag->easyEnable = easy_on_flags;

		FlagCategoryList::iterator iter = this->allSupportedFlagsByCategory.begin();
		while ( iter != this->allSupportedFlagsByCategory.end() ) {
			if ( flag->fsoCatagory == (*iter)->categoryName ) {
				break;
			}
			iter++;
		}
		if ( iter == this->allSupportedFlagsByCategory.end() ) {
			// did not find the category add it
			FlagCategory* flagCat = new FlagCategory();
			flagCat->categoryName = flag->fsoCatagory;

			Flag* headFlag = new Flag();
			headFlag->fsoCatagory = flag->fsoCatagory;
			headFlag->checkbox = NULL;
			flagCat->flags.Append(headFlag);
			flagCat->flags.Append(flag);
			this->allSupportedFlagsByCategory.Append(flagCat);
		} else {
			(*iter)->flags.Append(flag);
		}
	}		

	wxLogDebug(_T(" easy_flag_size: %d, %d; flag_size: %d, %d; num_easy_flags: %d, %d; num_flags: %d, %d"),
		easy_flag_size, sizeof(easy_flag_size),
		flag_size, sizeof(flag_size),
		num_easy_flags, sizeof(num_easy_flags),
		num_flags, sizeof(num_flags));

	wxByte buildCaps;
	bytesRead = flagfile.Read(&buildCaps, sizeof(buildCaps));
	if ( (size_t)wxInvalidOffset == bytesRead ) {
		wxLogInfo(_T(" Old build that does not output its capabilities, must not support openAL"));
		buildCaps = 0;
	}

	return DRAW_OK;
}

FlagListBox::~FlagListBox() {
	FlagCategoryList::iterator catIter = this->allSupportedFlagsByCategory.begin();
	while ( catIter != this->allSupportedFlagsByCategory.end() ) {
		FlagCategory* category = *catIter;
		FlagList::iterator iter = category->flags.begin();
		while ( iter != category->flags.end() ) {
			Flag *flag = *iter;
			delete flag;
			iter++;
		}
		category->flags.Clear();

		delete category;
		catIter++;
	}
	this->allSupportedFlagsByCategory.Clear();
}

void FlagListBox::FindFlagAt(size_t n, Flag **flag, Flag ** catFlag) const {
	size_t index = n;
	FlagCategoryList::const_iterator iter =
		this->allSupportedFlagsByCategory.begin();
	while ( iter != this->allSupportedFlagsByCategory.end() ) {
		FlagCategory* cat = *iter;
		if ( cat->flags.GetCount() > index ) {
			(*flag) = cat->flags.Item(index)->GetData();
			if ( catFlag != NULL ) {
				// cat flag is first in the list
				(*catFlag) = cat->flags.Item(0)->GetData(); 
			}
			break;
		} else {
			index -= cat->flags.GetCount();
		}
		iter++;
	}
}

void FlagListBox::OnDrawItem(wxDC &dc, const wxRect &rect, size_t n) const {
	if ( this->drawStatus == DRAW_OK ) {
		this->errorText->Hide();
		Flag* item = NULL;
		this->FindFlagAt(n, &item, NULL);		
		wxCHECK_RET(item != NULL, _T("Flag pointer is null"));
	
		if ( item->isRecomendedFlag ) {
			dc.DrawBitmap(this->skin->GetIdealIcon(), rect.x, rect.y);
		}

		if (item->checkbox != NULL) {
			item->checkbox->Show();
			item->checkboxSizer->SetDimension(
				rect.x + SkinSystem::IdealIconWidth,
				rect.y,
				SkinSystem::IdealIconWidth,
				rect.height);
		}
		if ( item->flagString.IsEmpty() ) {
			// draw a category
			dc.DrawText(item->fsoCatagory, rect.x + SkinSystem::IdealIconWidth + WIDTH_OF_CHECKBOX, rect.y);
		} else if ( item->shortDescription.IsEmpty() ) {
			dc.DrawText(item->flagString, rect.x + SkinSystem::IdealIconWidth + WIDTH_OF_CHECKBOX, rect.y);
		} else {
			dc.DrawText(item->shortDescription, rect.x + SkinSystem::IdealIconWidth + WIDTH_OF_CHECKBOX, rect.y);
		}
	} else {
		wxASSERT_MSG( n == 0, _T("FLAGLISTBOX: Trying to draw background n != 0") );
	}
}

wxCoord FlagListBox::OnMeasureItem(size_t n) const {
	if ( this->drawStatus == DRAW_OK ) {
		return SkinSystem::IdealIconHeight;
	} else {
		return this->GetSize().y;
	}
}

void FlagListBox::OnDrawBackground(wxDC &dc, const wxRect &rect, size_t n) const {
	Flag* item = NULL;
	this->FindFlagAt(n, &item, NULL);
	if ( item != NULL && item->flagString.IsEmpty() ) {
		dc.DestroyClippingRegion();
		wxColour background = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
		wxBrush b(background);
		dc.SetPen(wxPen(background));
		dc.SetBrush(b);
		dc.SetBackground(b);
		dc.DrawRectangle(rect);
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

void FlagListBox::OnCheckCategoryBox(wxCommandEvent &WXUNUSED(event)) {
	this->RefreshRect(this->GetRect(), true);
}

wxString FlagListBox::GenerateStringList() {
	wxString flagList;
	FlagCategoryList::const_iterator cat =
		this->allSupportedFlagsByCategory.begin();
	while ( cat != this->allSupportedFlagsByCategory.end() ) {
		FlagList::const_iterator flags =
			(*cat)->flags.begin();
		while ( flags != (*cat)->flags.end() ) {
			if ( (*flags)->checkbox != NULL
				&& (*flags)->checkbox->IsChecked() 
				&& !(*flags)->flagString.IsEmpty() ) {
					if ( !flagList.IsEmpty() ) {
						flagList += _T(" ");
					}
					flagList += (*flags)->flagString;
			}
			flags++;
		}
		cat++;
	}
	return flagList;
}

bool FlagListBox::SetFlag(wxString flagString, bool state) {
	FlagCategoryList::const_iterator category =
		this->allSupportedFlagsByCategory.begin();
	while (category != this->allSupportedFlagsByCategory.end()) {
		FlagList::const_iterator flag = (*category)->flags.begin();
		while( flag != (*category)->flags.end() ) {
			if ( !(*flag)->flagString.IsEmpty()
				&& (*flag)->flagString == flagString ) {
					(*flag)->checkbox->SetValue(state);
					return true;
			}
			flag++;
		}
		category++;
	}
	return false;
}

BEGIN_EVENT_TABLE(FlagListBox, wxVListBox)
EVT_SIZE(FlagListBox::OnSize)
END_EVENT_TABLE()

bool FlagListBox::SetFlagSet(wxString setToFind) {
	if ( this->flagSets.GetCount() == 0 ) {
		this->generateFlagSets();
	}
	FlagSetsList::const_iterator flagSetsIter =
		this->flagSets.begin();
	FlagSet* sets = NULL; 
	while(flagSetsIter != this->flagSets.end()) {
		if ( (*flagSetsIter)->Name.StartsWith(setToFind) ) {
			sets = *flagSetsIter;
		}
		flagSetsIter++;
	}
	if ( sets == NULL ) {
		// never found the set
		return false;
	}

	wxArrayString::const_iterator disableIter =
		sets->FlagsToDisable.begin();
	while ( disableIter != sets->FlagsToDisable.end() ) {
		this->SetFlag(*disableIter, false);
		disableIter++;
	}
	wxArrayString::const_iterator enableIter =
		sets->FlagsToEnable.begin();
	while ( enableIter != sets->FlagsToEnable.end() ) {
		this->SetFlag(*enableIter, true);
		enableIter++;
	}
	return true;
}

void FlagListBox::generateFlagSets() {
	this->flagSets.clear();
	// \todo include the flag sets of the mod.inis as well

	// custom
	{
		FlagSet* flagSetCustom = new FlagSet(_("Custom"));
		this->flagSets.Append(flagSetCustom);
	}
	// the easy flags.
	wxUint32 counter = 0;
	wxArrayString::const_iterator easyIter =
		this->easyflags.begin();
	while ( easyIter != this->easyflags.end() ) {
		wxString easyFlag = *easyIter;

		if ( easyFlag.StartsWith(_T("Custom")) ) {
			// do nothing, we already have a custom
		} else {
			FlagSet* flagSet = new FlagSet(easyFlag);
			FlagCategoryList::const_iterator catIter =
				this->allSupportedFlagsByCategory.begin();

			while ( catIter != this->allSupportedFlagsByCategory.end() ) {
				FlagList::const_iterator flagIter =
					(*catIter)->flags.begin();

				while ( flagIter != (*catIter)->flags.end() ) {
					Flag* flag = *flagIter;

					if ( !flag->flagString.IsEmpty()
						&& (flag->easyEnable & counter) > 0 ) {
						flagSet->FlagsToEnable.Add(flag->flagString);
					}
					if ( !flag->flagString.IsEmpty()
						&& (flag->easyDisable & counter) > 0 ) {
						flagSet->FlagsToDisable.Add(flag->flagString);
					}
					flagIter++;
				}
				catIter++;
			}
			this->flagSets.Append(flagSet);
		}

		if (counter < 1) {
			counter = 2; // prime the counter so we can bitshift for the rest
		} else {
			counter = counter << 1;
		}
		if ( counter > (wxUint32)(1 << 31) ) {
			// we have checked 31 bits of counter, this is too many easy flag sets
			easyIter = this->easyflags.end();
			wxLogError(_T("FSO executable has more than 31 easy flag categories"));
		} else {
			easyIter++;
		}
	}
}

wxArrayString& FlagListBox::GetFlagSets(wxArrayString &arr) {
	if ( this->flagSets.size() == 0 ) {
		this->generateFlagSets();
	}
	FlagSetsList::const_iterator flagSetsIter =
		this->flagSets.begin();
	while ( flagSetsIter != this->flagSets.end() ) {
		arr.Add((*flagSetsIter)->Name);
		flagSetsIter++;
	}
	return arr;
}

/** returns true when the FlagList will draw the the actual list,
false when the FlagList is showing an error message. */
bool FlagListBox::IsDrawOK() {
	return (this->drawStatus == DRAW_OK);
}