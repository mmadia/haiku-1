/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#ifndef _ShowImageStatusView_h
#define _ShowImageStatusView_h

#include <View.h>

class ShowImageStatusView : public BView
{
public:
	ShowImageStatusView(BRect r, const char* name, uint32 resizingMode, uint32 flags);
	
	virtual void Draw(BRect updateRect);
	
	void SetCaption( char * Caption );
	
private:
    char * m_caption;
};

#endif /* _ShowImageStatusView_h */
