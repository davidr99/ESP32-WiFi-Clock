#include <Arduino.h>
#include <SPI.h>

#define min(a,b) ((a)<(b)?(a):(b))

#ifndef FT800_Onscreen_Keyboard
#define FT800_Onscreen_Keyboard

	#include <FT_VM800B35.h>

	#define	TAG_NONE		-1
	#define TAG_CAPS        1
	#define TAG_BACKSPACE   2
	#define TAG_SPACE       3
	#define TAG_NUMBERS     4
	#define TAG_CLEAR       5
	#define TAG_TEXT        6
	#define TAG_DONE        7
	#define TAG_EXIT        253
	#define TAG_NOTHING     0
	#define TAG_UNKNOWN     255
	
	//template<class FT_Trans>
	class OnscreenKeyboard
	{
		public:
			OnscreenKeyboard(FT800IMPL_SPI *imp);
			
			void UpdateScreen(int tagId = -1);
			void SetName(char *name);
			char * GetText();
			void ClearText();
			bool IsClosed();
			bool IsCanceled();
			void Reset();
		
		protected:
			void UpdateUI();
			
			int _lastTagId;
			bool _caps = false;
			bool _numbers = false;
			bool _isClosed = false;
			bool _isCanceled = false;
			char _text[255];
			char _name[255];
			FT800IMPL_SPI *_ft800Imp;
		
	};

#endif
