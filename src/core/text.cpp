/*!
	Copyright (c) 2007, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.
	
	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#include "core/text.h"
#include "core/playable.h"

/*!
	\class CAText
	\brief Text sign
	
	Arbitrary text above or below playable elements.
*/

CAText::CAText( const QString s, CAPlayable *t )
 : CAMark( CAMark::Text, t ) {
	setText( s );
}

CAText::~CAText() {
}

CAMusElement* CAText::clone() {
	return new CAText( text(), static_cast<CAPlayable*>(associatedElement()) );
}

int CAText::compare(CAMusElement *elt) {
	if (elt->musElementType()!=CAMusElement::Mark)
		return -2;
	
	if (static_cast<CAMark*>(elt)->markType()!=CAMark::Text)
		return -1;
	
	if (static_cast<CAText*>(elt)->text()!=text())
		return 1;
	
	return 0;
}