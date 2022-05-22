#define min(a,b) ((a)<(b)?(a):(b))

#include <FT800_Onscreen_Keyboard.h>


//template<class FT_Trans>
OnscreenKeyboard::OnscreenKeyboard(FT800IMPL_SPI *imp)
{
	this->_ft800Imp = imp;
	strcpy(this->_name, "Change Me");
	this->Reset();
}

//template<class FT_Trans>
void OnscreenKeyboard::UpdateScreen(int tagId)
{
  if (tagId != this->_lastTagId || tagId == -1)
  {
    this->_lastTagId = tagId;

    switch(tagId)
    {
      case TAG_CAPS:
        this->_caps = !this->_caps;
        break;
      case TAG_BACKSPACE:
        {
          int length = strlen(this->_text);
          this->_text[length - 1] = 0x00;
          break;
        }
      case TAG_SPACE:
        sprintf(this->_text, "%s ", this->_text);
        break;
      case TAG_NUMBERS:
        this->_numbers = !this->_numbers;
        break;
      case TAG_CLEAR:
        this->ClearText();
        break;
      case TAG_TEXT:
        break;
	  case TAG_DONE:
		_isClosed = true;
		_isCanceled = false;
		break;
      case TAG_EXIT:
		_isCanceled = true;
		_isClosed = true;
        break;
	  case TAG_NONE:
      case TAG_UNKNOWN:
      case TAG_NOTHING:
        break;

      default:
        sprintf(this->_text, "%s%c", this->_text, tagId);
    }

    UpdateUI();
  }
}

//template<class FT_Trans>
void OnscreenKeyboard::UpdateUI()
{
	Serial.println("Update UI");
  this->_ft800Imp->DLStart();
  
  this->_ft800Imp->Begin(FT_RECTS);
  this->_ft800Imp->ColorRGB(0, 157, 255);
  this->_ft800Imp->Vertex2ii(477, 33, 0, 0);
  this->_ft800Imp->Vertex2ii(2, 2, 0, 0);
  this->_ft800Imp->End();
  
  this->_ft800Imp->ColorRGB(255, 255, 255);
  this->_ft800Imp->Cmd_Text(6, 8, 28, 0, this->_name);
  
  this->_ft800Imp->Tag(TAG_DONE);
  this->_ft800Imp->Cmd_Button(426, 76, 52, 36, 27, FT_OPT_FLAT, "Done");

  if (!this->_numbers)
  {
    if (this->_caps)
    {
      this->_ft800Imp->Cmd_Keys(0, 115, 480, 36, 29, FT_OPT_FLAT, "QWERTYUIOP");
      this->_ft800Imp->Cmd_Keys(10, 155, 469, 36, 29, FT_OPT_FLAT, "ASDFGHJKL");
      this->_ft800Imp->Cmd_Keys(55, 195, 367, 36, 29, FT_OPT_FLAT, "ZXCVBNM");
      this->_ft800Imp->Tag(TAG_CAPS);
      this->_ft800Imp->Cmd_Button(0, 195, 47, 36, 27, FT_OPT_FLAT, "a^");
    }
    else
    {
      this->_ft800Imp->Cmd_Keys(0, 115, 480, 36, 29, FT_OPT_FLAT, "qwertyuiop");
      this->_ft800Imp->Cmd_Keys(10, 155, 469, 36, 29, FT_OPT_FLAT, "asdfghjkl");
      this->_ft800Imp->Cmd_Keys(55, 195, 367, 36, 29, FT_OPT_FLAT, "zxcvbnm");
      this->_ft800Imp->Tag(TAG_CAPS);
      this->_ft800Imp->Cmd_Button(0, 195, 47, 36, 27, FT_OPT_FLAT, "A^");
    }
    
    this->_ft800Imp->Tag(TAG_NUMBERS);
    this->_ft800Imp->Cmd_Button(0, 235, 50, 36, 27, FT_OPT_FLAT, "12*");
  }
  else
  {
    this->_ft800Imp->Cmd_Keys(0, 115, 480, 36, 29, FT_OPT_FLAT, "1234567890");
    this->_ft800Imp->Cmd_Keys(10, 155, 469, 36, 29, FT_OPT_FLAT, "-@#$%^&*/");
    this->_ft800Imp->Cmd_Keys(55, 195, 367, 36, 29, FT_OPT_FLAT, ")_+[]{}");
      
    this->_ft800Imp->Tag(TAG_NUMBERS);
    this->_ft800Imp->Cmd_Button(0, 235, 50, 36, 27, FT_OPT_FLAT, "AB*");    
  }
  
  this->_ft800Imp->Tag(TAG_TEXT);
  this->_ft800Imp->Cmd_Text(0, 35, 28, 0, this->_text);
  

  this->_ft800Imp->Tag(TAG_SPACE);
  this->_ft800Imp->Cmd_Button(57, 235, 358, 36, 27, FT_OPT_FLAT, "Space");
  this->_ft800Imp->Tag(TAG_CLEAR);
  this->_ft800Imp->Cmd_Button(422, 235, 60, 36, 27, FT_OPT_FLAT, "Clear");
  this->_ft800Imp->Tag(TAG_BACKSPACE);
  this->_ft800Imp->Cmd_Button(433, 195, 47, 36, 27, FT_OPT_FLAT, "<-");
  this->_ft800Imp->Tag(TAG_EXIT);
  this->_ft800Imp->Cmd_Button(443, 4, 30, 24, 27, FT_OPT_FLAT, "X");
  
  this->_ft800Imp->DLEnd();
  this->_ft800Imp->Finish();
}

void OnscreenKeyboard::Reset()
{
	this->_isClosed = false;
	this->_isCanceled = false;
	this->ClearText();
}

bool OnscreenKeyboard::IsCanceled()
{
	return this->_isCanceled;
}

bool OnscreenKeyboard::IsClosed()
{
	return this->_isClosed;
}

//template<class FT_Trans>
void OnscreenKeyboard::ClearText()
{
	sprintf(this->_text, "");
}

//template<class FT_Trans>
void OnscreenKeyboard::SetName(char *name)
{
	stpcpy(this->_name, name);
}

//template<class FT_Trans>
char * OnscreenKeyboard::GetText()
{
	return this->_text;
}